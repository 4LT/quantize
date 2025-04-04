PLUGIN_NAME=quantize-alpha
VERSION=2
EXT=so

PKGCONFIG_PKGS=gegl-0.4
PKGCONFIG_CFLAGS=pkg-config --cflags $(PKGCONFIG_PKGS)
PKGCONFIG_LFLAGS=pkg-config --libs $(PKGCONFIG_PKGS)
CFLAGS=`$(PKGCONFIG_CFLAGS)` -Wall -std=c99 -fpic -O2 -I. \
	-DQU_VERSION=$(VERSION)
LFLAGS=`$(PKGCONFIG_LFLAGS)` -shared
BASE_DEPS=Makefile
OBJS=entry.o quakepal.o
INSTALL_PATH=$(HOME)/.local/share/gegl-0.4/plug-ins
INSTALLFLATPAK_PATH=$(HOME)/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins
ARTIFACT=$(PLUGIN_NAME)-$(VERSION).$(EXT)

.PHONY=all install clean

all: $(ARTIFACT)

install: $(ARTIFACT)
	mkdir -p $(INSTALL_PATH)
	cp $(ARTIFACT) $(INSTALL_PATH)/$(ARTIFACT)

install-flatpak: $(ARTIFACT)
	mkdir -p $(INSTALLFLATPAK_PATH)
	cp $(ARTIFACT) $(INSTALLFLATPAK_PATH)/$(ARTIFACT)


$(ARTIFACT): $(BASE_DEPS) $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(ARTIFACT)

entry.o: $(BASE_DEPS) entry.c quakepal.h
	$(CC) $(CFLAGS) -c entry.c

quakepal.o: $(BASE_DEPS) quakepal.c quakepal.h
	$(CC) $(CFLAGS) -c quakepal.c

clean:
	rm -f $(OBJS)
	rm -f $(PLUGIN_NAME)*.so
	rm -f $(PLUGIN_NAME)*.dll
