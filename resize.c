#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct image {
	uint32_t width;
	uint32_t height;
	uint16_t *data;
};

struct header {
	char sig[8];
	uint32_t width;
	uint32_t height;
};

void
xscale(uint16_t *row_in, uint16_t *out, uint32_t width_in, uint32_t width_out,
	uint32_t taps, uint32_t xpos)
{
	double x, tx, coeff, sum[4];
	uint32_t i, j, smp_i, smp_safe;
	int32_t val;

	sum[0] = sum[1] = sum[2] = sum[3] = 0.0;
	smp_i = (uint64_t)xpos * width_in / width_out;
	tx = ((uint64_t)xpos * width_in % width_out) / (double)width_out;

	for (i=1; i<=taps*2; i++) {
		x = (i > taps ? i - taps - tx : taps - i + tx) / (taps / 2);
		if (x < 1) {
			coeff = (3*x*x*x - 5*x*x + 2) / taps;
		} else {
			coeff = (-1*x*x*x + 5*x*x - 8*x + 4) / taps;
		}
		smp_safe = smp_i + i < taps ? 0 : smp_i + i - taps;
		smp_safe = smp_safe >= width_in ? width_in - 1 : smp_safe;
		for (j=0; j<4; j++) {
			sum[j] += row_in[smp_safe * 4 + j] / 65535.0 * coeff;
		}
	}

	for (i=0; i<4; i++) {
		val = 65535 * sum[i];
		out[i] = val < 0 ? 0 : (val > 65535 ? 65535 : val);
	}
}

void
xscale_transpose(struct image in, struct image out)
{
	uint16_t *in_row, *out_smp;
	uint32_t i, j, taps;

	taps = out.height < in.width ? 2 * in.width / out.height : 2;
	taps += taps & 1;

	for (i=0; i<out.width; i++) {
		in_row = in.data + i * in.width * 4;
		for (j=0; j<out.height; j++) {
			out_smp = out.data + (j * out.width + i) * 4;
			xscale(in_row, out_smp, in.width, out.height, taps, j);
		}
	}
}

void*
alloc_img(uint32_t width, uint32_t height)
{
	uint64_t tmp;
	size_t size;

	tmp = (uint64_t)width * height;
	size = 4 * sizeof(uint16_t);

	if (tmp > SIZE_MAX / size) {
		return 0;
	}
	return malloc(tmp * size);
}

int
main(int argc, char *argv[])
{
	struct header hdr;
	struct image in, tmp, out;
	size_t i, in_len, out_len;
	char *end;

	if (argc != 3) {
		fprintf(stderr, "usage: %s [width] [height]\n", argv[0]);
		return 1;
	}

	out.width = strtoul(argv[1], &end, 10);
	if (!end || !out.width) {
		fprintf(stderr, "bad width given\n");
		return 1;
	}
	out.height = strtoul(argv[2], &end, 10);
	if (!end || !out.height) {
		fprintf(stderr, "bad height given\n");
		return 1;
	}

	if (fread(&hdr, 1, sizeof(hdr), stdin) != sizeof(hdr)) {
		fprintf(stderr, "incomplete header\n");
		return 1;
	}
	if (memcmp("farbfeld", hdr.sig, strlen("farbfeld"))) {
		fprintf(stderr, "invalid magic\n");
		return 1;
	}
	in.width = ntohl(hdr.width);
	in.height = ntohl(hdr.height);
	if (!in.width || !in.height) {
		fprintf(stderr, "bad input image\n");
		return 1;
	}

	/* Fix the aspect ratio. */
	if (out.width / (double)in.width < out.height / (double)in.height) {
		out.height = in.height * out.width / in.width;
		out.height = out.height ? out.height : 1;
	} else {
		out.width = in.width * out.height / in.height;
		out.width = out.width ? out.width : 1;
	}

	hdr.width = htonl(out.width);
	hdr.height = htonl(out.height);
	if (fwrite(&hdr, sizeof(hdr), 1, stdout) != 1) {
		fprintf(stderr, "unable to write header\n");
		return 1;
	}

	tmp.width = in.height;
	tmp.height = out.width;

	in_len = in.width * in.height * 4;
	out_len = out.width * out.height * 4;

	in.data = alloc_img(in.width, in.height);
	out.data = alloc_img(out.width, out.height);
	tmp.data = alloc_img(tmp.width, tmp.height);

	if (!in.data || !out.data || !tmp.data) {
		fprintf(stderr, "unable to allocate memory.\n");
		return 1;
	}

	if (fread(in.data, sizeof(uint16_t), in_len, stdin) != in_len) {
		fprintf(stderr, "unexpected EOF\n");
		return 1;
	}
	for (i = 0; i < in_len; i++) {
		in.data[i] = ntohs(in.data[i]);
	}

	xscale_transpose(in, tmp);
	xscale_transpose(tmp, out);

	for (i = 0; i < out_len; i++) {
		out.data[i] = htons(out.data[i]);
	}
	if (fwrite(out.data, sizeof(uint16_t), out_len, stdout) != out_len) {
		fprintf(stderr, "write error\n");
		return 1;
	}
	return 0;
}
