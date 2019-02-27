#include "end.h"

namespace sbf {

int is_big_endian(void){
	union {
		uint32_t i;
		char c[4];
	} bi = { 0x01020304 };

	return bi.c[0] == 1;
}

} //namespace sbf