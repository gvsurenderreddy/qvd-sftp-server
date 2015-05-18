#CC=i686-w64-mingw32-gcc
CC=gcc
CFLAGS=-g -Werror

SOURCES=bufaux.c \
	buffer.c \
	fatal.c \
	log.c \
	misc.c \
	sshbuf.c \
	sshbuf-getput-basic.c \
	ssherr.c \
	openbsd-compat/bsd-misc.c \
	openbsd-compat/fmt_scaled.c \
	openbsd-compat/strlcat.c \
	openbsd-compat/strlcpy.c \
	openbsd-compat/strmode.c \
	openbsd-compat/vis.c \
	openbsd-compat/bsd-asprintf.c \
	openbsd-compat/strtonum.c \
	openbsd-compat/qvd_pw.c \
	openbsd-compat/strsep.c \
	sftp-common.c \
	sftp-server.c \
	sftp-server-main.c \
	xmalloc.c

sftp-server.exe: $(SOURCES:.c=.o)
	$(CC) --verbose  -o $@ $+ -lmsvcrt -lws2_32
	#strip $@
	
-include $(SOURCES:.c=.d)
	
%.o: %.c
	gcc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM $(CFLAGS) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

clean:
	rm -f sftp-server.exe *.o openbsd-compat/*.o *.d openbsd-compat/*.d
