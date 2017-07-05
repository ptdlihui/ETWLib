#include "etwlib.h"

int main()
{
    const char* privilege[1] = { SE_SYSTEM_PROFILE_NAME };
    ETWLib::GrantPrivilegeA(privilege, 1);
    ETWLib::SessionParameters params;

    params.AddUserModeProvider(L"Microsoft-Windows-Runtime-Graphics", true);
    params.AddUserModeProvider(L"Microsoft-Windows-DXGI", true);
    params.EnableProfilling(true);

    ETWLib::ETWSession session(L"TraceTest", L"Test.etl");
    session.SetParameters(params);

    ULONG status = session.StartSession(ETWLib::LogFileMode);

    Sleep(10 * 1000);

    session.CloseSession();

    return 0;
}