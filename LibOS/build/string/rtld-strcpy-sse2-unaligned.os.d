$(common-objpfx)string/rtld-strcpy-sse2-unaligned.os: \
 ../sysdeps/x86_64/multiarch/strcpy-sse2-unaligned.S \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
