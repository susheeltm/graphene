$(common-objpfx)stdlib/strtoll.os: \
 ../sysdeps/wordsize-64/strtoll.c ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
