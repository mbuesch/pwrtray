include ../make.inc

CFLAGS		+= $(BASE_CFLAGS) $(WARN_CFLAGS)
LDFLAGS		?=
LIBS		?= -lrt -lm

BIN		:= pwrtray-backend

# Battery modules. battery_dummy.c must be last.
BAT_MODULES	:=		\
	battery_powerbook.c	\
	battery_n810.c		\
	battery_class.c		\
	battery_acpi.c		\
	battery_dummy.c

# Backlight modules. backlight_dummy.c must be last.
BL_MODULES	:=		\
	backlight_omapfb.c	\
	backlight_class.c	\
	backlight_dummy.c

# Device lock modules. devicelock_dummy.c must be last.
DLOCK_MODULES	:=		\
	devicelock_n810.c	\
	devicelock_dummy.c

SRCS		:= main.c timer.c log.c args.c conf.c util.c fileaccess.c \
		  autodim.c x11lock.c xevrep.c probe.c \
		  battery.c $(BAT_MODULES) \
		  backlight.c $(BL_MODULES) \
		  devicelock.c $(DLOCK_MODULES)

V		= @             # Verbose build:  make V=1
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CC)

DEPS		= $(patsubst %.c,dep/%.d,$(1))
OBJS		= $(patsubst %.c,obj/%.o,$(1))

.SUFFIXES:
.PHONY: all install clean
.DEFAULT_GOAL := all

# Generate dependencies
$(call DEPS,$(SRCS)): dep/%.d: %.c
	@mkdir -p $(dir $@)
	$(QUIET_DEPEND) -o $@.tmp -MM -MT "$@ $(patsubst dep/%.d,obj/%.o,$@)" $(CFLAGS) $< && mv -f $@.tmp $@

-include $(call DEPS,$(SRCS))

# Generate object files
$(call OBJS,$(SRCS)): obj/%.o:
	@mkdir -p $(dir $@)
	$(QUIET_CC) -o $@ -c $(CFLAGS) $<

all: $(BIN)

$(BIN): $(call OBJS,$(SRCS))
	$(QUIET_CC) $(CFLAGS) -o $(BIN) $(LDFLAGS) $(LIBS) $(call OBJS,$(SRCS))

clean:
	rm -Rf dep obj core *~ $(BIN)

install: $(BIN)
	$(INSTALL) -d -m755 $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m755 $(BIN) $(DESTDIR)$(PREFIX)/bin/
