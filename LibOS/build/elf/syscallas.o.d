$(common-objpfx)elf/syscallas.o: syscallas.S \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 ../sysdeps/unix/sysv/linux/x86_64/syscalldb.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../sysdeps/unix/sysv/linux/x86_64/syscalldb.h:
