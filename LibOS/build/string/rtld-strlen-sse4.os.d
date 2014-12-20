$(common-objpfx)string/rtld-strlen-sse4.os: \
 ../sysdeps/x86_64/multiarch/strlen-sse4.S ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
