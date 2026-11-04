#!/bin/bash
3c \
-dump-stats \
-itypes-for-extern \
-alltypes \
-p \
compile_commands.json \
-extra-arg=-w \
-output-dir=out.checked \
cgi-src/ssi.c \
mmc.c \
cgi-src/redirect.c \
thttpd.c \
timers.c \
match.c \
fdwatch.c \
extras/htpasswd.c \
extras/makeweb.c \
tdate_parse.c \
libhttpd.c \
cgi-src/phf.c \
checkedc_utils.c \
unit_tests.c
