all: backend tray

backend:
	$(MAKE) -C backend all

tray:
	$(MAKE) -C tray all

clean:
	$(MAKE) -C backend clean
	$(MAKE) -C tray clean

install: backend tray
	$(MAKE) -C backend install
	$(MAKE) -C tray install

.PHONY: all backend tray clean install
