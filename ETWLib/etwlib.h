#pragma once
#include <string.h>
#include <guiddef.h>
#include <vector>
#include <windows.h>

#define EVENT_TRACE_TYPE_PERFSAMPLE 46

namespace ETWLib
{
    void GrantPrivilegeA(const char** privileges, int count);
    void GrantPrivilegeW(const wchar_t** privileges, int count);

    enum TraceMode
    {
        LogFileMode = 0x1,
        RealTimeMode = 0x2
    };

    enum KernelModeProviderFlag
    {
        ALPC = 0,
        DiskIO = 1,
        EventTraceConfig = 2,
        FileIO = 3,
        Image = 4,
        PageFaultV2 = 5,
        ProfInfo = 6,
        Process = 7,
        Registry = 8,
        SplitIO = 9,
        TCPIP = 10,
        Thread = 11,
        Udplp = 12,
        KMPCount
    };

    struct ProviderEnableParameters
    {
        GUID guid;
        unsigned char eventId;
        bool stackwalk;
    };


    struct SessionParameters
    {
        SessionParameters();
        void EnableProfilling(bool profillingEnable);
        void AddKernelModeProvider(KernelModeProviderFlag, unsigned char eventid, bool stack);
        void AddUserModeProvider(std::wstring, bool stack);

        std::vector<ProviderEnableParameters> UserModeProviders;
        std::vector<ProviderEnableParameters> KernelModeProviders;
        ULONG EnableKernelFlags;
        ULONG MaxETLFileSize = 128;
        ULONG BufferSize = 1024;
        ULONG MinBuffers = 64;
        ULONG MaxBuffers = 128;
    };

    class ETWSessionImp;

    class ETWSession
    {
    public:
        ETWSession(std::wstring sessionName, std::wstring etlFile);
        ~ETWSession();

        void SetParameters(SessionParameters&);
        bool StartSession(TraceMode mode);
        bool CloseSession();
    protected:
        ETWSessionImp* m_pImp;
    };

}