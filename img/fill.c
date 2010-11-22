#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define BUF_SZ			(1024*1024)
#define ARG_IDX_FILLVAL		1
#define	ARG_IDX_NUMBYTES	2
#define ARG_IDX_OUTFILE		3

/* creates an image filled with a given 32-bit value */

int main(int argc, char* argv[])
{
	char		*fname;
	uint32_t	fillval;
	uint32_t	*buf;
	uint32_t	desired_bytes;
	uint32_t	written_bytes;
	int		i;
	FILE		*f;

	if (argc != 4) {
		fprintf(stderr, "%s: fill_value num_bytes out_file\n", argv[0]);
		return -1;
	}

	fillval = atoi(argv[ARG_IDX_FILLVAL]);
	desired_bytes = atoi(argv[ARG_IDX_NUMBYTES]);
	fname = argv[ARG_IDX_OUTFILE];

	f = fopen(fname, "w");
	if (f == NULL) {
		fprintf(stderr, "%s: could not open %s\n", argv[0], fname);
		return -2;
	}

	buf = malloc(BUF_SZ);
	for (i = 0; i < BUF_SZ / sizeof(uint32_t); i++)
		buf[i] = fillval;

	written_bytes = 0;
	while (written_bytes < desired_bytes) {
		int		bytes_to_write;
		uint32_t	remaining_bytes;
		size_t		fwrite_elems;

		remaining_bytes = desired_bytes - written_bytes;
		if (remaining_bytes > BUF_SZ)
			bytes_to_write = BUF_SZ;
		else
			bytes_to_write = remaining_bytes;

		fwrite_elems = fwrite(buf, bytes_to_write, 1, f);
		assert (fwrite_elems == 1);

		written_bytes += bytes_to_write;
	}

	fclose(f);
	free(buf);

	return 0;
}
