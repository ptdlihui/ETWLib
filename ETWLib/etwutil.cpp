#include "etwutil.h"
#include <evntrace.h>
#include <evntcons.h>
#include <assert.h>
#include <strsafe.h>

namespace ETWLibUtil
{
    void ConvertPropertiesToInfo(PEVENT_TRACE_PROPERTIES pProperties, ETWLib::SessionInfo& info)
    {
        assert(pProperties);

        info.BufferSize = pProperties->BufferSize;
        info.MaxBuffers = pProperties->MaximumBuffers;
        info.MinBuffers = pProperties->MinimumBuffers;
        info.EnableKernelFlags[0] = pProperties->EnableFlags;
        info.MaxETLFileSize = pProperties->MaximumFileSize;

        info.SessionName = std::wstring((const wchar_t*)((const char*)(pProperties)+pProperties->LoggerNameOffset));
        info.LogFileName = std::wstring((const wchar_t*)((const char*)(pProperties)+pProperties->LogFileNameOffset));
    }

    bool 
    GetSessionInformation(ETWLib::ETWTraceID handle, ETWLib::SessionInfo& info)
    {
        std::vector<unsigned char> buffer;
        buffer.resize(sizeof(EVENT_TRACE_PROPERTIES) + MAX_SESSION_NAME + MAX_LOG_FILE_PATH_LENGTH);
        std::memset(buffer.data(), 0, buffer.size());
        PEVENT_TRACE_PROPERTIES pProperties = (PEVENT_TRACE_PROPERTIES)buffer.data();
        pProperties->Wnode.BufferSize = buffer.size();
        pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        pProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + MAX_SESSION_NAME;

        ULONG status = ::QueryTraceW(handle, nullptr, pProperties);
        if (status == ERROR_SUCCESS)
        {
            ConvertPropertiesToInfo(pProperties, info);
            info.TraceHandle = handle;
            return true;
        }

        return false;
    }
}