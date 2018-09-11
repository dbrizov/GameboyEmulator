#pragma once

#include <memory>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0])) // Works only for arrays on the stack

typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned short ushort;
typedef unsigned long ulong;
