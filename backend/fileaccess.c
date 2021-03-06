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

#include "fileaccess.h"
#include "log.h"
#include "util.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>


#define PROCFS_BASE	"/proc"
#define SYSFS_BASE	"/sys"


static void file_rewind(struct fileaccess *fa)
{
	rewind(fa->stream);
	lseek(fa->fd, 0, SEEK_SET);
}

void file_close(struct fileaccess *fa)
{
	if (fa) {
		fclose(fa->stream);
		close(fa->fd);
		free(fa);
	}
}

struct fileaccess * file_open(int flags, const char *path_fmt, ...)
{
	char path[PATH_MAX + 1];
	va_list ap;
	int fd;
	FILE *stream;
	struct fileaccess *fa;
	const char *opentype;

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	fd = open(path, flags);
	if (fd < 0)
		goto error;
	if (flags == O_RDONLY)
		opentype = "r";
	else if (flags == O_WRONLY)
		opentype = "w";
	else if (flags == O_RDWR)
		opentype = "w+";
	else
		goto err_close;
	stream = fdopen(fd, opentype);
	if (!stream)
		goto err_close;

	fa = zalloc(sizeof(*fa));
	if (!fa)
		goto err_fclose;

	fa->fd = fd;
	fa->stream = stream;

	return fa;

err_fclose:
	fclose(stream);
err_close:
	close(fd);
error:
	return NULL;
}

struct fileaccess * sysfs_file_open(int flags, const char *path_fmt, ...)
{
	char path[PATH_MAX + 1];
	va_list ap;

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	return file_open(flags, "%s/%s", SYSFS_BASE, path);
}

struct fileaccess * procfs_file_open(int flags, const char *path_fmt, ...)
{
	char path[PATH_MAX + 1];
	va_list ap;

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	return file_open(flags, "%s/%s", PROCFS_BASE, path);
}

int file_read_buf(struct fileaccess *fa, char *buf, size_t size)
{
	ssize_t count;
	size_t pos = 0;

	if (!size)
		return 0;

	file_rewind(fa);
	while (size) {
		count = read(fa->fd, buf + pos, size);
		if (count < 0)
			return -errno;
		if (!count)
			break;
		size -= count;
		pos += count;
	}

	return pos;
}

int file_read_string(struct fileaccess *fa, char *buf, size_t size)
{
	int count;

	if (!size)
		return 0;
	count = file_read_buf(fa, buf, size - 1);
	if (count < 0)
		return count;
	buf[count] = '\0';

	return count;
}

int file_read_int(struct fileaccess *fa, int *value, int base)
{
	char buf[64];
	int count;
	long val;
	char *tail;

	count = file_read_buf(fa, buf, sizeof(buf) - 1);
	if (count < 0)
		return count;
	buf[count] = '\0';

	errno = 0;
	val = strtol(buf, &tail, base);
	if (errno || (*tail != '\0' && *tail != '\n'))
		return -ETXTBSY;
	*value = val;

	return 0;
}

int file_write_int(struct fileaccess *fa, int value, int base)
{
	char buf[64];
	const char *buf_ptr;
	const char *fmt;
	size_t count;
	ssize_t res;

	if (base == 0 || base == 10)
		fmt = "%d";
	else if (base == 16)
		fmt = "0x%X";
	else
		return -EINVAL;

	snprintf(buf, sizeof(buf), fmt, value);

	file_rewind(fa);
	count = strlen(buf);
	buf_ptr = buf;
	while (count) {
		res = write(fa->fd, buf_ptr, count);
		if (res < 0)
			return -ETXTBSY;
		count -= (size_t)res;
		buf_ptr += (size_t)res;
	}

	return 0;
}

int file_read_bool(struct fileaccess *fa, int *value)
{
	int val, err;

	err = file_read_int(fa, &val, 0);
	if (err)
		return err;
	if (val < 0)
		return -ETXTBSY;
	*value = !!val;

	return 0;
}

int file_write_bool(struct fileaccess *fa, int value)
{
	return file_write_int(fa, !!value, 0);
}

int file_read_text_lines(struct fileaccess *fa, struct list_head *lines_list,
			 int strip_whitespace)
{
	char *lineptr = NULL, *str;
	size_t size = 0;
	ssize_t count;
	struct text_line *tl;
	int err;

	INIT_LIST_HEAD(lines_list);
	file_rewind(fa);
	while (1) {
		count = getline(&lineptr, &size, fa->stream);
		if (count <= 0)
			break;
		while (count > 0 &&
			(lineptr[count - 1] == '\r' ||
				lineptr[count - 1] == '\n')) {
			lineptr[count - 1] = '\0';
			count--;
		}

		tl = malloc(sizeof(*tl));
		if (!tl) {
			err = -ENOMEM;
			goto error_unwind;
		}
		if (strip_whitespace)
			str = string_strip(lineptr);
		else
			str = lineptr;
		tl->text = strdup(str);
		if (!tl->text) {
			err = -ENOMEM;
			free(tl);
			goto error_unwind;
		}
		list_add_tail(&tl->list, lines_list);
	}
	free(lineptr);

	return 0;

error_unwind:
	text_lines_free(lines_list);
	free(lineptr);

	return err;
}

void text_line_free(struct text_line *tl)
{
	if (tl) {
		list_del(&tl->list);
		free(tl->text);
		free(tl);
	}
}

void text_lines_free(struct list_head *lines_list)
{
	struct text_line *tl, *tl_tmp;

	if (lines_list) {
		list_for_each_entry_safe(tl, tl_tmp, lines_list, list)
			text_line_free(tl);
		INIT_LIST_HEAD(lines_list);
	}
}

int list_directory(struct list_head *dir_entries, const char *path_fmt, ...)
{
	char path[PATH_MAX + 1];
	va_list ap;
	DIR *dir;
	struct dirent *dirent;
	int err;
	struct dir_entry *de;
	int count = 0;

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	dir = opendir(path);
	if (!dir)
		return -errno;
	INIT_LIST_HEAD(dir_entries);
	while (1) {
		errno = 0;
		dirent = readdir(dir);
		if (!dirent) {
			err = errno;
			if (err)
				goto error_unwind;
			break;
		}

		if (strcmp(dirent->d_name, ".") == 0 ||
		    strcmp(dirent->d_name, "..") == 0)
			continue;

		de = malloc(sizeof(*de));
		if (!de) {
			err = -ENOMEM;
			goto error_unwind;
		}
		de->name = strdup(dirent->d_name);
		if (!de->name) {
			err = -ENOMEM;
			free(de);
			goto error_unwind;
		}
		de->type = dirent->d_type;
		list_add_tail(&de->list, dir_entries);
		count++;
	}
	closedir(dir);

	return count;

error_unwind:
	dir_entries_free(dir_entries);
	closedir(dir);

	return err;
}

int list_sysfs_directory(struct list_head *dir_entries, const char *path_fmt, ...)
{
	char path[PATH_MAX + 1];
	va_list ap;

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	return list_directory(dir_entries, "%s/%s", SYSFS_BASE, path);
}

int list_procfs_directory(struct list_head *dir_entries, const char *path_fmt, ...)
{
	char path[PATH_MAX + 1];
	va_list ap;

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	return list_directory(dir_entries, "%s/%s", PROCFS_BASE, path);
}

void dir_entry_free(struct dir_entry *dir_entry)
{
	if (dir_entry) {
		list_del(&dir_entry->list);
		free(dir_entry->name);
		free(dir_entry);
	}
}

void dir_entries_free(struct list_head *dir_entries)
{
	struct dir_entry *d, *d_tmp;

	if (dir_entries) {
		list_for_each_entry_safe(d, d_tmp, dir_entries, list)
			dir_entry_free(d);
		INIT_LIST_HEAD(dir_entries);
	}
}
