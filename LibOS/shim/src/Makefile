MAKEFLAGS += --check-symlink-times

CC	= gcc
AS	= gcc
AR	= ar rcs
LD	= ld

OMIT_FRAME_POINTER = no

CFLAGS	= -Wall -fPIC -std=gnu99 -fgnu89-inline -Winline -Wwrite-strings \
	  -fmerge-all-constants -Wstrict-prototypes \
	  -Werror=implicit-function-declaration \
	  -fno-stack-protector -fno-builtin -Wno-inline \
	  -I../include -I../../../Pal/lib -I../../../Pal/include/pal
ifeq ($(OMIT_FRAME_POINTER),yes)
CFLAGS += -DOMIT_FRAME_POINTER=1
else
CFLAGS += -fno-omit-frame-pointer -DOMIT_FRAME_POINTER=0
endif
ASFLAGS	= -Wa,--noexecstack -x assembler-with-cpp -I../include

LDFLAGS	= -shared -nostdlib --version-script shim.map -T shim.lds \
	  -z combreloc -z relro -z defs \
	  -dynamic-link=libpal.so -rpath-link=$(PWD)
LDFLAGS-debug = $(patsubst shim.map,shim-debug.map,$(LDFLAGS))
ARFLAGS	=

shim_target = libsysdb.a libsysdb.so libsysdb_debug.so

defs	= -DIN_SHIM
fs	= chroot str pipe socket proc dev
ipcns	= pid sysv
objs	= $(addprefix bookkeep/shim_,handle vma thread signal) \
	  $(patsubst %.c,%,$(wildcard utils/*.c)) \
	  $(addprefix fs/shim_,dcache namei fs_hash fs) \
	  $(patsubst %.c,%,$(foreach f,$(fs),$(wildcard fs/$(f)/*.c))) \
	  $(addprefix ipc/shim_,ipc ipc_helper ipc_child) \
	  $(addprefix ipc/shim_ipc_,$(ipcns)) \
	  elf/shim_rtld \
	  $(addprefix shim_,init table syscalls checkpoint random malloc \
	  async parser debug) syscallas start \
	  $(patsubst %.c,%,$(wildcard sys/*.c))
graphene_lib = ../../../Pal/lib/graphene-lib.a
pal_lib = ../../../Pal/src/libpal.so
headers = ../include/*.h ../../../Pal/lib/*.h ../../../Pal/include/pal/*.h

all: $(shim_target)

debug: debug = debug
debug: CC = gcc -gdwarf-2 -g3
debug: CFLAGS += -DDEBUG
debug: $(shim_target)

profile: CC = gcc
profile: CFLAGS += -DPROFILE
profile: $(shim_target)

$(graphene_lib):
	make -C ../../../Pal/lib $(debug)

libsysdb.so: $(addsuffix .o,$(objs)) $(filter %.map %.lds,$(LDFLAGS)) \
	     $(graphene_lib) $(pal_lib)
	@echo [ $@ ]
	$(LD) $(LDFLAGS) -o $@ $(filter-out %.map %.lds,$^) -soname $@ \
		-e shim_start

libsysdb_debug.so: $(addsuffix .o,$(objs)) \
		   $(filter %.map %.lds,$($LDFLAGS-debug)) \
		   $(graphene_lib) $(pal_lib)
	@echo [ $@ ]
	$(LD) $(LDFLAGS-debug) -o $@ $(filter-out %.map %.lds,$^) -soname $@ \
		-e shim_start

libsysdb.a: $(addsuffix .o,$(objs))
	@echo [ $@ ]
	@mv -f $@ $@.backup || true
	@$(AR) $(ARFLAGS) $@ $^

%.asm: %.c $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) $(defs) -c $< -o $<.o
	@objdump -S $<.o > $@
	@rm $<.o

$(addsuffix .o,$(addprefix ipc/shim_ipc_,$(ipcns))): ipc/*.h

%.o: %.c $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) $(defs) -c $< -o $@

%.e: %.c $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) $(defs) -E $< -o $@

%.o: %.S $(headers)
	@echo [ $@ ]
	@$(AS) $(ASFLAGS) $(defs) -c $< -o $@

%.e: %.S $(headers)
	@echo [ $@ ]
	@$(AS) $(ASFLAGS) $(defs) -E $< -o $@

clean:
	rm -f $(addsuffix .o,$(objs)) $(shim_target)
