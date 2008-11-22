#include "crc.h"


static u_int32_t crcTable[256];


void crc_gen() {
	u_int32_t crc, poly = 0xEDB88320;
	int i, j;

	for(i = 0; i < 256; i++) {
		crc = i;
		for(j = 8; j > 0; j--) {
			if( crc & 1 )
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		crcTable[i] = crc;
	}
}

u_int32_t crc_calc_string(u_int32_t crc, const char * string) {
	u_int32_t c;

	crc ^= 0xFFFFFFFF;

	while( '\0' != (c = *string++) ) 
		crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ c];

	return crc ^ 0xFFFFFFFF;
}

u_int32_t crc_calc_array(u_int32_t crc, const char * array, size_t size) {
	u_int32_t c;

	crc ^= 0xFFFFFFFF;

	while( size-- ) {
		c = *array++;
		crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ c];
	}

	return (crc ^ 0xFFFFFFFF);
}
