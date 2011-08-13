# Enable xlock build?
export FEATURE_XLOCK	?= y
# Enable xevrep build?
export FEATURE_XEVREP	?= y
# Enable tray frontend build?
export FEATURE_TRAY	?= y


ALL_TARGETS	:= backend \
		   $(if $(filter 1 y,$(FEATURE_TRAY)),tray) \
		   $(if $(filter 1 y,$(FEATURE_XLOCK)),xlock) \
		   $(if $(filter 1 y,$(FEATURE_XEVREP)),xevrep)

MAKE_FLAGS	:= --no-print-directory

all: $(ALL_TARGETS)

backend:
	$(MAKE) $(MAKE_FLAGS) -C backend all

tray:
	$(MAKE) $(MAKE_FLAGS) -C tray all

xlock:
	$(MAKE) $(MAKE_FLAGS) -C xlock all

xevrep:
	$(MAKE) $(MAKE_FLAGS) -C xevrep all

clean:
	for target in $(ALL_TARGETS); do $(MAKE) $(MAKE_FLAGS) -C $$target clean; done

install: $(ALL_TARGETS)
	for target in $(ALL_TARGETS); do $(MAKE) $(MAKE_FLAGS) -C $$target install; done

.PHONY: all backend tray xlock xevrep clean install
