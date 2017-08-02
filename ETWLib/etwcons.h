#pragma once


#define EVENT_TRACE_TYPE_PERFSAMPLE     46
#define EXTENSION_SIZE                  256

#define EXT_BUFFER_GROUPMASK            0x1
#define EXT_BUFFER_PROCESS              0x2
#define EXT_BUFFER_STACKWALK            0x3

// MASK 1
#define PERF_MEMORY                     0x00000001
#define PERF_PROFILE                    0x00000002
#define PERF_CONTEXT_SWITCH             0x00000004
#define PERF_FOOTPRINT                  0x00000008
#define PERF_DRIVERS                    0x00000010
#define PERF_REFSET                     0x00000020
#define PERF_POOL                       0x00000040 
#define PERF_POOLTRACE                  0x00000041
#define PERF_DPC                        0x00000080 
#define PERF_COMPACT_CSWITCH            0x00000100 
#define PERF_DISPATCHER                 0x00000200
#define PERF_PMC_PROFILE                0x00000400 
#define PERF_PROFILING                  0x00000402 
#define PERF_PROCESS_INSWAP             0x00000800 
#define PERF_AFFINITY                   0x00001000
#define PERF_PRIORITY                   0x00002000 
#define PERF_INTERRUPT                  0x00004000
#define PERF_VIRTUAL_ALLOC              0x00008000 
#define PERF_SPINLOCK                   0x00010000
#define PERF_SYNC_OBJECTS               0x00020000 
#define PERF_DPC_QUEUE                  0x00040000
#define PERF_MEMINFO                    0x00080000 
#define PERF_CONTMEM_GEN                0x00100000 
#define PERF_SPINLOCK_CNTRS             0x00200000 
#define PERF_SPININSTR                  0x00210000 
#define PERF_SESSION                    0x00400000 
#define PERF_MEMINFO_WS                 0x00800000 
#define PERF_KERNEL_QUEUE               0x01000000 
#define PERF_INTERRUPT_STEER            0x02000000
#define PERF_SHOULD_YIELD               0x04000000
#define PERF_WS                         0x08000000

//Mask 2
#define PERF_ANTI_STARVATION            0x00000001
#define PERF_PROCESS_FREEZE             0x00000002
#define PERF_PFN_LIST                   0x00000004
#define PERF_WS_DETAIL                  0x00000008
#define PERF_WS_ENTRY                   0x00000010
#define PERF_HEAP                       0x00000020
#define PERF_SYSCALL                    0x00000040
#define PERF_UMS                        0x00000080
#define PERF_BACKTRACE                  0x00000100
#define PERF_VULCAN                     0x00000200 
#define PERF_OBJECTS                    0x00000400 
#define PERF_EVENTS                     0x00000800
#define PERF_FULLTRACE                  0x00001000
#define PERF_DFSS                       0x00002000
#define PERF_PREFETCH                   0x00004000
#define PERF_PROCESSOR_IDLE             0x00008000 
#define PERF_CPU_CONFIG                 0x00010000
#define PERF_TIMER                      0x00020000
#define PERF_CLOCK_INTERRUPT            0x00040000 
#define PERF_LOAD_BALANCER              0x00080000 
#define PERF_CLOCK_TIMER                0x00100000
#define PERF_IDLE_SELECTION             0x00200000
#define PERF_IPI                        0x00400000 
#define PERF_IO_TIMER                   0x00800000
#define PERF_REG_HIVE                   0x01000000
#define PERF_REG_NOTIF                  0x02000000
#define PERF_PPM_EXIT_LATENCY           0x04000000 
#define PERF_WORKER_THREAD              0x08000000 

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