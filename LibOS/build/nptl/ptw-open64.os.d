$(common-objpfx)nptl/ptw-open64.os: \
 ../sysdeps/unix/sysv/linux/wordsize-64/open64.c \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
