#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "util.h"

#define PROGRAM_NAME argv[0]
#define INPUT_FILE argv[1]

int 
main(int argc, char** argv)
{
	/* TODO argparse func */
	if (argc != 2) {
		usage(PROGRAM_NAME);
		return EXIT_FAILURE;
	}

	/* check if the input file exists */
	if (access(INPUT_FILE, F_OK)) {
		printf("Input file \"%s\" not found.\n", INPUT_FILE);
		return EXIT_FAILURE;
	}

	FILE* input = fopen(INPUT_FILE, "rb");
	char buffer[64];

	fread(buffer, sizeof(buffer), 64, input);

	for (int i = 0; i < 64; i++) 
		printf("%u ", buffer[i]);
	/* TODO convert to big endian */
	/* TODO for each byte { do cache operation } */
	printf("\n");

	return EXIT_SUCCESS;
}
