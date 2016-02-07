/*
 * Copy me if you can.
 * by FRIGN
 */
#include <arpa/inet.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resample.h"

void
fix_ratio(uint32_t sw, uint32_t sh, uint32_t *dw, uint32_t *dh)
{
	double x, y;

	x = *dw / (double)sw;
	y = *dh / (double)sh;

	if (x && (!y || x<y)) {
		*dh = (sh * x) + 0.5;
	} else {
		*dw = (sw * y) + 0.5;
	}

	if (!*dh) {
		*dh = 1;
	}
	if (!*dw) {
		*dw = 1;
	}
}

int
main(int argc, char *argv[])
{
	uint32_t width_in, height_in, width_out, height_out, i, j, tmp32;
	size_t psl_len, psl_offset, buf_in_len, buf_out_len;
	uint16_t *io_buf;
	uint8_t hdr[strlen("farbfeld") + 2 * sizeof(uint32_t)];
	uint8_t *tmp, *psl_buf, *psl_pos0, *sl_out;
	char *end;
	struct yscaler ys;

	if (argc != 3) {
		fprintf(stderr, "usage: %s [BOX WIDTH] [BOX HEIGHT]\n", argv[0]);
		return 1;
	}

	width_out = strtoul(argv[1], &end, 10);
	if (!end || !width_out) {
		fprintf(stderr, "bad width given\n");
		return 1;
	}
	height_out = strtoul(argv[2], &end, 10);
	if (!end || !height_out) {
		fprintf(stderr, "bad height given\n");
		return 1;
	}

	if (fread(hdr, 1, sizeof(hdr), stdin) != sizeof(hdr)) {
		fprintf(stderr, "incomplete header\n");
		return 1;
	}
	if (memcmp("farbfeld", hdr, strlen("farbfeld"))) {
		fprintf(stderr, "invalid magic\n");
		return 1;
	}
	width_in = ntohl(*((uint32_t *)(hdr + 8)));
	height_in = ntohl(*((uint32_t *)(hdr + 12)));

	fix_ratio(width_in, height_in, &width_out, &height_out);

	fputs("farbfeld", stdout);
	tmp32 = htonl(width_out);
	if (fwrite(&tmp32, sizeof(uint32_t), 1, stdout) != 1) {
		fprintf(stderr, "unable to write header\n");
		return 1;
	}
	tmp32 = htonl(height_out);
	if (fwrite(&tmp32, sizeof(uint32_t), 1, stdout) != 1) {
		fprintf(stderr, "unable to write header\n");
		return 1;
	}

	psl_len = padded_sl_len_offset(width_in, width_out, 4, &psl_offset);
	psl_buf = malloc(psl_len);
	psl_pos0 = psl_buf + psl_offset;
	
	sl_out = malloc(width_out * 4);
	buf_in_len = width_in * sizeof(uint16_t) * 4;
	buf_out_len = width_out * sizeof(uint16_t) * 4;
	io_buf = malloc(buf_in_len > buf_out_len ? buf_in_len : buf_out_len);
	yscaler_init(&ys, height_in, height_out, width_out * 4);

	for (i = 0; i < height_out; i++) {
		while ((tmp = yscaler_next(&ys))) {
			if (fread(io_buf, 1, buf_in_len, stdin) != buf_in_len) {
				fprintf(stderr, "unexpected EOF\n");
				return 1;
			}
			for (j = 0; j < width_in * 4; j++) {
				psl_pos0[j] = ntohs(io_buf[j]) / 257;
			}
			padded_sl_extend_edges(psl_buf, width_in, psl_offset, 4);
			xscale_padded(psl_pos0, width_in, tmp, width_out, 4);
		}
		yscaler_scale(&ys, sl_out, i);
		for (j = 0; j < width_out * 4; j++) {
			io_buf[j] = htons(sl_out[j] * 257);
		}
		if (fwrite(io_buf, 1, buf_out_len, stdout) != buf_out_len) {
			fprintf(stderr, "write error\n");
			return 1;
		}
	}
	return 0;
}
