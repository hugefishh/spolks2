#ifndef TYPES_H
#define TYPES_H

#ifdef __linux__
#include <sys/time.h> 
#endif // __linux__




typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long long  SOCKET;

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)


#endif //TYPES_H


