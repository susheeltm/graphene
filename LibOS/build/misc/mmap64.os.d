$(common-objpfx)misc/mmap64.os: \
 ../sysdeps/unix/sysv/linux/wordsize-64/mmap64.c \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
