#include "etwlib.h"
#include <iostream>
#include <locale>
#include <atlbase.h>

using namespace std;

int main(int argc, char *argv[])
{
    char** commandParams = argv + 1;
    int paramNum = argc - 1;

    std::wstring sessionName(L"TraceTest");
    std::wstring etlFileName(L"d:\\Test.etl");
    ETWLib::SessionType sessionType = ETWLib::NormalSession;

    ULONG pids[MAX_PROCESS_NUMBER];
    std::memset(pids, 0, sizeof(pids));

    char** pCurParam = commandParams;
    int i = 0;
    while (i < paramNum)
    {
        std::string p(*pCurParam);

        if (p.at(0) == '-')
        {
            std::string instruct(p.substr(1));
            if (instruct == std::string("name"))
            {
                pCurParam++;
                i++;
                if (i >= paramNum)
                    break;

                CA2W c(*pCurParam);
                sessionName = c;
            }
            else if (instruct == std::string("file"))
            {
                pCurParam++;
                i++;
                if (i >= paramNum)
                    break;

                CA2W c(*pCurParam);
                etlFileName = c;
            }
            else if (instruct == std::string("type"))
            {
                pCurParam++;
                i++;

                if (i >= paramNum)
                    break;

                std::string typeStr(*pCurParam);
                if (typeStr.find("normal") != std::string::npos)
                    sessionType = ETWLib::NormalSession;
                else if (typeStr.find("heap") != std::string::npos)
                    sessionType = ETWLib::HeapSession;
                else if (typeStr.find("base") != std::string::npos)
                    sessionType = ETWLib::BaseSession;
            }
            else if (instruct == std::string("pids"))
            {
                pCurParam++;
                i++;

                int pindex = 0;
                std::string id(*pCurParam);
                while (id.at(0) != '-' && pindex < MAX_PROCESS_NUMBER)
                {
                    pids[pindex++] = std::stoi(id);
                    pCurParam++;
                    i++;

                    if (i >= paramNum)
                        break;

                    id = *pCurParam;


                }

                pCurParam--;
                i--;
            }
        }

        pCurParam++;
        i++;
    }
    
    std::vector<ETWLib::SessionInfo> infos;
    ETWLib::QueryAllSessions(infos);
	const char* privilege[1] = { SE_SYSTEM_PROFILE_NAME };
	ETWLib::GrantPrivilegeA(privilege, 1);

    // Example to kill the exsited session
    for (int i = 0; i < infos.size(); i++)
    {
		//std::wcout << infos[i].SessionName << std::endl;
        if (infos[i].SessionName == sessionName)
        {
            ETWLib::ETWSession attachedSession(infos[i].TraceHandle);
            attachedSession.CloseSession();
        }
    }

    // Example to start the new session
    ETWLib::SessionParameters params(sessionType);

    if (sessionType == ETWLib::Heap)
    {
        params.AddKernelModeProvider(ETWLib::Heap, HeapCreate, true);
        params.AddKernelModeProvider(ETWLib::Heap, HeapDestroy, true);
    }
    else if (sessionType == ETWLib::BaseSession)
    {
        params.AddKernelModeProvider(ETWLib::Kernel, Profile, true);
    }
    else
    {
        params.AddUserModeProvider(L"Microsoft-Windows-Runtime-Graphics", true);
        params.AddUserModeProvider(L"Microsoft-Windows-DXGI", true);
        params.EnableProfilling(true);
    }

    std::memcpy(params.ProcessIDs, pids, sizeof(params.ProcessIDs));



	ETWLib::ETWSession session(sessionName, etlFileName);
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