/* date = April 22nd 2026 6:15 pm */

#ifndef LINUX_OS_H
#define LINUX_OS_H

typedef struct LNX_Alloc_Header LNX_Alloc_Header;
struct LNX_Alloc_Header
{
  U64 size;
};

#define OS_ENTRY_POINT() int main()


#endif //LINUX_OS_H

