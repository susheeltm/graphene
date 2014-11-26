MAKEFLAGS += --check-symlink-times

CC	= gcc49
LD	= ld

CFLAGS	= -Wall -fPIC -O2 -std=gnu99 -fgnu89-inline -Wall -U_FORTIFY_SOURCE \
	  -fno-stack-protector -fno-builtin \
	  -DPALDIR="$(shell readlink -f ../..)/" -DIN_PAL \
	  -I. -I../.. -I../../../include -I../../host/FreeBSD \
	  -I../../../lib -I../../../linux-3.5/graphene -I../../../include/includeBSD
LDFLAGS	= -shared -nostdlib -z combreloc -z defs --version-script pal-sec.map -e start

objs	= bpf_helper filter main manifest wrapper slab printf
graphene_lib = ../../../lib/graphene-lib.a

all: libpal_sec.so

debug: debug = debug
debug: CC = gcc49 -g -gdwarf-2
debug: CFLAGS += -DDEBUG
debug: libpal_sec.so

$(graphene_lib):
	make -C ../../../lib $(debug)

libpal_sec.so: $(addsuffix .o,$(objs)) $(graphene_lib)
	@echo [ security/FreeBSD/$@ ]
	@[ ! -f $@ ] || mv -f $@ $@.backup
	@$(LD) $(LDFLAGS) -o $@ $^ -soname=libpal_sec.so
	@cd ../.. && ln -sf security/FreeBSD/libpal_sec.so

%.o: %.c
	@echo [ security/FreeBSD/$@ ]
	@$(CC) $(CFLAGS) -c $< -o $@

%.e: %.c
	@echo [ security/FreeBSD/$@ ]
	@$(CC) $(CFLAGS) -E $< -o $@

clean:
	rm -rf *.o
	[ ! -f libpal_sec.so ] || mv -f libpal_sec.so libpal_sec.so.backup
