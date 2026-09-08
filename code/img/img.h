/* date = June 4th 2026 2:38 pm */

#ifndef IMG_H
#define IMG_H

typedef struct IMG_Loaded IMG_Loaded;
struct IMG_Loaded
{
  V2i size;
  I32 channels;
  String8 data;
};

#endif //IMG_H
