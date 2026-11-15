#!/bin/bash

gcc  -g $(pkg-config --cflags --libs ncurses pam) pam_login.c image.c main.c -o image