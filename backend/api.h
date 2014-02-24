#ifndef PWRTRAY_API_H_
#define PWRTRAY_API_H_

#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PT_SOCK_DIR	"/var/run/pwrtray"
#define PT_SOCKET	PT_SOCK_DIR "/socket"

//#define PT_PACKED	__attribute__((__packed__))
#define PT_PACKED

enum {
	PTREQ_PING			= 0x0,
	PTREQ_WANT_NOTIFY,
	PTREQ_XEVREP,

	/* Backlight controls */
	PTREQ_BL_GETSTATE		= 0x100,
	PTREQ_BL_SETBRIGHTNESS,
	PTREQ_BL_AUTODIM,

	/* Battery controls */
	PTREQ_BAT_GETSTATE		= 0x200,

	/* Notifications */
	PTNOTI_SRVDOWN			= 0xF00,
	PTNOTI_BL_CHANGED,
	PTNOTI_BAT_CHANGED,
};

/* (struct pt_message *)->bat_stat.flags */
#define PT_BAT_FLG_ONAC		(1 << 0) /* On AC */
#define PT_BAT_FLG_ACUNKNOWN	(1 << 1) /* AC status unknown */
#define PT_BAT_FLG_CHARGING	(1 << 2) /* Currently charging */
#define PT_BAT_FLG_CHUNKNOWN	(1 << 3) /* Charging status unknown */

/* (struct pt_message *)->bl_stat.flags */
#define PT_BL_FLG_AUTODIM	(1 << 0) /* Auto dimming enabled */

/* (struct pt_message *)->bl_autodim.flags */
#define PT_AUTODIM_FLG_ENABLE	(1 << 0) /* Auto dimming enable */

/* (struct pt_message *)->flags */
#define PT_FLG_REPLY		(1 << 0) /* This is a reply to a previous message */
#define PT_FLG_OK		(1 << 1) /* There was no error */
#define PT_FLG_ENABLE		(1 << 2) /* Generic boolean flag */

struct pt_message {
	uint16_t id;
	uint16_t flags;
	union {
		struct { /* Backlight / LCD state */
			uint32_t flags;
			int32_t min_brightness;
			int32_t max_brightness;
			int32_t brightness_step;
			int32_t brightness;
			int32_t default_autodim_max_percent;
		} PT_PACKED bl_stat;
		struct { /* Set backlight */
			int32_t brightness;
		} PT_PACKED bl_set;
		struct { /* Autodim controls */
			uint32_t flags;
			int32_t max_percent;
		} PT_PACKED bl_autodim;
		struct { /* Battery state */
			uint32_t flags;
			int32_t min_level;
			int32_t max_level;
			int32_t level;
		} PT_PACKED bat_stat;
		struct { /* Error code (only for PT_FLG_REPLY) */
			int32_t code;
		} PT_PACKED error;
	} PT_PACKED;
} PT_PACKED;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PWRTRAY_API_H_ */
