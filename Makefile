# Enable xlock build?
export FEATURE_XLOCK	?= y


ALL_TARGETS	:= backend tray $(if $(filter 1 y,$(FEATURE_XLOCK)),xlock)

all: $(ALL_TARGETS)

backend:
	$(MAKE) -C backend all

tray:
	$(MAKE) -C tray all

xlock:
	$(MAKE) -C xlock all

clean:
	for target in $(ALL_TARGETS); do $(MAKE) -C $$target clean; done

install: $(ALL_TARGETS)
	for target in $(ALL_TARGETS); do $(MAKE) -C $$target install; done

.PHONY: all backend tray xlock clean install
