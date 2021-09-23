/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

extern int log_printf(char* fmt, ...);

namespace dst {

auto GET_BIT = [](auto base, auto index) {
	return (((unsigned char*)base)[index >> 3] >> (7 - (index & 7))) & 1;
};

auto GET_NIBBLE = [](auto base, auto index) {
	return (((unsigned char*)base)[index >> 1] >> ((index & 1) << 2)) & 0x0f;
};

}

#endif
