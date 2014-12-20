$(common-objpfx)string/rtld-strnlen-sse2-no-bsf.os: \
 ../sysdeps/x86_64/multiarch/strnlen-sse2-no-bsf.S \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 ../sysdeps/x86_64/multiarch/strlen-sse2-no-bsf.S

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../sysdeps/x86_64/multiarch/strlen-sse2-no-bsf.S:
