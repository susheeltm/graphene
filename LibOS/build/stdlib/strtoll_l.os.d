$(common-objpfx)stdlib/strtoll_l.os: \
 ../sysdeps/wordsize-64/strtoll_l.c ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
