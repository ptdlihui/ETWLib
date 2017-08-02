#include "etwproviders.h"
#include <tdh.h>
#include <windows.h>



namespace ETWLib
{
    void
    ETWProviders::init()
    {
        ULONG bufferSize = 0u;
        std::vector<unsigned char> buffer;

        TDHSTATUS ret = ERROR_SUCCESS;

        do {
            buffer.resize(bufferSize);
            ret = TdhEnumerateProviders(reinterpret_cast<PPROVIDER_ENUMERATION_INFO>(buffer.data()), &bufferSize);
        } while (ret == ERROR_INSUFFICIENT_BUFFER);

        const PPROVIDER_ENUMERATION_INFO pProviders = reinterpret_cast<const PPROVIDER_ENUMERATION_INFO>(buffer.data());

        for (int i = 0; i < pProviders->NumberOfProviders; i++)
        {
            GUID guid = pProviders->TraceProviderInfoArray[i].ProviderGuid;
            std::wstring name(reinterpret_cast<const wchar_t*>(buffer.data() + pProviders->TraceProviderInfoArray[i].ProviderNameOffset));
            m_name2Guid.emplace(name, guid);
        }

    }

    GUID
    ETWProviders::UserModeProvider(std::wstring name)
    {
        return m_name2Guid[name];
    }

    GUID
    ETWProviders::KernelModeProvider(KernelModeProviderFlag flag)
    {
        switch (flag)
        {
        case ALPC: return ALPCGuid;
        case DiskIO: return DiskIoGuid;
        case EventTraceConfig: return EventTraceConfigGuid;
        case FileIO: return FileIoGuid;
        case Image: return ImageLoadGuid;
        case PageFaultV2: return PageFaultGuid;
        case ProfInfo: return PerfInfoGuid;
        case Process: return ProcessGuid;
        case Registry: return RegistryGuid;
        case SplitIO: return SplitIoGuid;
        case TCPIP: return TcpIpGuid;
        case Thread: return ThreadGuid;
        case Udplp: return UdpIpGuid;
        case Heap: return HeapGuid;
        case Kernel: return KernelGuid;
        default:
            return GUID();
        }

        return GUID();
    }

	std::vector<std::wstring> ETWProviders::AllProviders_Name() 
	{
		std::vector<std::wstring> allproviders_name;
		for(auto itor = m_name2Guid.begin();itor != m_name2Guid.end();++itor)
		{
			allproviders_name.push_back(itor->first);
		}
		return allproviders_name;
	}
};