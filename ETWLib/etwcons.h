#pragma once


#define EVENT_TRACE_TYPE_PERFSAMPLE 46
#define EXTENSION_SIZE 256

namespace ETWLib
{
    struct FlagExtension
    {
        unsigned short offset;
        unsigned char length = 0xff;
        unsigned char flag = 0x80;
    };

    struct ExtHeader
    {
        unsigned short number;
        unsigned short flag;
    };

    struct TraceExtension
    {
        TraceExtension()
        {
            std::memset(data, 0, sizeof(data));
        }
        unsigned long data[EXTENSION_SIZE];

        void AppendItem(unsigned int start, ExtHeader& header, unsigned long* items)
        {
            std::memcpy(data + start, &header, sizeof(unsigned long));
            if (items)
                std::memcpy(data + start + 1, items, sizeof(unsigned long) * (header.number - 1));
        }
    };
}