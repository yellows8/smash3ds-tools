#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>

/*
replay data format:
0x0 u32: checksum. val = 0; val ^= <each word in replay starting @ offset 0x4>
0x4 u32: magic word, 0x4b4d4343 "CCMK".
0x8 u32: replay version. 0 = original, 1 = game v1.0.4 update.
0xc u32: replay data size excluding the header, this must be >0.
0x10 u16: header size, must be 0x2480(with <=v1.0.4 at least).
0x12 u16: must be non-zero and <=2.
*/

unsigned int process_replay(unsigned char *buf, unsigned int bufsize)
{
	unsigned int *buf32 = (unsigned int*)buf;
	unsigned short *buf16 = (unsigned short*)buf;
	unsigned int ret=0;
	unsigned int original_checksum=0, calc_checksum = 0;
	unsigned int pos;

	if(buf32[0x4>>2] != 0x4b4d4343)
	{
		printf("Invalid magic-number.\n");
		return 0;
	}

	original_checksum = buf32[0];

	for(pos=1; pos<(bufsize>>2); pos++)calc_checksum ^= buf32[pos];

	printf("Original checksum = 0x%08x, calc checksum = 0x%08x.\n", original_checksum, calc_checksum);
	if(original_checksum != calc_checksum)
	{
		buf32[0] = calc_checksum;
		ret = 1;
	}

	printf("Replay version: 0x%x\n", buf32[0x8>>2]);
	printf("Replay data size excluding header: 0x%x\n", buf32[0xc>>2]);
	printf("Header size: 0x%x\n", buf16[0x10>>1]);
	printf("0x12 value: 0x%x\n", buf16[0x12>>1]);

	return ret;
}

int main(int argc, char **argv)
{
	unsigned char *buf;
	unsigned int bufsize;
	FILE *f;
	struct stat filestats;

	if(argc<2)
	{
		printf("smash3ds_replaytool by yellows8\n");
		printf("Tool for smash bros 3DS replay files from extdata.\n");
		printf("Usage:\n");
		printf("smash3ds_replaytool <path, such as /id/replay/rd*.bin>\n");
		return 0;
	}

	if(stat(argv[1], &filestats)==-1)
	{
		printf("Failed to stat file.\n");
		return 1;
	}

	bufsize = filestats.st_size;
	buf = (unsigned char*)malloc(bufsize);
	if(buf==NULL)
	{
		printf("Failed to alloc mem.\n");
		return 2;
	}

	f = fopen(argv[1], "r+");
	if(f==NULL)
	{
		printf("Failed to open file.\n");
		free(buf);
		return 1;
	}

	if(fread(buf, 1, bufsize, f) != bufsize)
	{
		printf("Failed to read to file.\n");
		free(buf);
		fclose(f);
		return 1;
	}

	if(process_replay(buf, bufsize))
	{
		printf("Writing updated data to the file...\n");
		fseek(f, 0, SEEK_SET);
		if(fwrite(buf, 1, bufsize, f) != bufsize)printf("Failed to write to file.\n");
	}

	free(buf);
	fclose(f);

	return 0;
}

