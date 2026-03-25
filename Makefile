SHELL=/bin/bash
CROSS_COMPILE ?=

VERSION=$(shell git describe --tags)

include cross_compile.mk

CFLAGS := -g -O2 -Wall -DSA_VERSION=$(VERSION) -D_GNU_SOURCE
LDFLAGS := -lglib-2.0
INCLUDES := -include logging.h $(shell pkg-config --cflags --libs glib-2.0)

SCHEDQOS := schedqos

CJSON_SRC := cJSON.c
CJSON_HDR := cJSON.h

PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
CONFIGSDIR := /var/run/schedqos

INSTALL ?= install
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

SRC := schedqos.c parse_argp.c configs_parser.c $(CJSON_SRC) \
       netlink_monitor.c qos_manager.c utils.c qos_tagging.c \
       sched_profiles.c
OBJS :=$(subst .c,.o,$(SRC))

ifneq ($(STATIC),)
	LDFLAGS := $(LDFLAGS) -static
endif

ifneq ($(DEBUG),)
	CFLAGS := $(CFLAGS) -DDEBUG
endif

all: $(CJSON_SRC) $(SCHEDQOS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(CJSON_SRC):
	git submodule init
	git submodule update
	cp cJSON/cJSON.c .
	cp cJSON/cJSON.h .

$(OBJS): $(shell ls *.h)

$(SCHEDQOS): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(filter %.o,$^) $(LDFLAGS) -o $@

package: $(SCHEDQOS)
	tar cfz $(SCHEDQOS)-$(ARCH)-$(VERSION)$(shell [ "$(STATIC)x" != "x" ] && echo "-static").tar.gz $(SCHEDQOS)

release:
	[ "$(shell ls | grep $(SCHEDQOS).*.tar.gz)x" == "x" ] || (echo "Release file found, clean then try again" && exit 1)
	$(MAKE) clobber
	$(MAKE) ARCH=x86 package
	$(MAKE) clean
	$(MAKE) ARCH=x86 STATIC=1 package
	$(MAKE) clobber
	$(MAKE) ARCH=arm64 package
	$(MAKE) clean
	$(MAKE) ARCH=arm64 STATIC=1 package
	$(MAKE) package-pp

static:
	$(MAKE) STATIC=1

debug:
	$(MAKE) DEBUG=1

clean:
	rm -rf $(SCHEDQOS) *.o $(CJSON_SRC) $(CJSON_HDR)

clobber: clean
	$(MAKE) -C clean

install: $(SCHEDQOS)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) $(SCHEDQOS) $(DESTDIR)$(BINDIR)/$(SCHEDQOS)
	$(INSTALL) -d $(CONFIGSDIR)/app_configs
	for config in $(shell ls ./configs/*.json);			\
	do								\
		$(INSTALL_DATA) $$config $(CONFIGSDIR);			\
	done
	for config in $(shell ls ./configs/app_configs/*.json);		\
	do								\
		$(INSTALL_DATA) $$config $(CONFIGSDIR)/app_configs;	\
	done

uninstall:
	rm -rf $(DESTDIR)$(BINDIR)/$(SCHEDQOS)
	rm -rf $(CONFIGSDIR)

help:
	@echo "Following build targets are available:"
	@echo ""
	@echo "	static:		Create statically linked binary"
	@echo "	debug:		Create a debug build which contains verbose debug prints"
	@echo "	clean:		Clean sched-analyzer, but not dependent libraries"
	@echo "	clobber:	Clean everything"
	@echo "	install:	Install the binary and the configs
	@echo "	uninstall:	Uninstall the binary and the configs
	@echo ""
	@echo "Cross compile:"
	@echo ""
	@echo "	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-"
	@echo ""
	@echo "	You can only specifiy the ARCH and we will try to guess the correct gcc CROSS_COMPILE to use"
