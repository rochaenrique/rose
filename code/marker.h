#ifndef _H_MARKER_
#define _H_MARKER_

//#include <stdint.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdarg.h>

#define swap(a, b, Type) \
do { Type swap_temp = a; a = b; b = swap_temp; } while (0)

typedef struct Read_File_Result
{
  void *memory;
  U32 size;
} Read_File_Result;

typedef struct High_Freq_Time
{
  U64 ns;
  F64 ms;
  F64 s;
} High_Freq_Time;

#endif _H_MARKER_

