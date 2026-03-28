VERSION = $(shell cat VERSION)
CFLAGS += -g -Wall -DAPP_VERSION="\"$(VERSION)\"" -Wno-address-of-packed-member -Wextra
LDFLAGS +=
CC ?= gcc
PKG_CONFIG ?= pkg-config
POPPLER_CFLAGS = $(shell $(PKG_CONFIG) --cflags poppler-glib)
POPPLER_LIBS = $(shell $(PKG_CONFIG) --libs poppler-glib)
UNAME_S := $(shell uname -s)
DIST_DIR = dist
DMG_NAME = $(DIST_DIR)/pdf-webslides_$(VERSION)-macos.dmg
DMG_STAGE = $(DIST_DIR)/pdf-webslides-$(VERSION)-macos

all: pdf-webslides

pdf-webslides: webslides.o cli.o utils.o res.o
	$(CC) webslides.o cli.o utils.o res.o $(LDFLAGS) -o pdf-webslides $(POPPLER_LIBS)

webslides.o: webslides.c
	$(CC) webslides.c -c $(CFLAGS) $(POPPLER_CFLAGS)
	
cli.o: cli.c
	$(CC) cli.c -c $(CFLAGS)
	
utils.o: utils.c
	$(CC) utils.c -c $(CFLAGS)
	
res.o: resconv res.c freeze.svg black.svg open.svg index.html.template
	./resconv freeze.svg > res_gen.c
	./resconv black.svg >> res_gen.c
	./resconv open.svg >> res_gen.c
	./resconv index.html.template >> res_gen.c
	$(CC) res.c -c $(CFLAGS)

resconv: resconv.c
	$(CC) resconv.c -o resconv

deb: pdf-webslides
	fakeroot -u ./makedeb.sh

dmg: pdf-webslides
ifeq ($(UNAME_S),Darwin)
	mkdir -p $(DIST_DIR)
	rm -rf $(DMG_STAGE)
	mkdir -p $(DMG_STAGE)
	cp pdf-webslides README.md LICENSE $(DMG_STAGE)/
	ln -sfn /Applications $(DMG_STAGE)/Applications
	rm -f $(DMG_NAME)
	hdiutil create -volname "pdf-webslides $(VERSION)" -srcfolder $(DMG_STAGE) -ov -format UDZO $(DMG_NAME)
else
	@echo "The dmg target is only supported on macOS."
	@exit 1
endif
	
clean:
	rm -rf *.o *.deb pdf-webslides resconv $(DMG_STAGE) $(DMG_NAME)
