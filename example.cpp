#include "etwlib.h"
#include <iostream>
#include <locale>
using namespace std;

int main()
{
    std::vector<ETWLib::SessionInfo> infos;
    ETWLib::QueryAllSessions(infos);
	const char* privilege[1] = { SE_SYSTEM_PROFILE_NAME };
	ETWLib::GrantPrivilegeA(privilege, 1);

    // Example to kill the exsited session
    for (int i = 0; i < infos.size(); i++)
    {
		//std::wcout << infos[i].SessionName << std::endl;
        if (infos[i].SessionName == std::wstring(L"TraceTest"))
        {
            ETWLib::ETWSession attachedSession(infos[i].TraceHandle);
            attachedSession.CloseSession();
        }
    }

    // Example to start the new session
	ETWLib::SessionParameters params;

	//params.AddUserModeProvider(L"Microsoft-Windows-Runtime-Graphics", true);
	//params.AddUserModeProvider(L"Microsoft-Windows-DXGI", true);
	//params.AddUserModeProvider(L"Microsoft-Windows-WebdavClient-LookupServiceTrigger", true);
    //params.AddUserModeProvider(L"Windows Kernel Trace", true);
    DWORD pid = 13660;
    params.AddUserModeProvider(L"Heap Trace Provider", false, ETWLib::LevelVerbose, &pid, 1);
    params.EnableKernelFlags[0] = 0;
    //params.EnableProfilling(true);

	ETWLib::ETWSession session(L"TraceTest", L"d:\\Test.etl");
	session.SetParameters(params);

	bool status = session.StartSession(ETWLib::LogFileMode);//1 stand for success

    if (session.TraceID() > 0)
    {
        ::MessageBoxA(0, "Pending", "Click Me", MB_OK);
        //cout << status << endl;
        session.CloseSession();
    }


	return 0;
}