#include "etwlib.h"
#include "etwproviders.h"
#include <evntrace.h>
#include <evntcons.h>
#include <assert.h>
#include <strsafe.h>
#include "etwutil.h"
#include "etwcons.h"

#define MAX_FILTER_NUMBER 8
static ETWLib::ETWProviders Providers;

template <typename char_type>
BOOL __LookupPrivilegeValue__(const char_type* , const char_type* , LUID* )
{

}

template <>
BOOL __LookupPrivilegeValue__<char>(const char* systemName, const char* privilege, LUID* luid)
{
    return LookupPrivilegeValueA(systemName, privilege, luid);
}

template <>
BOOL __LookupPrivilegeValue__<wchar_t>(const wchar_t* systemName, const wchar_t* privilege, LUID* luid)
{
    return LookupPrivilegeValueW(systemName, privilege, luid);
}

template <typename char_type> 
BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    const char_type* lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!__LookupPrivilegeValue__<char_type>(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid))        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        printf("AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}

namespace ETWLib
{
	std::vector<std::wstring> GetUserProvidersName() 
	{
		return Providers.AllProviders_Name();
	}
	bool GrantPrivilegeW(const wchar_t** privileges, int count)
    {
        if (count < 1)
            return false;

        HANDLE thisProcessToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &thisProcessToken))
        {
            for (int i = 0; i < count; i++)
                SetPrivilege<wchar_t>(thisProcessToken, privileges[i], TRUE);
			return true;
        }
		else 
		{
			return false;
		}
    }

    bool GrantPrivilegeA(const char** privileges, int count)
    {
        if (count < 1)
            return false;

        HANDLE thisProcessToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &thisProcessToken))
        {
            for (int i = 0; i < count; i++)
                SetPrivilege<char>(thisProcessToken, privileges[i], TRUE);
			return true;
        }
		else 
		{
			return false;
		}
    }

    SessionParameters::SessionParameters(SessionType type)
    {
        std::memset(EnableKernelFlags, 0, sizeof(EnableKernelFlags));
        EnableKernelFlags[0] = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD | EVENT_TRACE_FLAG_IMAGE_LOAD | EVENT_TRACE_FLAG_VIRTUAL_ALLOC;
        
        std::memset(ProcessIDs, 0, sizeof(ProcessIDs));
        Type = type;
    }

    void
    SessionParameters::AddKernelModeProvider(KernelModeProviderFlag provider, unsigned short eventid, bool stack)
    {
        ProviderEnableParameters params;
        params.guid = Providers.KernelModeProvider(provider);
        params.eventId = eventid;
        params.stackwalk = stack;
        std::memset(params.processID, 0, sizeof(params.processID));

        KernelModeProviders.push_back(params);
    }

	void 
	SessionParameters::EraseKernelModeProvider(KernelModeProviderFlag provider) 
	{
		for (auto itor = KernelModeProviders.begin(); itor != KernelModeProviders.end(); ++itor) 
		{
			if (itor->guid == Providers.KernelModeProvider(provider))
			{
				KernelModeProviders.erase(itor);
			}
		}
	}

    void
    SessionParameters::AddUserModeProvider(std::wstring name, bool stack, TraceLevel level, DWORD* pProcessIDs, unsigned int count)
    {
        ProviderEnableParameters params;
        params.guid = Providers.UserModeProvider(name);
        params.eventId = level;
        params.stackwalk = stack;
        std::memset(params.processID, 0, sizeof(DWORD) * MAX_PROCESS_NUMBER);

        if (count > 0 && count <= MAX_PROCESS_NUMBER)
        {
            std::memcpy(params.processID, pProcessIDs, count * sizeof(DWORD));
        }

        UserModeProviders.push_back(params);
    }

	void 
	SessionParameters::EraseUserModeProvider(std::wstring name) 
	{
		auto itor = UserModeProviders.begin();
		while (itor != UserModeProviders.end()) 
		{
			if (itor->guid == Providers.UserModeProvider(name))
			{
				itor = UserModeProviders.erase(itor);
			}
			else 
			{
				++itor;
			}
		}
	}

    void
    SessionParameters::EnableProfilling(bool enabled)
    {
        if (enabled == false)
            return;

        EnableKernelFlags[0] |= EVENT_TRACE_FLAG_PROFILE;

        for (auto& instance : UserModeProviders)
            instance.stackwalk = true;

        bool kernelProviderFound = false;
        GUID profguid = Providers.KernelModeProvider(ProfInfo);

        for (auto& instance : KernelModeProviders)
            if (instance.guid == profguid && instance.eventId == EVENT_TRACE_TYPE_PERFSAMPLE)
            {
                instance.stackwalk = true;
                kernelProviderFound = true;
            }

        if (kernelProviderFound == false)
        {
            ProviderEnableParameters perfProvider;
            perfProvider.guid = profguid;
            perfProvider.eventId = EVENT_TRACE_TYPE_PERFSAMPLE;
            perfProvider.stackwalk = true;

            KernelModeProviders.push_back(perfProvider);
        }
    }


    struct SessionContext : public SessionParameters
    {
        void copyFromParameters(SessionParameters& params)
        {
            KernelModeProviders.resize(params.KernelModeProviders.size());
            std::copy(params.KernelModeProviders.begin(), params.KernelModeProviders.end(), KernelModeProviders.begin());

            UserModeProviders.resize(params.UserModeProviders.size());
            std::copy(params.UserModeProviders.begin(), params.UserModeProviders.end(), UserModeProviders.begin());

            std::memcpy(EnableKernelFlags, params.EnableKernelFlags, sizeof(EnableKernelFlags));
            MaxETLFileSize = params.MaxETLFileSize;
            BufferSize = params.BufferSize;
            MinBuffers = params.MinBuffers;
            MaxBuffers = params.MaxBuffers;

            Type = params.Type;
            std::memcpy(ProcessIDs, params.ProcessIDs, sizeof(ProcessIDs));
        }

        std::wstring m_sessionName;
        std::wstring m_etlFileName;

        std::vector<unsigned char> m_etwPropertiesBuffer;

        TraceMode m_mode;
        TRACEHANDLE m_traceHandle;
        unsigned int m_extensionOffset = 0;

        void allocETWProperties(std::wstring sessionName, std::wstring etlFileName)
        {
            m_sessionName = sessionName;
            m_etlFileName = etlFileName;

            m_etwPropertiesBuffer.resize(sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.size() + etlFileName.size() + 2) * sizeof(std::wstring::value_type) + EXTENSION_SIZE * sizeof(ULONG));
            std::memset(m_etwPropertiesBuffer.data(), 0, m_etwPropertiesBuffer.size());

            PEVENT_TRACE_PROPERTIES pProperties = etwProperties();

            pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            pProperties->LogFileNameOffset = (etlFileName.size() > 0 ? (sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.size() + 1) * sizeof(std::wstring::value_type)) : 0);

            StringCchCopyW((LPWSTR)(m_etwPropertiesBuffer.data() + pProperties->LoggerNameOffset), (sessionName.size() + 1), sessionName.c_str());

            if (pProperties->LogFileNameOffset > 0)
                StringCchCopyW((LPWSTR)(m_etwPropertiesBuffer.data() + pProperties->LogFileNameOffset), (etlFileName.size() + 1), etlFileName.c_str());

            pProperties->Wnode.BufferSize = m_etwPropertiesBuffer.size();
            m_mode = (pProperties->LogFileNameOffset > 0) ? LogFileMode : RealTimeMode;

            m_extensionOffset = pProperties->LogFileNameOffset + (etlFileName.size() + 1) * sizeof(std::wstring::value_type);
            
        }

        PEVENT_TRACE_PROPERTIES etwProperties()
        {
            assert(m_etwPropertiesBuffer.size() > sizeof(EVENT_TRACE_PROPERTIES));
            return reinterpret_cast<PEVENT_TRACE_PROPERTIES>(m_etwPropertiesBuffer.data());
        }

        void ConfigPropertyBufferSize()
        {
            etwProperties()->MaximumFileSize = MaxETLFileSize;
            etwProperties()->BufferSize = BufferSize;
            etwProperties()->MinimumBuffers = MinBuffers;
            etwProperties()->MaximumBuffers = MaxBuffers;
        }

        void ConifgNormalSession()
        {
            assert(Type == NormalSession);
            PEVENT_TRACE_PROPERTIES pTraceProperties = etwProperties();

            if (m_mode == RealTimeMode)
                pTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

            if (m_mode == LogFileMode)
                pTraceProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;

            if (KernelModeProviders.size() > 0)
                pTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

            pTraceProperties->Wnode.ClientContext = 1;
            pTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

            ConfigPropertyBufferSize();
        }


        void ConfigHeapSession()
        {
            PEVENT_TRACE_PROPERTIES pTraceProperties = etwProperties();

            if (m_mode == RealTimeMode)
                pTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

            if (m_mode == LogFileMode)
                pTraceProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_INDEPENDENT_SESSION_MODE;

            if (KernelModeProviders.size() > 0)
                pTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

            pTraceProperties->Wnode.ClientContext = 1;
            pTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
            pTraceProperties->Wnode.Guid = HeapGuid;



        }
    };


    class ETWSessionImp
    {
    public:
        ETWSessionImp(std::wstring sessionName, std::wstring etlFile)
        {
            m_context.allocETWProperties(sessionName, etlFile);
            m_context.m_traceHandle = 0;
            m_attached = false;
        }

        ETWSessionImp(ETWTraceID id)
        {
            m_attached = true;
            attachToTrace(id);
        }

        void setParameters(SessionParameters& params)
        {
            m_context.copyFromParameters(params);
        }

        bool startSession(TraceMode mode)
        {
            ULONG status = startSessionImp(mode);
            return status == ERROR_SUCCESS;
        }

        bool closeSession()
        {
            ULONG status = closeTrace();
            return status == ERROR_SUCCESS;
        }

        ETWTraceID traceID()
        {
            return m_context.m_traceHandle;
        }

    protected:
        ULONG attachToTrace(ULONG64 traceHandle)
        {
            m_context.m_traceHandle = traceHandle;

            SessionInfo info;
            if (ETWLibUtil::GetSessionInformation(traceHandle, info))
            {
                m_context.allocETWProperties(info.SessionName, info.LogFileName);
                m_context.copyFromParameters(info);

                ULONG status = ::QueryTraceW(traceHandle, nullptr, m_context.etwProperties());
                return status;
            }

            return ERROR_INVALID_PARAMETER;
            

        }

        ULONG startSessionImp(TraceMode mode)
        {
            if (m_context.m_traceHandle != 0)
                return ERROR_INVALID_HANDLE;

            m_context.m_mode = mode;

            ULONG status = startTrace();
            if (status != ERROR_SUCCESS)
                return status;

            status = enableUserModeProviders();
            if (status != ERROR_SUCCESS)
                return status;

            status = enableKerneProviders();
            if (status != ERROR_SUCCESS)
                return status;

            return ERROR_SUCCESS;
        }
        ULONG startTrace()
        {
            if (m_attached)
                return ERROR_ALREADY_EXISTS;

            PEVENT_TRACE_PROPERTIES pTraceProperties = m_context.etwProperties();

            if (m_context.m_mode == RealTimeMode)
                pTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

            if (m_context.m_mode == LogFileMode)
                pTraceProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;

            if (m_context.KernelModeProviders.size() > 0)
                pTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

            pTraceProperties->Wnode.ClientContext = 1;
            pTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

            pTraceProperties->MaximumFileSize = m_context.MaxETLFileSize;
            pTraceProperties->BufferSize = m_context.BufferSize;
            pTraceProperties->MinimumBuffers = m_context.MinBuffers;
            pTraceProperties->MaximumBuffers = m_context.MaxBuffers;


            ULONG status = StartTraceW(&(m_context.m_traceHandle), m_context.m_sessionName.c_str(), pTraceProperties);
            return status;
        }

        ULONG closeTrace()
        {
            if (m_context.m_traceHandle == 0)
                return ERROR_INVALID_HANDLE;

            ULONG status = 0;
            if (m_context.m_traceHandle <= MAX_SESSION_COUNT)
                ControlTraceW(m_context.m_traceHandle, nullptr, m_context.etwProperties(), EVENT_TRACE_CONTROL_STOP);
            else
                ControlTraceW(NULL, m_context.m_sessionName.c_str(), m_context.etwProperties(), EVENT_TRACE_CONTROL_STOP);

            return status;
        }

        ULONG enableKerneProviders()
        {
            if (m_context.m_traceHandle == 0)
                return ERROR_INVALID_HANDLE;

            std::vector<CLASSIC_EVENT_ID> kernelEvents;
            for (auto& instance : m_context.KernelModeProviders)
            {
                if (instance.stackwalk)
                {
                    CLASSIC_EVENT_ID eventid;
                    std::memset(&eventid, 0, sizeof(CLASSIC_EVENT_ID));
                    eventid.EventGuid = instance.guid;
                    eventid.Type = instance.eventId;
                    kernelEvents.push_back(eventid);
                }
            }

            ULONG status = ERROR_SUCCESS;
            if (kernelEvents.size() > 0)
                status = TraceSetInformation(m_context.m_traceHandle, TraceStackTracingInfo, kernelEvents.data(), kernelEvents.size() * sizeof(CLASSIC_EVENT_ID));

            if (status == ERROR_SUCCESS)
            {
                m_context.EnableKernelFlags[0] |= EVENT_TRACE_FLAG_VIRTUAL_ALLOC;
                m_context.EnableKernelFlags[2] = 0x40000020;
                status = TraceSetInformation(m_context.m_traceHandle, TraceSystemTraceEnableFlagsInfo, m_context.EnableKernelFlags, sizeof(m_context.EnableKernelFlags));
            }

            return status;
        }

        ULONG enableUserModeProviders()
        {
            if (m_context.m_traceHandle == 0)
                return ERROR_INVALID_HANDLE;

            ULONG status = ERROR_SUCCESS;
            for (auto& instance : m_context.UserModeProviders)
            {
				ENABLE_TRACE_PARAMETERS params{};
                if (instance.stackwalk)
                    params.EnableProperty = EVENT_ENABLE_PROPERTY_STACK_TRACE;
                params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
                if (addProcessIDConfig(instance))
                {
                    params.FilterDescCount = 1;
                    params.EnableFilterDesc = m_userModeProviderFilter;
                }

                status = EnableTraceEx2(m_context.m_traceHandle, &(instance.guid), EVENT_CONTROL_CODE_ENABLE_PROVIDER, instance.eventId , 0, 0, 0, &params);

                if (status != ERROR_SUCCESS)
                    return status;
            }
            return status;
        }

        bool addProcessIDConfig(ProviderEnableParameters& params)
        {
            if (params.processID[0] > 0)
            {
                std::memset(m_userModeProviderFilter, 0, sizeof(EVENT_FILTER_DESCRIPTOR) * MAX_FILTER_NUMBER);
                ULONG processIDCount = 0;
                for (int i = 0; i < MAX_FILTER_NUMBER; i++)
                {
                    if (params.processID[i] == 0)
                        break;
                    processIDCount++;
                };

                m_userModeProviderFilter[0].Ptr = reinterpret_cast<ULONGLONG>(params.processID);
                m_userModeProviderFilter[0].Size = processIDCount * sizeof(DWORD);
                m_userModeProviderFilter[0].Type = EVENT_FILTER_TYPE_PID;
                return true;
            }

            return false;
        }
    protected:
        SessionContext m_context;
        bool m_attached;
        EVENT_FILTER_DESCRIPTOR m_userModeProviderFilter[MAX_FILTER_NUMBER];
    };


    ETWSession::ETWSession(std::wstring sessionName, std::wstring etlFile)
    {
        m_pImp = new ETWSessionImp(sessionName, etlFile);
    }

    ETWSession::ETWSession(ETWTraceID id)
    {
        m_pImp = new ETWSessionImp(id);
    }

    ETWSession::~ETWSession()
    {
        if (m_pImp)
            delete m_pImp;
        m_pImp = nullptr;
    }

    void
    ETWSession::SetParameters(SessionParameters& params)
    {
        assert(m_pImp);
        m_pImp->setParameters(params);
    }

    bool
    ETWSession::StartSession(TraceMode mode)
    {
        assert(m_pImp);
        return m_pImp->startSession(mode);
    }

    bool
    ETWSession::CloseSession()
    {
        assert(m_pImp);
        return m_pImp->closeSession();
    }

    ETWTraceID
    ETWSession::TraceID() const
    {
        assert(m_pImp);
        return m_pImp->traceID();
    }

    class ETWSessionConsumerImp
    {
    public:
        ETWSessionConsumerImp(ULONG64 traceHandle, std::wstring sessionName, std::wstring etlFileName)
            : m_sessionName(sessionName)
            , m_etlFileName(etlFileName)
            , m_traceHandle(traceHandle)
            , m_handle(INVALID_PROCESSTRACE_HANDLE)
        {
            openSession();
        }

        ~ETWSessionConsumerImp()
        {
            if (m_handle)
                closeSession();
        }

        SessionInfo& information()
        {
            return m_info;
        }

        ETWTraceID traceID()
        {
            return m_info.TraceHandle;
        }

    protected:
        bool openSession()
        {
            EVENT_TRACE_LOGFILEW logFile{};
            logFile.LoggerName = m_sessionName.size() > 0  ? &m_sessionName[0] : nullptr;
            logFile.LogFileName = m_etlFileName.size() > 0 ? &m_etlFileName[0] : nullptr;
            logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME;

            m_handle = OpenTraceW(&logFile);

            if (m_handle != INVALID_PROCESSTRACE_HANDLE)
            {
                queryBasicInfo();
                queryAdvancedInfo();
            }

            return m_handle != NULL;
        }

        void queryBasicInfo()
        {
            if (m_handle != INVALID_PROCESSTRACE_HANDLE)
                ETWLibUtil::GetSessionInformation(m_traceHandle, m_info);
        }

        void queryAdvancedInfo()
        {
            if (m_handle != INVALID_PROCESSTRACE_HANDLE)
            {
                GUID guids[64];
                ULONG returnedSize = 0;
                ULONG status = ::TraceQueryInformation(m_traceHandle, TraceGuidQueryProcess, guids, sizeof(GUID) * 64, &returnedSize);
            }
        }

        bool closeSession()
        {
            if (m_handle != INVALID_PROCESSTRACE_HANDLE)
            {
                ULONG status = CloseTrace(m_handle);
                return status == ERROR_SUCCESS;
            }

            m_handle = INVALID_PROCESSTRACE_HANDLE;
            return false;
        }
    protected:
        SessionInfo m_info;
        TRACEHANDLE m_handle;
        TRACEHANDLE m_traceHandle;
        std::wstring m_sessionName;
        std::wstring m_etlFileName;
    };

    ETWSessionConsumer::ETWSessionConsumer(ULONG64 traceHandle, std::wstring sessionName, std::wstring etlFileName)
    {
        m_pImp = new ETWSessionConsumerImp(traceHandle, sessionName, etlFileName);
    }

    ETWSessionConsumer::~ETWSessionConsumer()
    {
        delete m_pImp;
        m_pImp = nullptr;
    }

    SessionInfo& ETWSessionConsumer::SessionInfomation()
    {
        if (m_pImp)
            return m_pImp->information();
        return SessionInfo();
    }

    void QueryAllSessions(std::vector<SessionInfo>& infos)
    {
        for (TRACEHANDLE trace = 0; trace < MAX_SESSION_COUNT; trace++)
        {
            SessionInfo info;
            if (ETWLibUtil::GetSessionInformation(trace, info))
                infos.push_back(info);
        }
    }

}