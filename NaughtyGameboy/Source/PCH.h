#pragma once

// byte and ushort are moth unsigned shorts. The ideas is to be human readable, but to eliminate type casting between them
typedef unsigned short byte;
typedef unsigned short ushort;

#define ARRAY_SIZE(arr) (sizeof arr / sizeof arr[0])
