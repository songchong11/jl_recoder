
#ifndef _IMA_ADPCM_H_
#define _IMA_ADPCM_H_
#include "system/includes.h"

#if 0
typedef unsigned int uint;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
#endif

typedef struct
{
	int valprev;
	int index;
}CodecState;

extern CodecState mg;

void encode(CodecState* state, s16* input, int numSamples, u8* output);
void decode(CodecState* state, u8* input, int numSamples, s16* output);
void adpcm_init();

#endif //_IMA_ADPCM_H_

