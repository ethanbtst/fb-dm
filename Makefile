# .DEFAULT_GOAL := build

CC = gcc
PKG_CONFIG ?= pkg-config
PKGS = pam

CFLAGS := -Wall -g -Wextra ${shell $(PKG_CONFIG) --cflags $(PKGS)}
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
	cp -f fb-dm /opt/fb-dm
	cp -rf img /opt/fb-dm
	cp config.ini /opt/fb-dm
	cp -f fb-dm@.service /usr/lib/systemd/system