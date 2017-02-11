CC		:= cc
CXX		:= c++
INSTALL		:= install

DESTDIR		:=
PREFIX		:= /usr/local

WARN_CFLAGS	:= -Wall -Wextra -Wswitch-enum \
		   -Wsuggest-attribute=noreturn -Wundef \
		   -Wpointer-arith -Winline -Wlogical-op -Wshadow \
		   -Wno-unused-parameter -Wno-missing-field-initializers \
		   -Wcast-qual

WARN_CXXFLAGS	:= $(WARN_CFLAGS)

BASE_CFLAGS	:= -g -O2 -DPREFIX=$(PREFIX) \
		   -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE \
		   $(if $(filter 1 y,$(FEATURE_XLOCK)),-DFEATURE_XLOCK=1) \
		   $(if $(filter 1 y,$(FEATURE_XEVREP)),-DFEATURE_XEVREP=1) \
		   $(if $(filter 1 y,$(PROFILE)),-pg)

BASE_CXXFLAGS	:= $(BASE_CFLAGS)
