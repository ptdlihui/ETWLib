#include "etwlib.h"
#include "etwproviders.h"
#include <evntrace.h>
#include <evntcons.h>
#include <assert.h>
#include <strsafe.h>

#define MAX_SESSION_NAME 1024

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

    SessionParameters::SessionParameters()
    {
        EnableKernelFlags = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD | EVENT_TRACE_FLAG_IMAGE_LOAD;
    }

    void
    SessionParameters::AddKernelModeProvider(KernelModeProviderFlag provider, unsigned char eventid, bool stack)
    {
        KernelModeProviders.push_back({ Providers.KernelModeProvider(provider), eventid, stack});
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
    SessionParameters::AddUserModeProvider(std::wstring name, bool stack)
    {
        UserModeProviders.push_back({ Providers.UserModeProvider(name), 0, stack });
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

        EnableKernelFlags |= EVENT_TRACE_FLAG_PROFILE;

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

            EnableKernelFlags = params.EnableKernelFlags;
            MaxETLFileSize = params.MaxETLFileSize;
            BufferSize = params.BufferSize;
            MinBuffers = params.MinBuffers;
            MaxBuffers = params.MaxBuffers;
        }

        std::wstring m_sessionName;
        std::wstring m_etlFileName;

        std::vector<unsigned char> m_etwPropertiesBuffer;

        TraceMode m_mode;
        TRACEHANDLE m_traceHandle;

        void allocETWProperties(std::wstring sessionName, std::wstring etlFileName)
        {
            m_sessionName = sessionName;
            m_etlFileName = etlFileName;

            m_etwPropertiesBuffer.resize(sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.size() + etlFileName.size() + 2) * sizeof(std::wstring::value_type));
            std::memset(m_etwPropertiesBuffer.data(), 0, m_etwPropertiesBuffer.size());

            PEVENT_TRACE_PROPERTIES pProperties = etwProperties();

            pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            pProperties->LogFileNameOffset = (etlFileName.size() > 0 ? (sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.size() + 1) * sizeof(std::wstring::value_type)) : 0);

            StringCchCopyW((LPWSTR)(m_etwPropertiesBuffer.data() + pProperties->LoggerNameOffset), (sessionName.size() + 1), sessionName.c_str());

            if (pProperties->LogFileNameOffset > 0)
                StringCchCopyW((LPWSTR)(m_etwPropertiesBuffer.data() + pProperties->LogFileNameOffset), (etlFileName.size() + 1), etlFileName.c_str());

            m_mode = (pProperties->LogFileNameOffset > 0) ? LogFileMode : RealTimeMode;
        }

        PEVENT_TRACE_PROPERTIES etwProperties()
        {
            assert(m_etwPropertiesBuffer.size() > sizeof(EVENT_TRACE_PROPERTIES));
            return reinterpret_cast<PEVENT_TRACE_PROPERTIES>(m_etwPropertiesBuffer.data());
        }
    };


    class ETWSessionImp
    {
    public:
        ETWSessionImp(std::wstring sessionName, std::wstring etlFile)
        {
            m_context.allocETWProperties(sessionName, etlFile);
            m_context.m_traceHandle = 0;
        }

        void setParameters(SessionParameters& params)
        {
            m_context.copyFromParameters(params);
        }

        bool startSession(TraceMode mode)
        {
            if (m_context.m_traceHandle != 0)
                return false;

            m_context.m_mode = mode;

            ULONG status = startTrace();
            if (status != ERROR_SUCCESS)
                return false;

            status = enableUserModeProviders();
            if (status != ERROR_SUCCESS)
                return false;

            status = enableKerneProviders();
            if (status != ERROR_SUCCESS)
                return false;

            return true;
        }

        bool closeSession()
        {
            ULONG status = closeTrace();
            return status == ERROR_SUCCESS;
        }

    protected:
        ULONG startTrace()
        {
            PEVENT_TRACE_PROPERTIES pTraceProperties = m_context.etwProperties();

            if (m_context.m_mode == RealTimeMode)
                pTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

            if (m_context.m_mode == LogFileMode)
                pTraceProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_CIRCULAR;

            if (m_context.KernelModeProviders.size() > 0)
                pTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

            pTraceProperties->Wnode.BufferSize = m_context.m_etwPropertiesBuffer.size();
            pTraceProperties->Wnode.ClientContext = 1;
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
            ULONG status = ControlTraceW(m_context.m_traceHandle, NULL, m_context.etwProperties(), EVENT_TRACE_CONTROL_STOP);
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
                ULONG kernelFlags = m_context.EnableKernelFlags;
                status = TraceSetInformation(m_context.m_traceHandle, TraceSystemTraceEnableFlagsInfo, &kernelFlags, sizeof(kernelFlags));
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

                status = EnableTraceEx2(m_context.m_traceHandle, &(instance.guid), EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0, 0, 0, &params);

                if (status != ERROR_SUCCESS)
                    return status;
            }

            return status;
        }
    protected:
        SessionContext m_context;
    };


    ETWSession::ETWSession(std::wstring sessionName, std::wstring etlFile)
    {
        m_pImp = new ETWSessionImp(sessionName, etlFile);
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


    void ConvertPropertiesToInfo(PEVENT_TRACE_PROPERTIES pProperties, SessionInfo& info)
    {
        assert(pProperties);

        info.BufferSize = pProperties->BufferSize;
        info.MaxBuffers = pProperties->MaximumBuffers;
        info.MinBuffers = pProperties->MinimumBuffers;
        info.EnableKernelFlags = pProperties->EnableFlags;
        info.MaxETLFileSize = pProperties->MaximumFileSize;

        info.SessionName = std::wstring((const wchar_t*)((const char*)(pProperties) + pProperties->LoggerNameOffset));
        info.LogFileName = std::wstring((const wchar_t*)((const char*)(pProperties) + pProperties->LogFileNameOffset));
    }


#define MAX_LOG_FILE_PATH_LENGTH 1024
#define MAX_SESSION_NAME_LENGTH 256

    void QueryAllSessions(std::vector<SessionInfo>& infos)
    {
        size_t propertySize = sizeof(EVENT_TRACE_PROPERTIES) + MAX_LOG_FILE_PATH_LENGTH + MAX_SESSION_NAME_LENGTH;
        PEVENT_TRACE_PROPERTIES sessions[MAX_SESSION_COUNT];

        std::vector<unsigned char> buffer;
        buffer.resize(propertySize * MAX_SESSION_COUNT);

        unsigned char* pHeader = buffer.data();
        for (unsigned int i = 0; i < MAX_SESSION_COUNT; i++)
        {
            unsigned char* pCur = pHeader + i * propertySize;
            PEVENT_TRACE_PROPERTIES pProperty = (PEVENT_TRACE_PROPERTIES)(pCur);
            pProperty->Wnode.BufferSize = propertySize;
            pProperty->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            pProperty->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + MAX_SESSION_NAME;
            sessions[i] = pProperty;
        }

        ULONG sessionCount = 0;
        ULONG status = ::QueryAllTracesW(sessions, MAX_SESSION_COUNT, &sessionCount);

        if (status == ERROR_SUCCESS && sessionCount > 0)
        {
            for (unsigned int i = 0; i < sessionCount; i++)
            {
                SessionInfo info;
                ConvertPropertiesToInfo(sessions[i], info);
                infos.push_back(info);
            }
        }
        else
            infos.clear();
    }
}