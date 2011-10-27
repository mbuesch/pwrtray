CXX		?= g++
MOC		?= moc-qt4
STRIP		?= strip
INSTALL		?= install

QT_INCDIR	?= /usr/include
QT_LIBDIR	?= /usr/lib

CXXFLAGS	+= -Os -Wall -fno-exceptions \
		   -I$(QT_INCDIR) \
		   -I$(QT_INCDIR)/QtCore \
		   -I$(QT_INCDIR)/QtGui \
		   -I$(QT_INCDIR)/qt4 \
		   -I$(QT_INCDIR)/qt4/QtCore \
		   -I$(QT_INCDIR)/qt4/QtGui
CXXFLAGS	+= -DNULL=0 -DPREFIX=$(PREFIX)
LDFLAGS		?= -L$(QT_LIBDIR) \
		  -L$(QT_LIBDIR)/qt4
LIBS		?= -lQtCore -lQtGui
DESTDIR		?=
PREFIX		?= /usr/local

BIN		= pwrtray
SRCS		= main.cpp backend.cpp

V		= @             # Verbose build:  make V=1
Q		= $(V:1=)
QUIET_CXX	= $(Q:@=@echo '     CXX      '$@;)$(CXX)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CXX)
QUIET_MOC	= $(Q:@=@echo '     MOC      '$@;)$(MOC)
QUIET_STRIP	= $(Q:@=@echo '     STRIP    '$@;)$(STRIP)

DEPS		= $(patsubst %.cpp,dep/%.d,$(1))
OBJS		= $(patsubst %.cpp,obj/%.o,$(1))

.SUFFIXES:
.PHONY: all install clean
.DEFAULT_GOAL := all

# Generate dependencies
$(call DEPS,$(SRCS)): dep/%.d: %.cpp
	@mkdir -p $(dir $@)
	$(QUIET_DEPEND) -o $@.tmp -MM -MG -MT "$@ $(patsubst dep/%.d,obj/%.o,$@)" $(CXXFLAGS) $< && mv -f $@.tmp $@

-include $(call DEPS,$(SRCS))

# Generate object files
$(call OBJS,$(SRCS)): obj/%.o:
	@mkdir -p $(dir $@)
	$(QUIET_CXX) -o $@ -c $(CXXFLAGS) $<

moc/%.moc: %.h
	@mkdir -p $(dir $@)
	$(QUIET_MOC) -o $@ $<

all: $(BIN)

$(BIN): $(call OBJS,$(SRCS))
	$(QUIET_CXX) $(CXXFLAGS) -o $(BIN) $(LDFLAGS) $(LIBS) $(call OBJS,$(SRCS))
	$(QUIET_STRIP) $(BIN)

clean:
	rm -Rf dep obj moc core *~ $(BIN)

install: $(BIN)
	$(INSTALL) -d -m755 $(DESTDIR)$(PREFIX)/share/pwrtray/
	$(INSTALL) -d -m755 $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m644 ./bulb.png $(DESTDIR)$(PREFIX)/share/pwrtray/
	$(INSTALL) -m755 $(BIN) $(DESTDIR)$(PREFIX)/bin/
