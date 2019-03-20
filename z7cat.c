/**
 * @file z7cat.c
 * @brief Create Intel Hex file from input binary files
 * @author mcjtag
 * @date 13.03.2019
 * @copyright MIT License
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>

#define BLK_SIZE 	65536		/* binary data block size (in bytes) */
#define HEX_SIZE	512		/* Temporary hex array (in bytes) */
#define REC_SIZE	512		/* Record char array (in bytes) */
#define BYTE_COUNT	32		/* Byte count of data record (in bytes), possible values: 16, 32, 255 */

static unsigned char blk_dat[BLK_SIZE];
static unsigned char hex_dat[HEX_SIZE];
static char record[REC_SIZE];

static void create_ela_record(unsigned short addr);
static void create_data_record(unsigned char *data, unsigned short offset);
static unsigned char checksum(int n);
static void hex2record(int n);
static int arg_parse(int argc, char *argv[]);
static void usage(char *arg);

/**
 * @brief Main function
 * @param argc Argument count
 * @param argv Argument vector
 * return 0 - success, -1 - failure
 */
int main(int argc, char *argv[])
{
	int tfd, ifd;
	FILE *mfd, *ftmp;
	int oi;
	unsigned int size = 0;
	unsigned int addr, offs;
	
	/* Parsing input arguments */
	oi = arg_parse(argc, argv);
	if (oi == -1)
		return -1;
	
	/* Create temporary binary file */
	ftmp = tmpfile();
	if (!ftmp) {
		perror("Open error");
		return -1;
	}
	tfd = fileno(ftmp);
	lseek(tfd, 0, SEEK_SET);
	
	/* Fill temporary file with zeros */
	memset(blk_dat, 0, BLK_SIZE);
	sscanf(argv[oi + 2], "%x", &size); /* Get size of temporary binary file, actually flash memory size (in bytes) */
	for (unsigned int u = 0; u < size; u += BLK_SIZE)
		if (write(tfd, blk_dat, BLK_SIZE) != BLK_SIZE) {
			fprintf(stderr, "Error: write operation\n");
			fclose(ftmp);
			return -1;
		}
	
	/* Fill temporary file with input files  */
	for (int i = 1; i < oi; i += 2) {
		ifd = open(argv[i], O_RDONLY);
		if (ifd == -1) {
			perror("Open error");
			fclose(ftmp);
			return -1;
		}
		sscanf(argv[i + 1], "%x", &addr); /* Get address offset of input file */
		lseek(tfd, addr, SEEK_SET);
		while((size = read(ifd, blk_dat, BLK_SIZE)) > 0)
			if (write(tfd, blk_dat, size) != size) {
				fprintf(stderr, "Error: write operation\n");
				fclose(ftmp);
				close(ifd);
				return -1;
			}
		close(ifd);
	}
	lseek(tfd, 0, SEEK_SET);
	
	/* Create output file */
	mfd = fopen(argv[oi + 1], "w");
	if (!mfd) {
		perror("Open error");
		return -1;
	}
	
	/* Create Intel Hex format content */
	addr = 0;
	while((size = read(tfd, blk_dat, BLK_SIZE)) > 0) { /* For each BLOCK */
		create_ela_record(addr);
		addr++;
		fprintf(mfd, ":%s\n", record);
		offs = 0;
		for (int i =  0; i < BLK_SIZE / BYTE_COUNT; i++) { /* For each SUBBLOCK (BYTE_COUNT) of BLOCK */
			create_data_record(&blk_dat[i*BYTE_COUNT], offs);
			offs += BYTE_COUNT;
			fprintf(mfd, ":%s\n", record);
		}
	}
	fprintf(mfd, ":00000001FF\n");
	
	fclose(mfd);
	fclose(ftmp);
	
	return 0;
}

/**
 * @brief Create Extended Linear Address record
 * @param addr Upper half of absolute address
 * @return void
 */
static void create_ela_record(unsigned short addr)
{
	hex_dat[0] = 2;
	hex_dat[1] = 0;
	hex_dat[2] = 0;
	hex_dat[3] = 4;
	hex_dat[4] = (addr >> 8) & 0xFF;
	hex_dat[5] = (addr >> 0) & 0xFF;
	hex_dat[6] = checksum(6);
	hex2record(7);
}

/**
 * @brief Create Data record
 * @param data Pointer to data array
 * @param offset Address offset (lower half of absolute address)
 * return void
 */
static void create_data_record(unsigned char *data, unsigned short offset)
{
	hex_dat[0] = BYTE_COUNT;
	hex_dat[1] = (offset >> 8) & 0xFF;
	hex_dat[2] = (offset >> 0) & 0xFF;
	hex_dat[3] = 0;
	for (int i = 0; i < BYTE_COUNT; i++)
		hex_dat[4+i] = data[i];
	hex_dat[4 + BYTE_COUNT] = checksum(4 + BYTE_COUNT);
	hex2record(5 + BYTE_COUNT);
}

/**
 * @brief Calculate Checksum of current record
 * @param n Number of bytes (hex array)
 * @return checksum
 */
static unsigned char checksum(int n)
{
	unsigned char cs = 0;
	
	for (int i = 0; i < n; i++)
		cs += hex_dat[i];
	cs = (cs ^ 0xFF) + 1;
	
	return cs;
}

/**
 * @brief Convert hex array to char array
 * @param n Number of bytes (hex array)
 * @return void
 */
static void hex2record(int n)
{
	char *pstr = record;
	for (int i = 0; i < n; i++)
		pstr += sprintf(pstr, "%02X", hex_dat[i]);
}

/**
 * @brief Parsing of input arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @return -1 - failure, otherwise - success
 */
static int arg_parse(int argc, char *argv[])
{
	int oi = 0;
	
	if (argc == 1) {
		usage(argv[0]);
		return -1;
	}
	
	for (int i = 1; i < argc; i++) {
		if (!strcmp("-o", argv[i])) {
			oi = i;
			break;
		}
	}
	
	if (!oi) {
		fprintf(stderr, "Error: output flag missed\n");
		usage(argv[0]);
		return -1;
	}
	
	if (oi + 3 > argc) {
		fprintf(stderr, "Error: too few output arguments\n");
		usage(argv[0]);
		return -1;
	}
	
	if ((oi - 1) % 2) {
		fprintf(stderr, "Error: incorrect number of input arguments\n");
		usage(argv[0]);
		return -1;
	}
	
	return oi;
}

/**
 * @brief Show usage information
 * @param arg Pointer to argv[0]
 * @return void
 */
static void usage(char *arg)
{
	printf("Usage: %s [<input_file> <address_offset>] [...] -o <output_file> <file_size>\n", arg);
	printf("Example: %s ifile0 0x0 ifile1 0x1000 -o ofile 0x1000000\n", arg);
}