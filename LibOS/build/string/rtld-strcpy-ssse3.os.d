$(common-objpfx)string/rtld-strcpy-ssse3.os: \
 ../sysdeps/x86_64/multiarch/strcpy-ssse3.S ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
