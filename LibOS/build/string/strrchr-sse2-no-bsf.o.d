$(common-objpfx)string/strrchr-sse2-no-bsf.o: \
 ../sysdeps/x86_64/multiarch/strrchr-sse2-no-bsf.S \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
