/*
 *   Copyright (C) 2010 Michael Buesch <m@bues.ch>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "main.h"
#include "timer.h"
#include "args.h"
#include "api.h"
#include "log.h"
#include "conf.h"
#include "util.h"
#include "list.h"
#include "battery.h"
#include "backlight.h"
#include "devicelock.h"
#include "autodim.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/resource.h>


struct client {
	int fd;
	int notifications_enabled;
	struct list_head list;
};

static int socket_fd = -1;
static sig_atomic_t in_signal;
static int sig_block_count;
static LIST_HEAD(client_list);

struct backend backend;


static int send_message(struct client *c, struct pt_message *msg, uint16_t flags)
{
	int ret;
	unsigned int count, pos = 0;

	msg->flags |= htons(flags);
	count = sizeof(*msg);
	while (count) {
		ret = send(c->fd, ((uint8_t *)msg) + pos, count, 0);
		if (ret < 0) {
			logerr("Failed to send message to client, fd=%d\n",
			       c->fd);
			return -1;
		}
		count -= ret;
		pos += ret;
	}

	return 0;
}

static int enable_autodim(int max_percent)
{
	int err = 0;

	if (!backend.autodim) {
		err = -ENOMEM;
		backend.autodim = autodim_alloc();
		if (backend.autodim)
			err = autodim_init(backend.autodim, backend.backlight, backend.config);
		if (err) {
			autodim_free(backend.autodim);
			backend.autodim = NULL;
			return err;
		}
	}
	autodim_set_max_percent(backend.autodim, max_percent);

	return err;
}

static void disable_autodim(void)
{
	autodim_destroy(backend.autodim);
	autodim_free(backend.autodim);
	backend.autodim = NULL;
}

static void received_message(struct client *c, struct pt_message *msg)
{
	struct pt_message reply = {
		.id	= msg->id,
		.flags	= (msg->flags & ~htons(PT_FLG_OK)) | htons(PT_FLG_REPLY),
	};
	int err;

	switch (ntohs(msg->id)) {
	case PTREQ_PING:
		send_message(c, &reply, PT_FLG_OK);
		break;
	case PTREQ_WANT_NOTIFY:
		if (msg->flags & htons(PT_FLG_ENABLE))
			c->notifications_enabled = 1;
		else
			c->notifications_enabled = 0;
		send_message(c, &reply, PT_FLG_OK);
		break;
	case PTREQ_XEVREP:
		err = 0;
		if (msg->flags & htons(PT_FLG_ENABLE))
			err = xevrep_enable(&backend.xevrep);
		else
			xevrep_disable(&backend.xevrep);
		send_message(c, &reply, err ? 0 : PT_FLG_OK);
		break;
	case PTREQ_BL_GETSTATE:
		err = backlight_fill_pt_message_stat(backend.backlight,
						     &reply);
		send_message(c, &reply, err ? 0 : PT_FLG_OK);
		break;
	case PTREQ_BL_SETBRIGHTNESS:
		err = backend.backlight->set_brightness(backend.backlight,
							msg->bl_set.brightness);
		reply.error.code = htonl(err);
		send_message(c, &reply, err ? 0 : PT_FLG_OK);
		break;
	case PTREQ_BL_AUTODIM:
		err = 0;
		if (msg->bl_autodim.flags & htonl(PT_AUTODIM_FLG_ENABLE))
			err = enable_autodim(htonl(msg->bl_autodim.max_percent));
		else
			disable_autodim();
		reply.error.code = htonl(err);
		send_message(c, &reply, err ? 0 : PT_FLG_OK);
		break;
	case PTREQ_BAT_GETSTATE:
		err = battery_fill_pt_message_stat(backend.battery, &reply);
		send_message(c, &reply, err ? 0 : PT_FLG_OK);
		break;
	default:
		logerr("Received unknown message %u\n", ntohs(msg->id));
	}
}

static void notify_client(struct client *c, struct pt_message *msg, uint16_t flags)
{
	if (c->notifications_enabled)
		send_message(c, msg, flags);
}

void notify_clients(struct pt_message *msg, uint16_t flags)
{
	struct client *c;

	list_for_each_entry(c, &client_list, list)
		notify_client(c, msg, flags);
}

static struct client * new_client(int fd)
{
	struct client *c;

	c = zalloc(sizeof(*c));
	if (!c)
		return NULL;

	c->fd = fd;
	INIT_LIST_HEAD(&c->list);

	return c;
}

static void remove_client(struct client *c)
{
	list_del(&c->list);
	logdebug("Client disconnected, fd=%d\n", c->fd);
	free(c);
}

static void disconnect_client(struct client *c)
{
	struct pt_message msg = {
		.id	= htons(PTNOTI_SRVDOWN),
	};

	notify_client(c, &msg, PT_FLG_OK);
}

static void force_disconnect_clients(void)
{
	struct client *c, *c_tmp;

	list_for_each_entry_safe(c, c_tmp, &client_list, list) {
		disconnect_client(c);
		remove_client(c);
	}
}

static void recv_clients(void)
{
	struct client *c, *c_tmp;
	int count;
	struct pt_message msg;

	list_for_each_entry_safe(c, c_tmp, &client_list, list) {
		count = recv(c->fd, &msg, sizeof(msg), 0);
		if (count < 0)
			continue;
		if (count == 0) {
			remove_client(c);
			continue;
		}
		received_message(c, &msg);
	}
}

static void socket_accept(int fd)
{
	socklen_t socklen;
	struct sockaddr_un remoteaddr;
	int err, cfd;
	pid_t pid;
	struct client *c;

	socklen = sizeof(remoteaddr);
	cfd = accept(fd, (struct sockaddr *)&remoteaddr, &socklen);
	if (cfd == -1)
		return;
	/* connected */
	pid = getpid();
	err = ioctl(cfd, SIOCSPGRP, &pid);
	if (err) {
		logerr("Failed to set SIOCSPGRP: %s\n",
		       strerror(errno));
		goto error_close;
	}
	err = fcntl(cfd, F_SETFL, O_NONBLOCK | O_ASYNC);
	if (err) {
		logerr("Failed to set flags on client: %s\n",
		       strerror(errno));
		goto error_close;
	}
	c = new_client(cfd);
	if (!c)
		goto error_close;
	list_add_tail(&c->list, &client_list);
	logdebug("Client connected, fd=%d\n", cfd);

	return;

error_close:
	close(cfd);
}

static int new_socket(const char *path, unsigned int perm,
		      unsigned int nrlisten)
{
	struct sockaddr_un sockaddr;
	int err, fd;
	pid_t pid;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		logerr("Failed to create socket %s: %s\n",
		       path, strerror(errno));
		goto error;
	}
	pid = getpid();
	err = ioctl(fd, SIOCSPGRP, &pid);
	if (err) {
		logerr("Failed to set SIOCSPGRP: %s\n",
		       strerror(errno));
		goto error_close_sock;
	}
	err = fcntl(fd, F_SETFL, O_NONBLOCK | O_ASYNC);
	if (err) {
		logerr("Failed to set flags on socket %s: %s\n",
		       path, strerror(errno));
		goto error_close_sock;
	}
	if (cmdargs.force)
		unlink(path);
	sockaddr.sun_family = AF_UNIX;
	strncpy(sockaddr.sun_path, path, sizeof(sockaddr.sun_path) - 1);
	err = bind(fd, (struct sockaddr *)&sockaddr, SUN_LEN(&sockaddr));
	if (err) {
		logerr("Failed to bind socket to %s: %s\n",
		       path, strerror(errno));
		goto error_close_sock;
	}
	err = chmod(path, perm);
	if (err) {
		logerr("Failed to set %s socket permissions: %s\n",
		       path, strerror(errno));
		goto error_unlink_sock;
	}
	err = listen(fd, nrlisten);
	if (err) {
		logerr("Failed to listen on socket %s: %s\n",
		       path, strerror(errno));
		goto error_unlink_sock;
	}

	return fd;

error_unlink_sock:
	unlink(path);
error_close_sock:
	close(fd);
error:
	return -1;
}


static int create_socket(void)
{
	int err;

	err = mkdir(PT_SOCK_DIR, 0755);
	if (err && errno != EEXIST) {
		logerr("Failed to create directory %s: %s\n",
		       PT_SOCK_DIR, strerror(errno));
		return err;
	}

	socket_fd = new_socket(PT_SOCKET, 0666, 10);
	if (socket_fd == -1)
		goto err_rmdir;

	return 0;

err_rmdir:
	rmdir(PT_SOCK_DIR);
	return -1;
}

static void remove_socket(void)
{
	if (socket_fd != -1) {
		close(socket_fd);
		socket_fd = -1;
		unlink(PT_SOCKET);
		rmdir(PT_SOCK_DIR);
	}
}

static int create_pidfile(void)
{
	char buf[32] = { 0, };
	pid_t pid = getpid();
	int fd;
	ssize_t res;

	if (!cmdargs.pidfile)
		return 0;

	fd = open(cmdargs.pidfile, O_RDWR | O_CREAT | O_TRUNC, 0444);
	if (fd < 0) {
		logerr("Failed to create PID-file %s: %s\n",
		       cmdargs.pidfile, strerror(errno));
		return -1;
	}
	snprintf(buf, sizeof(buf), "%lu",
		 (unsigned long)pid);
	res = write(fd, buf, strlen(buf));
	close(fd);
	if (res != strlen(buf)) {
		logerr("Failed to write PID-file %s: %s\n",
		       cmdargs.pidfile, strerror(errno));
		return -1;
	}

	return 0;
}

static void remove_pidfile(void)
{
	if (cmdargs.pidfile) {
		unlink(cmdargs.pidfile);
		cmdargs.pidfile = NULL;
	}
}

static void shutdown_cleanup(void)
{
	block_signals();

	force_disconnect_clients();

	xevrep_disable(&backend.xevrep);
	xevrep_sigchld(&backend.xevrep, 1);

	unblock_x11_input(&backend.x11lock);
	x11lock_sigchld(&backend.x11lock, 1);

	autodim_destroy(backend.autodim);
	autodim_free(backend.autodim);
	backend.autodim = NULL;

	devicelock_destroy(backend.devicelock);
	backend.devicelock = NULL;
	backlight_destroy(backend.backlight);
	backend.backlight = NULL;
	battery_destroy(backend.battery);
	backend.battery = NULL;

	remove_pidfile();
	remove_socket();

	config_file_free(backend.config);
	backend.config = NULL;

	unblock_signals();
}

static inline void enter_signal(void)
{
	in_signal = 1;
	compiler_barrier();
}

static inline void leave_signal(void)
{
	compiler_barrier();
	in_signal = 0;
}

static void signal_terminate(int signal)
{
	enter_signal();

	loginfo("Terminating signal received.\n");
	shutdown_cleanup();

	leave_signal();
	exit(1);
}

static void signal_pipe(int signal)
{
	enter_signal();

	logerr("Broken pipe.\n");

	leave_signal();
}

static void signal_async_io(int signal)
{
	enter_signal();

	socket_accept(socket_fd);
	recv_clients();

	leave_signal();
}

static void signal_input_event_1(int signal)
{
	enter_signal();

	if (backend.autodim)
		autodim_handle_input_event(backend.autodim);

	leave_signal();
}

static void signal_input_event_2(int signal)
{
	enter_signal();

	backend.devicelock->event(backend.devicelock);

	leave_signal();
}

static void signal_child(int signal)
{
	enter_signal();

	x11lock_sigchld(&backend.x11lock, 0);
	xevrep_sigchld(&backend.xevrep, 0);

	leave_signal();
}

#define sigset_set_blocked_sigs(setp) do {	\
		sigemptyset((setp));		\
		sigaddset((setp), SIGCHLD);	\
		sigaddset((setp), SIGUSR1);	\
		sigaddset((setp), SIGUSR2);	\
		sigaddset((setp), SIGIO);	\
		sigaddset((setp), SIGPIPE);	\
		sigaddset((setp), SIGINT);	\
		sigaddset((setp), SIGTERM);	\
	} while (0)

void block_signals(void)
{
	sigset_t set;

	if (!in_signal) {
		assert(sig_block_count >= 0);
		if (sig_block_count++ == 0) {
			sigset_set_blocked_sigs(&set);
			if (sigprocmask(SIG_BLOCK, &set, NULL)) {
				logerr("Failed to block signals: %s\n",
				       strerror(errno));
			}
		}
	}
	compiler_barrier();
}

void unblock_signals(void)
{
	sigset_t set;

	compiler_barrier();
	if (!in_signal) {
		if (--sig_block_count == 0) {
			sigset_set_blocked_sigs(&set);
			if (sigprocmask(SIG_UNBLOCK, &set, NULL)) {
				logerr("Failed to unblock signals: %s\n",
				       strerror(errno));
			}
		}
		assert(sig_block_count >= 0);
	}
}

static int install_sighandler(int signal, void (*handler)(int))
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	sigset_set_blocked_sigs(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = handler;

	return sigaction(signal, &act, NULL);
}

static int setup_signal_handlers(void)
{
	int err = 0;

	err |= install_sighandler(SIGINT, signal_terminate);
	err |= install_sighandler(SIGTERM, signal_terminate);
	err |= install_sighandler(SIGPIPE, signal_pipe);
	err |= install_sighandler(SIGIO, signal_async_io);
	err |= install_sighandler(SIGUSR1, signal_input_event_1);
	err |= install_sighandler(SIGUSR2, signal_input_event_2);
	err |= install_sighandler(SIGCHLD, signal_child);

	return err ? -1 : 0;
}

static int set_niceness(void)
{
	int err, nice_level;

	nice_level = config_get_int(backend.config, "SYSTEM", "nice", 5);
	nice_level = clamp(nice_level, -20, 19);
	err = setpriority(PRIO_PROCESS, 0, nice_level);
	if (err) {
		logerr("Failed to set niceness to %d: %s\n",
		       nice_level, strerror(errno));
		return -errno;
	}

	return 0;
}

int mainloop(void)
{
	int err, value;
	unsigned int timer_errors = 0;

	log_initialize();

	err = -ENOMEM;
	backend.config = config_file_parse("/etc/pwrtray-backendrc");
	if (!backend.config)
		goto error;
	err = sleeptimer_system_init();
	if (err)
		goto error;
	err = -ENOMEM;
	backend.battery = battery_probe();
	if (!backend.battery)
		goto error;
	backend.backlight = backlight_probe();
	if (!backend.backlight)
		goto error;
	if (config_get_bool(backend.config, "BACKLIGHT", "autodim_default_on", 0)) {
		value = config_get_int(backend.config, "BACKLIGHT",
				       "autodim_default_max", 100);
		if (enable_autodim(value))
			logerr("Failed to initially enable autodimming\n");
	}
	backend.devicelock = devicelock_probe();
	if (!backend.devicelock)
		goto error;
	err = create_socket();
	if (err)
		goto error;
	err = create_pidfile();
	if (err)
		goto error;
	err = setup_signal_handlers();
	if (err)
		goto error;
	err = set_niceness();
	if (err)
		goto error;

	loginfo("pwrtray-backend started\n");

	while (1) {
		err = sleeptimer_wait_next();
		if (!err)
			continue;
		if (timer_errors < 10) {
			timer_errors++;
			logdebug("Mainloop: sleeptimer_wait_next() failed with %d (%s)\n",
				 err, strerror(-err));
		}
		msleep(1000);
	}

error:
	shutdown_cleanup();
	log_exit();

	return err ? 1 : 0;
}

int main(int argc, char **argv)
{
	int err;
	pid_t pid;

	err = parse_commandline(argc, argv);
	if (err)
		return (err < 0) ? 1 : 0;

	if (cmdargs.background) {
		pid = fork();
		if (pid == 0) /* child */
			return mainloop();
		if (pid > 0) {
			logdebug("Forked background process pid=%lu\n",
				 (unsigned long)pid);
			return 0;
		}
		logerr("Failed to fork into background: %s\n",
		       strerror(errno));
		return 1;
	}

	return mainloop();
}
