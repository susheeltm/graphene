$(common-objpfx)nptl/pthread_spin_trylock.os: \
 ../nptl/sysdeps/x86_64/pthread_spin_trylock.S ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 $(common-objpfx)pthread-errnos.h

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

$(common-objpfx)pthread-errnos.h:
