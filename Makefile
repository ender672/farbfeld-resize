# farbfeld - suckless image format with conversion tools
# See LICENSE file for copyright and license details
include config.mk

resize: resize.c

clean:
	rm -f resize
