SHELL=/bin/bash
CROSS_COMPILE ?=

VERSION=$(shell git describe --tags)

include cross_compile.mk

CFLAGS := -g -O2 -Wall -DSA_VERSION=$(VERSION)
LDFLAGS :=

SCHEDQOS := schedqos

SRC := schedqos.c parse_argp.c
OBJS :=$(subst .c,.o,$(SRC))

ifneq ($(STATIC),)
	LDFLAGS := $(LDFLAGS) $(shell [ $$(find /usr/lib -name libzstd.a | grep .) ] && echo -lzstd)
	LDFLAGS := $(LDFLAGS) -static
endif

ifneq ($(DEBUG),)
	CFLAGS := $(CFLAGS) -DDEBUG
endif

all: $(SCHEDQOS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

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
	rm -rf $(SCHEDQOS) *.o

clobber: clean
	$(MAKE) -C clean

help:
	@echo "Following build targets are available:"
	@echo ""
	@echo "	static:		Create statically linked binary"
	@echo "	debug:		Create a debug build which contains verbose debug prints"
	@echo "	clean:		Clean sched-analyzer, but not dependent libraries"
	@echo "	clobber:	Clean everything"
	@echo ""
	@echo "Cross compile:"
	@echo ""
	@echo "	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-"
	@echo ""
	@echo "	You can only specifiy the ARCH and we will try to guess the correct gcc CROSS_COMPILE to use"
