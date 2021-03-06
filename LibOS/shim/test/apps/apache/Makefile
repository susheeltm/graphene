HOST := $(firstword $(shell ifconfig | grep 'inet addr:' | grep -v '127.0.0.1' -m 1 | cut -d: -f2))
PORT := 8000

NPROCS := 1
OS := $(shell uname -s)
ifeq ($(OS),Linux)
	NPROCS := $(shell grep -c ^processor /proc/cpuinfo)
endif

HTTPD_DIR = httpd-2.4.3
APR_DIR = apr-1.4.6
APRUTIL_DIR = apr-util-1.5.1
PHP_DIR = php-5.3.16

INSTALL_DIR = $(PWD)/obj
SRC_DIRS = $(HTTPD_DIR) $(APR_DIR) $(APRUTIL_DIR) $(PHP_DIR)
HTDOC=$(INSTALL_DIR)/htdocs

TARGET = $(INSTALL_DIR)/bin/httpd $(INSTALL_DIR)/modules/libphp5.so \
	 $(INSTALL_DIR)/modules/mod_auth_basic_sandbox.so \
	 conf httpd.manifest test-data

all: $(TARGET)

debug: MAKE_FLAGS=CC="gcc -g"
debug: APXS_FLAGS=-S CC="gcc -g"
debug: $(TARGET)

%: %.tar.gz
	tar -xzf $<

%: %.tar.bz2
	tar -xjf $<

$(INSTALL_DIR)/lib/libapr-1.so.0: $(APR_DIR)
	#cd $< && patch -p1 < ../disable-epoll.patch
	cd $< && ./configure --prefix=$(INSTALL_DIR)
	cd $< && make -j$(NPROCS) $(MAKE_FLAGS)
	cd $< && make install

$(INSTALL_DIR)/lib/libaprutil-1.so.0: $(APRUTIL_DIR) $(INSTALL_DIR)/lib/libapr-1.so.0
	cd $< && ./configure --prefix=$(INSTALL_DIR) --with-apr=$(INSTALL_DIR)
	cd $< && make -j$(NPROCS) $(MAKE_FLAGS)
	cd $< && make install

$(INSTALL_DIR)/bin/httpd $(INSTALL_DIR)/bin/apxs: $(INSTALL_DIR)/lib/libapr-1.so.0 $(INSTALL_DIR)/lib/libaprutil-1.so.0
	[ -d $(HTTPD_DIR) ] || tar -xzf $(HTTPD_DIR).tar.gz
	[ -f $(HTTPD_DIR)/Makefile ] || \
	cd $(HTTPD_DIR) && ./configure --prefix=$(INSTALL_DIR) --with-apr=$(INSTALL_DIR) \
	--with-apr-util=$(INSTALL_DIR) --with-mpm=prefork
	cd $(HTTPD_DIR) && make -j$(NPROCS) $(MAKE_FLAGS)
	cd $(HTTPD_DIR) && make install

$(INSTALL_DIR)/modules/libphp5.so: $(PHP_DIR) $(INSTALL_DIR)/bin/apxs
	cd $< && ./configure --prefix=$(INSTALL_DIR) --with-apxs2=$(INSTALL_DIR)/bin/apxs \
	--disable-cgi --disable-cli --disable-soap
	cd $< && make -j$(NPROCS) $(MAKE_FLAGS)
	cd $< && make install

.PHONY: modules

modules: $(INSTALL_DIR)/modules/mod_auth_basic_sandbox.so

$(INSTALL_DIR)/modules/mod_auth_basic_sandbox.so: mod_auth_basic_sandbox.c $(INSTALL_DIR)/bin/apxs
	$(INSTALL_DIR)/bin/apxs $(APXS_FLAGS) \
		-S CFLAGS="-I../../../include" \
		-S LDFLAGS="-L../../../../build/libos -llibos" -c -i -A $<

%.manifest: %.manifest.template
	make -C .. apache/$@ appdir=apache/ \
		extra_rules="-e 's:\\\$$\$$(HOST):$(HOST):g' -e 's:\\\$$\$$(PORT):$(PORT):g'"


conf:
	cd $(INSTALL_DIR)/conf && ([ -f httpd.conf.old ] || mv httpd.conf httpd.conf.old)
	cd $(INSTALL_DIR)/conf && \
		sed -e "s/Listen 80/Listen $(HOST):$(PORT)/g" \
		    -e "s/User daemon/#User root/g" \
		    -e "s/Group daemon/#Group root/g" \
		    -e "s/#EnableMMAP off/EnableMMAP off/g" \
		    -e "s/#EnableSendfile on/EnableSendfile on/g" \
		    -e "s/DirectoryIndex index.html/DirectoryIndex index.html index.php/g" \
		    -e "s/^[ ]*CustomLog/#CustomLog/g" \
		httpd.conf.old > httpd.conf.new
	cd $(INSTALL_DIR)/conf && \
		echo "\n\
<IfModule mpm_prefork_module>\n\
    StartServers             4\n\
    MinSpareServers          1\n\
    MaxSpareServers          4\n\
    MaxConnectionsPerChild   0\n\
</IfModule>\n" >> httpd.conf.new
	cd $(INSTALL_DIR)/conf && ln -sf httpd.conf.new httpd.conf

start-native-server:
	$(PREFIX) $(INSTALL_DIR)/bin/httpd -D FOREGROUND

start-graphene-server:
	rm -rf $(INSTALL_DIR)/logs/httpd.pid
	$(PREFIX) ./httpd.manifest -D FOREGROUND

random-data = $(foreach n,1 2 3 4 5 6 7 8 9 10,2K.$n.html) \
	      $(foreach n,1 2 3 4 5,10K.$n.html) \
	      $(foreach n,1 2 3 4 5,100K.$n.html) \
	      $(foreach n,1 2 3,1M.$n.html) \
	      $(foreach n,1 2 3,10M.$n.html) \
	      $(foreach n,1 2 3,100.$n.html)

test-data = $(HTDOC)/oscar-web $(HTDOC)/oscar-web-static \
	    $(addprefix $(HTDOC)/random/,$(random-data)) \
	    $(HTDOC)/auth/secret.html $(HTDOC)/auth/.htaccess

$(HTDOC)/%: $(PWD)/../web-data/%.tar.gz
	[ -d "$@" ] || (mkdir -p $@ && cd $(HTDOC) && tar -xzf $^)

$(HTDOC)/random/%.html:
	[ -d $(HTDOC)/random ] || mkdir -p $(HTDOC)/random
	dd if=/dev/urandom of=$@ count=1 bs=$(basename $(basename $(notdir $@)))

$(HTDOC)/auth:
	mkdir -p $@

$(HTDOC)/auth/secret.html: $(HTDOC)/auth
	echo "This is the secret" > $@

$(HTDOC)/auth/.htpasswd: $(INSTALL_DIR)/bin/htpasswd $(HTDOC)/auth
	$< -b -c $@ test test

$(HTDOC)/auth/.htaccess: $(HTDOC)/auth/.htpasswd
	echo "AuthName \"Login\"\n\
AuthType Basic\n\
AuthUserFile $(HTDOC)/auth/.htpasswd\n\
AuthGroupFile /dev/null\n\
require user test" > $@

test-data: $(test-data)

distclean: clean
	rm -rf $(INSTALL_DIR) $(SRC_DIRS)

clean:
	rm -rf httpd.manifest $(test-data) \
		$(addprefix mod_auth_basic_sandbox.,la lo o slo)
