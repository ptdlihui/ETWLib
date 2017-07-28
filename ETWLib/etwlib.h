#pragma once
#include <string.h>
#include <guiddef.h>
#include <vector>
#include <windows.h>

#define EVENT_TRACE_TYPE_PERFSAMPLE 46
#define MAX_SESSION_COUNT 64
#define MAX_SESSION_NAME 1024
#define MAX_LOG_FILE_PATH_LENGTH 1024
#define MAX_PROCESS_NUMBER 8

namespace ETWLib
{
    using ETWTraceID = ULONG64;

    bool GrantPrivilegeA(const char** privileges, int count);
    bool GrantPrivilegeW(const wchar_t** privileges, int count);
	std::vector<std::wstring> GetUserProvidersName();
    enum TraceMode
    {
        LogFileMode = 0x1,
        RealTimeMode = 0x2
    };

    enum TraceLevel
    {
        LevelNone = 0,
        LevelCritical,
        LevleFatal,
        LevelError,
        LevelWarning,
        LevelInformation,
        LevelVerbose,
        LevelCount
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
        Heap = 13,
        KMPCount
    };

    struct ProviderEnableParameters
    {
        GUID guid;
        unsigned char eventId;
        bool stackwalk;
        DWORD processID[MAX_PROCESS_NUMBER];
    };


    struct SessionParameters
    {
        SessionParameters();
        void EnableProfilling(bool profillingEnable);
		void AddKernelModeProvider(KernelModeProviderFlag, unsigned char eventid, bool stack);
		void EraseKernelModeProvider(KernelModeProviderFlag);

        void AddUserModeProvider(std::wstring, bool stack, TraceLevel level = LevelVerbose, DWORD* pProcessIDs = nullptr, unsigned int count = 0);
		void EraseUserModeProvider(std::wstring);
		
        std::vector<ProviderEnableParameters> UserModeProviders;
        std::vector<ProviderEnableParameters> KernelModeProviders;
        ULONG EnableKernelFlags[8];
        ULONG MaxETLFileSize = 128;
        ULONG BufferSize = 1024;
        ULONG MinBuffers = 64;
        ULONG MaxBuffers = 1024;
    };

    struct SessionInfo : public SessionParameters
    {
        std::wstring SessionName;
        std::wstring LogFileName;
        ULONG64 TraceHandle;
    };

    void QueryAllSessions(std::vector<SessionInfo>&);

    class ETWSessionImp;

    class ETWSession
    {
    public:
        ETWSession(std::wstring sessionName, std::wstring etlFile);
        ETWSession(ETWTraceID traceHandle);
        ~ETWSession();

        ETWTraceID TraceID() const;

        void SetParameters(SessionParameters&);
        bool StartSession(TraceMode mode);
        bool CloseSession();
    protected:
        ETWSessionImp* m_pImp;
    };

    class ETWSessionConsumerImp;

    class ETWSessionConsumer
    {
    public:
        ETWSessionConsumer(ULONG64 traceHandle, std::wstring sessionName, std::wstring etlFile);
        ~ETWSessionConsumer();

        SessionInfo& SessionInfomation();
    protected:
        ETWSessionConsumerImp* m_pImp;
    };

}