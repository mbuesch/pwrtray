CC		?= gcc
STRIP		?= strip
INSTALL		?= install

CFLAGS		?= -Os -Wall
LDFLAGS		?=
LIBS		?= -lrt
DESTDIR		?=
PREFIX		?= /usr

CFLAGS		+= -DPREFIX=$(PREFIX) -D_GNU_SOURCE

BIN		= pwrtray-backend
BATT_SRCS	= battery.c battery_powerbook.c battery_n810.c
BL_SRCS		= backlight.c backlight_class.c backlight_omapfb.c
SL_SRCS		= screenlock.c screenlock_n810.c
SRCS		= main.c timer.c log.c args.c conf.c util.c fileaccess.c autodim.c \
		  $(BATT_SRCS) $(BL_SRCS) $(SL_SRCS)

V		= @             # Verbose build:  make V=1
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CC)
QUIET_STRIP	= $(Q:@=@echo '     STRIP    '$@;)$(STRIP)

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
	$(QUIET_STRIP) $(BIN)

clean:
	rm -Rf dep obj core *~ $(BIN)

install: $(BIN)
	$(INSTALL) -d -m755 $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m755 $(BIN) $(DESTDIR)$(PREFIX)/bin/
