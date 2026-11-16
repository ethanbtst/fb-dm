# .DEFAULT_GOAL := build

CC = gcc
PKG_CONFIG ?= pkg-config
PKGS = pam

CFLAGS := -Wall -Wextra ${shell $(PKG_CONFIG) --cflags $(PKGS)}
LDLIBS := $(shell $(PKG_CONFIG) --libs $(PKGS))

TARGET = fb-dm
SRCS = src/main.c src/image.c src/term_utils.c src/pam_login.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

uninstall:
	rm -rf /opt/fb-dm

install:
	mkdir -p /opt/fb-dm
	cp fb-dm /opt/fb-dm/fb-dm
	cp -r lilyb /opt/fb-dm/lilyb
	cp fb-dm@.service /usr/lib/systemd/system/fb-dm@.service
