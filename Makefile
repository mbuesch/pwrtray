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
	$(foreach TARGET,$(ALL_TARGETS),$(MAKE) -C $(TARGET) clean && ) true

install: $(ALL_TARGETS)
	$(foreach TARGET,$(ALL_TARGETS),$(MAKE) -C $(TARGET) install && ) true

.PHONY: all backend tray xlock clean install
