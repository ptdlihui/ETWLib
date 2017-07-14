#include "etwlib.h"
#include <iostream>
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
	params.EnableProfilling(true);

	ETWLib::ETWSession session(L"TraceTest", L"d:\\Test.etl");
	session.SetParameters(params);

	ULONG status = session.StartSession(ETWLib::LogFileMode);
	cout << status << endl;
	for (int i = 0; i < 10; i++)
	{
		Sleep(1 * 1000);
		printf("%d\n", i + 1);
	}

	session.CloseSession();


	return 0;
}