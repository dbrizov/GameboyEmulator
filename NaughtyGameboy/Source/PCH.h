#pragma once

#include <memory>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0])) // Works only for arrays on the stack

// byte and ushort are both unsigned shorts. The idea is to be human readable, but to eliminate type casting between them
typedef unsigned short byte;
typedef unsigned short ushort;

// The ulong is used for counthing the CPU clock cycles
typedef unsigned long ulong;
