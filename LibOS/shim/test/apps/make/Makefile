manifests = make.manifest make-huge.manifest make-local.manifest

graphene_dirs = LibOS/shim/src LibOS/shim/include Pal/lib Pal/include Pal/src
graphene_target = $(addprefix graphene/,$(graphene_dirs))
graphene_libs = $(addprefix graphene/Pal/src/,libpal.so libpal.a)

all debug: $(manifests) configure $(graphene_target) $(graphene_libs)

test_tarballs = oggenc-1.4 bzip2-1.0.6
test_targets = $(test_tarballs) helloworld graphene

make_src = make-3.82

make-local: $(make_src)/make
	cp $< make-local

$(make_src) $(test_tarballs): %: %.tar.gz
	[ -d $@ ] || tar -zxf $<

$(make_src)/make:
	cd $(make_src) && ./configure
	cd $(make_src) && make CC="gcc -g"

$(graphene_target): graphene/%: ../../../../../%
	cd $< && git checkout-index -a -f --prefix=$(PWD)/graphene/

$(graphene_libs): graphene/%: ../../../../../%
	mkdir -p $(dir $@)
	ln -sf $(shell readlink -f $<) $@

test: all $(test_targets)

$(manifests): %: %.template
	make -C .. make/$@ appdir=make/

configure: $(test_targets)
	for d in $(test_targets); do \
		if [ ! -f $$d/Makefile ] && [ -f $$d/configure ]; then \
			cd $$d; ./configure; cd ..; \
		fi; \
	done

clean: clean-manifests clean-tests

distclean: clean-manifests clean-tests
	rm -rf $(make_src) make result $(test_tarballs) $(graphene_target) $(graphene_libs)

clean-manifests:
	rm -rf $(manifests)

clean-tests:
	for d in $(test_targets); do (make -C $$d clean || true); done
