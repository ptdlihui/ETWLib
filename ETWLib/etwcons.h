#pragma once


struct FlagExtension
{
    unsigned short offset;
    unsigned char length;
    unsigned char flag;
};

#define EXTENSION_SIZE 256
struct TraceExtension
{
    unsigned long data[EXTENSION_SIZE];
};