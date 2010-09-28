all: backend tray xlock

backend:
	$(MAKE) -C backend all

tray:
	$(MAKE) -C tray all

xlock:
	$(MAKE) -C xlock all

clean:
	$(MAKE) -C backend clean
	$(MAKE) -C tray clean
	$(MAKE) -C xlock clean

install: backend tray xlock
	$(MAKE) -C backend install
	$(MAKE) -C tray install
	$(MAKE) -C xlock install

.PHONY: all backend tray xlock clean install
