$(common-objpfx)dirent/readdir64_r.o: \
 ../sysdeps/unix/sysv/linux/wordsize-64/readdir64_r.c \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
