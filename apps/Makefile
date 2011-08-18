all: gorecord modet

gorecord: gorecord.c tv-freq.c
	gcc -Wall $(CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -o gorecord gorecord.c tv-freq.c

modet: modet.c
	gcc -Wall $(CFLAGS) -o modet modet.c -lncurses

install:
	@echo "Installing apps to /usr/bin/"
	install -m 0755 gorecord /usr/bin/
	install -m 0755 modet /usr/bin/

clean:
	rm -f gorecord modet *.o
