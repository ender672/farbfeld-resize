# farbfeld - suckless image format with conversion tools
# See LICENSE file for copyright and license details
include config.mk

resize: resize.c resample.o
	${CC} resample.o -o $@ ${CFLAGS} ${LIBS} ${LDFLAGS} resize.c

clean:
	rm -f resample.o resize
