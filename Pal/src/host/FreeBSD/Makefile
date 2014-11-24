MAKEFLAGS += --check-symlink-times

CC	= gcc49
AS	= gcc49
AR	= ar rcs

CFLAGS	= -Wall -fPIC -O2 -std=gnu99 -fgnu89-inline -Wall -U_FORTIFY_SOURCE \
	  -fno-omit-frame-pointer \
	  -fno-stack-protector -fno-builtin \
	  -I. -I../.. -I../../../include -I../../../lib -I../../../ipc/linux \
	  -I../../../linux-3.5/graphene -I../../../include/includeBSD -fPIC
ASFLAGS = -DPIC -DSHARED -fPIC -DASSEMBLER -Wa,--noexecstack \
	  -x assembler-with-cpp \
	  -I. -I../.. -I../../../include -I../../../include/includeBSD
ARFLAGS	=

host_target = libpal-FreeBSD.a

defs	= -DIN_PAL -DGDB_SCRIPT="$(CURDIR)/pal-gdb.py"
objs	= $(addprefix db_,files devices pipes sockets streams memory threading \
	    semaphore mutex events process object main rtld misc ipc \
	    exception) manifest clone-x86_64 gettimeofday-x86_64
graphene_lib = ../../../lib/graphene-lib.a
headers	= $(wildcard *.h) $(wildcard ../../*.h) $(wildcard ../../../lib/*.h)

all: $(host_target)

debug: debug = debug
debug: CC = gcc49 -g
debug: CFLAGS += -DDEBUG
debug: $(host_target)

$(graphene_lib):
	make -C ../../../lib $(debug)

libpal-FreeBSD.a: $(addsuffix .o,$(objs)) $(graphene_lib)
	@echo [ host/FreeBSD/$@ ]
	@$(AR) $(ARFLAGS) $@ $^

%.asm: %.c $(headers)
	$(CC) $(CFLAGS) $(defs) -c $< -o $<.o
	@objdump -S $<.o > $@
	@rm $<.o

%.o: %.c $(headers)
	@echo [ host/FreeBSD/$@ ]
	@$(CC) $(CFLAGS) $(defs) -c $< -o $@

%.e: %.c $(headers)
	@echo [ host/FreeBSD/$@ ]
	@$(CC) $(CFLAGS) $(defs) -E $< -o $@

%.o: %.S $(headers)
	@echo [ host/FreeBSD/$@ ]
	@$(AS) $(ASFLAGS) $(defs) -c $< -o $@

%.e: %.S $(headers)
	@echo [ host/FreeBSD/$@ ]
	@$(AS) $(ASFLAGS) $(defs) -E $< -o $@

clean:
	rm -f $(addsuffix .o,$(objs)) $(host_target)
