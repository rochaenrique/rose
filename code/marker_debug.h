/* date = April 7th 2026 1:21 pm */

#ifndef DEBUG_H
#define DEBUG_H

#include "marker.h"

typedef enum Debug_State
{
  DebugState_None,
  DebugState_Recording,
  DebugState_Playing,
} Debug_State;

typedef struct Debug_Counters
{
  Debug_State state;
  F64 frame_time_ns;
  U64 cycles;
  U64 page_reclaims;
  U64 page_faults;
  U64 swaps;
  U64 fs_input;
  U64 fs_output;
} Debug_Counters;

typedef struct Debug_Info
{
  Debug_Counters past;
  Debug_Counters min;
  Debug_Counters max;
} Debug_Info;

#endif //DEBUG_H
