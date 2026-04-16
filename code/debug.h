/* date = April 7th 2026 1:21 pm */

#ifndef DEBUG_H
#define DEBUG_H

#include "core.h"

typedef enum Debug_State
{
  DebugState_None,
  DebugState_Recording,
  DebugState_Playing,
} Debug_State;

typedef struct Debug_Counters
{
  Debug_State state;
  f64 frame_time_ns;
  u64 cycles;
  u64 page_reclaims;
  u64 page_faults;
  u64 swaps;
  u64 fs_input;
  u64 fs_output;
} Debug_Counters;

typedef struct Debug_Info
{
  Debug_Counters past;
  Debug_Counters min;
  Debug_Counters max;
} Debug_Info;

#endif //DEBUG_H
