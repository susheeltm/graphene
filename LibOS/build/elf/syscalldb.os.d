$(common-objpfx)elf/syscalldb.os: \
 syscalldb.c ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 ../sysdeps/unix/sysv/linux/x86_64/syscalldb.h \
 /usr/lib/gcc/x86_64-linux-gnu/4.6/include/stdarg.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../sysdeps/unix/sysv/linux/x86_64/syscalldb.h:

/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stdarg.h:
