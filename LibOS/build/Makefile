srcdir = /home/cse506/FreeBSD/BSD/graphene/LibOS/glibc-2.17

# Uncomment the line below if you want to do parallel build.
# PARALLELMFLAGS = -j 4

.PHONY: all install

all .DEFAULT:
	$(MAKE) -r PARALLELMFLAGS="$(PARALLELMFLAGS)" -C $(srcdir) objdir=`pwd` $@

install:
	LANGUAGE=C LC_ALL=C; export LANGUAGE LC_ALL; \
	$(MAKE) -r PARALLELMFLAGS="$(PARALLELMFLAGS)" -C $(srcdir) objdir=`pwd` $@
