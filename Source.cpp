#include <iostream>
#include <windows.h>
#include <lm.h>
#include <string>
#include <vector>
#include <filesystem>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "mpr.lib")

using namespace std;

// ��������� ��� �������� ������ � ��������
struct ServerInfo {
    wstring name;
};

// ������� ��� ��������� ������ ��������� �����
vector<ServerInfo> getRemoteMachines() {
    vector<ServerInfo> servers;

    PSERVER_INFO_100 pBuf = NULL;
    PSERVER_INFO_100 pTmpBuf;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;
    NET_API_STATUS nStatus;
    LPCWSTR pszServerName = NULL;  // NULL ��� ���������� ������
    DWORD dwLevel = 100;  // ������� 100, ����� �������� ������ ����� ��������
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwServerType = SV_TYPE_WORKSTATION | SV_TYPE_SERVER;  // ���� �������� (������� ������� � �������)

    nStatus = NetServerEnum(
        pszServerName,
        dwLevel,
        (LPBYTE*)&pBuf,
        dwPrefMaxLen,
        &dwEntriesRead,
        &dwTotalEntries,
        dwServerType,
        NULL,
        &dwResumeHandle
    );

    if ((nStatus == NERR_Success || nStatus == ERROR_MORE_DATA) && pBuf != NULL) {
        pTmpBuf = pBuf;

        for (DWORD i = 0; i < dwEntriesRead; i++) {
            if (pTmpBuf == NULL) {
                break;
            }
            wstring serverName = L"\\\\" + wstring(pTmpBuf->sv100_name);
            servers.push_back({ serverName });
            pTmpBuf++;
        }
    }
    else {
        wcout << L"Failed to enumerate servers. Error: " << nStatus << endl;
    }

    if (pBuf != NULL) {
        NetApiBufferFree(pBuf);
    }

    return servers;
}

// ������� ��� ������ ��������� ������
wstring chooseRemoteMachine(const vector<ServerInfo>& servers) {
    int choice;
    wcout << L"Select remote machine:" << endl;
    for (size_t i = 0; i < servers.size(); i++) {
        wcout << i + 1 << L". " << servers[i].name << endl;
    }
    wcout << L"Enter number: ";
    cin >> choice;

    if (choice > 0 && choice <= servers.size()) {
        return servers[choice - 1].name;
    }
    else {
        wcout << L"Invalid choice, defaulting to local machine." << endl;
        return L"";
    }
}


// ������� ��� ������������ ����� ��������
void enumNetworkShares(const wchar_t* serverName) {
    PSHARE_INFO_502 pBuf = NULL;

    DWORD dwEntriesRead = 0, dwTotalEntries = 0, dwResumeHandle = 0;
    NET_API_STATUS nStatus;

    nStatus = NetShareEnum(
        const_cast<LPWSTR>(serverName),
        502,
        (LPBYTE*)&pBuf,
        MAX_PREFERRED_LENGTH,
        &dwEntriesRead,
        &dwTotalEntries,
        &dwResumeHandle
    );

    if (nStatus == NERR_Success || nStatus == ERROR_MORE_DATA) {
        PSHARE_INFO_502 pTmpBuf = pBuf;
        for (DWORD i = 0; i < dwEntriesRead; i++) {
            if (pTmpBuf == NULL) break;
            wcout << L"Share name: " << pTmpBuf->shi502_netname << endl;
            wcout << L"Path: " << pTmpBuf->shi502_path << endl;
            wcout << L"--------------------------------------" << endl;
            pTmpBuf++;
        }
    }
    else {
        wcout << L"Error: " << nStatus << endl;
    }

    if (pBuf != NULL) {
        NetApiBufferFree(pBuf);
    }
}

bool createNetworkShare(const wchar_t* serverName, const std::wstring& shareName, const std::wstring& path, const std::wstring& description) {
    SHARE_INFO_502 shareInfo;
    DWORD result;

    ZeroMemory(&shareInfo, sizeof(shareInfo));
    shareInfo.shi502_netname = const_cast<LPWSTR>(shareName.c_str());  // ��� ������ �������
    shareInfo.shi502_type = STYPE_DISKTREE;  // ��� ������� (����)
    shareInfo.shi502_remark = const_cast<LPWSTR>(description.c_str()); // �������� ������ �������
    shareInfo.shi502_permissions = ACCESS_ALL; // ����� ������� (������ �����)
    shareInfo.shi502_max_uses = -1; // �� ������������ ���������� �����������
    shareInfo.shi502_path = const_cast<LPWSTR>(path.c_str()); // ���� � ����� �����
    shareInfo.shi502_passwd = NULL; // ������ (���� �� ������������)

    result = NetShareAdd(const_cast<LPWSTR>(serverName), 502, (LPBYTE)&shareInfo, NULL);

    if (result == NERR_Success) {
        std::wcout << L"Successfully added share: " << shareName << std::endl;
        return true;
    }
    else {
        std::wcout << L"Failed to add share. Error: " << result << std::endl;
        // ������� ��������� ��������� �� ������
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            result,
            0,
            (LPWSTR)&lpMsgBuf,
            0,
            NULL
        );

        LocalFree(lpMsgBuf);
        return false;
    }
}

int main() {
    vector<ServerInfo> servers = getRemoteMachines();

    int choice;
    wstring serverName;
    wstring username;
    wstring password;
    bool exitFlag = false;



    while (!exitFlag) {
         wcout << L"1. View network shares" << endl;
          wcout << L"2. Create a new network share" << endl;
          wcout << L"0. Exit" << endl;
          wcout << L"Enter your choice: ";
          cin >> choice;

          switch (choice) {
          case 1: {
              serverName = chooseRemoteMachine(servers);
              enumNetworkShares(serverName.empty() ? NULL : serverName.c_str());
              break;
          }
          case 2: {
              serverName = chooseRemoteMachine(servers);
                  wstring shareName, folderPath, description;

                  wcout << L"Enter path for the new folder (e.g., C:\\PathToSharedFolder): ";
                  wcin.ignore();
                  getline(wcin, folderPath);



                  wcout << L"Enter the share name: ";
                  getline(wcin, shareName);

                  wcout << L"Enter a description for the share: ";
                  getline(wcin, description);

                  if (createNetworkShare(serverName.empty() ? NULL : serverName.c_str(),shareName, folderPath, description)) {
                      wcout << L"Network share created successfully." << endl;
                  }
                  else {
                      wcout << L"Failed to create network share." << endl;
                  }
             
              break;
          }
          case 0:
              exitFlag = true;
              break;
          default:
              wcout << L"Invalid choice. Please try again." << endl;
          }
      }



     

    return 0;
}
