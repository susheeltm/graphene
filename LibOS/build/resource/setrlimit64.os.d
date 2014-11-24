$(common-objpfx)resource/setrlimit64.os: \
 ../sysdeps/unix/sysv/linux/wordsize-64/setrlimit64.c \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
