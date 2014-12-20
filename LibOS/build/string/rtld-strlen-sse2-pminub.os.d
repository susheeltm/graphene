$(common-objpfx)string/rtld-strlen-sse2-pminub.os: \
 ../sysdeps/x86_64/multiarch/strlen-sse2-pminub.S \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
