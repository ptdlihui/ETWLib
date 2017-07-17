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

	params.AddUserModeProvider(L"Microsoft-Windows-Runtime-Graphics", true);
	params.AddUserModeProvider(L"Microsoft-Windows-DXGI", true);
	//params.AddUserModeProvider(L"Microsoft-Windows-WebdavClient-LookupServiceTrigger", true);
	params.AddUserModeProvider(L"Microsoft-Windows-ApplicationExperience-LookupServiceTrigger", true);
	params.EnableProfilling(true);

	ETWLib::ETWSession session(L"TraceTest", L"d:\\Test.etl");
	session.SetParameters(params);

	bool status = session.StartSession(ETWLib::LogFileMode);//1 stand for success
	//cout << status << endl;
	session.CloseSession();


	return 0;
}