#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void 
usage(char* program_name)
{
	printf("\nUsage: %s [nsets] [bsize] [assoc] [repl] [output flag] [input file]\n\n"
			, program_name);
}

uint32_t 
swap_endian(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint32_t
convtoul(char *argv) {
	char *endptr;
	unsigned long value;

    value = strtoul(argv, &endptr, 10);

    return (uint32_t)value;
}
