$(common-objpfx)stdlib/strtoull_l.os: \
 ../sysdeps/wordsize-64/strtoull_l.c ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h: