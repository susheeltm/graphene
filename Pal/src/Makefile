MAKEFLAGS += --check-symlink-times

include Makefile.Host

CC	= gcc
AR	= ar rcs
LD	= ld

CFLAGS	= -Wall -fPIC -O2 -std=gnu99 -fgnu89-inline -U_FORTIFY_SOURCE \
	  -fno-omit-frame-pointer \
	  -fno-stack-protector -fno-builtin \
	  -fPIC -pthread\
	  -I. -I../include -Ihost/$(OS) -I../lib -I../include/includeBSD 
ifeq ($(OS),Linux)
LDFLAGS	= -shared -nostdlib --version-script pal.map -z combreloc -z defs \
	  -T pal.lds  
else
LDFLAGS	= -shared -nostdlib --version-script pal.map -z combreloc -z defs \
	  -e pal_start
endif
ARFLAGS	= 

pal_target = libpal.a libpal.so

include host/$(OS)/Makefile.am

defs	= -DIN_PAL
objs	= $(addprefix db_,streams memory threading semaphore events process \
	    object main misc ipc exception rtld) slab printf
graphene_lib = ../lib/graphene-lib.a
host_lib = host/$(OS)/libpal-$(OS).a
headers	= $(wildcard *.h) $(wildcard ../lib/*.h)

ifeq ($(OS),Linux)
all: $(pal_target) security
else
all: $(pal_target) 
endif

debug: debug = debug
debug: CC = gcc -g -gdwarf-2 -gstrict-dwarf
debug: CFLAGS += -DDEBUG
debug: $(pal_target) security

.PHONY: security $(host_lib) $(graphene_lib)

$(graphene_lib):
	make -C ../lib $(debug)

$(host_lib):
	@[ ! -d host/$(OS) ] || make -C host/$(OS) $(debug)
ifeq ($(OS),Linux)
security:
	@[ ! -d security/$(OS) ] || make -C security/$(OS) $(debug)
endif 

libpal.so: $(addsuffix .o,$(objs)) $(filter %.map %.lds,$(LDFLAGS)) \
	   $(host_lib) $(graphene_lib)
	@echo [ ld $(LDFLAGS) -o $@ $(filter-out %.map %.lds,$^) -soname=$@  ]
	@$(LD) $(LDFLAGS) -o $@ $(filter-out %.map %.lds,$^) -soname=$@

libpal.a: $(addsuffix .o,$(objs)) $(host_lib) $(graphene_lib)
	@echo [ $@ ]
	@$(AR) $(ARFLAGS) $@ $^

%.asm: %.c $(headers)
	$(CC) $(CFLAGS) $(defs) -c $< -o $<.o
	@objdump -S $<.o > $@
	@rm $<.o

%.o: %.c $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) $(defs) -c $< -o $@

%.e: %.c $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) $(defs) -E $< -o $@

clean:
	rm -f $(addsuffix .o,$(objs)) $(pal_target)
	@[ ! -d host/$(OS) ] || make -C host/$(OS) clean
	@[ ! -d security/$(OS) ] || make -C security/$(OS) clean
