#include <windows.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <tchar.h>

#include <iostream>
#include <vector>

using namespace std;

bool EnablePrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
        std::cerr << "LookupPrivilegeValue error: " << GetLastError() << std::endl;
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;
    cout << tp.Privileges[0].Attributes << endl;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        std::cerr << "AdjustTokenPrivileges error: " << GetLastError() << std::endl;
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        std::cerr << "The token does not have the specified privilege." << std::endl;
        return false;
    }

    return true;
}

int getPID(const std::string& name)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (!Process32First(snapshot, &entry)) return NULL;

    do
    {
        if (strcmp(entry.szExeFile, name.c_str()) == 0)
        {
            CloseHandle(snapshot);
            return entry.th32ProcessID;
        }
    } while (Process32Next(snapshot, &entry));

    CloseHandle(snapshot);
    return NULL;
}

DWORD64 GetModule(HANDLE hProcess, const std::string& name)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                std::string modName = szModName;
                if (modName.find(name) != std::string::npos)
                {
                    return (DWORD64)hMods[i];
                }
            }
        }
    }
    return NULL;
}

std::vector<HANDLE> GetHandle(DWORD pid)
{
    std::vector<HANDLE> processHandles;
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (processHandle == NULL) {
        std::cerr << "OpenProcess error: " << GetLastError() << std::endl;
        return processHandles;
    }
    processHandles.push_back(processHandle);
    return processHandles;
}

struct EnumWindowParam {
    std::wstring windowName;
    std::vector<HANDLE> handles;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    TCHAR windowTitle[256];
    EnumWindowParam* param = (EnumWindowParam*)(lParam);
    GetWindowText(hwnd, windowTitle, sizeof(windowTitle) / sizeof(TCHAR));
    
#ifdef _UNICODE
    std::wstring windowTitleStr = windowTitle;
#else
    std::wstring windowTitleStr = std::wstring(windowTitle, windowTitle + _tcslen(windowTitle));
#endif

    // if (windowTitleStr == param->windowName) {
    if (windowTitleStr.find(param->windowName) != std::wstring::npos) {
        std::wcout << windowTitleStr << endl;
        DWORD processID;
        GetWindowThreadProcessId(hwnd, &processID);
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
        if (hProcess != NULL) {
            param->handles.push_back(hProcess);
        }
    }
    return TRUE; // ����ö�ٴ���
}

std::vector<HANDLE> GetHandle(LPCTSTR windowName) {
    EnumWindowParam param;
    param.windowName = std::wstring(windowName, windowName + _tcslen(windowName));
    EnumWindows(EnumWindowsProc, (LPARAM)&param);
    return param.handles;
}

std::vector<HANDLE> GetHandle(std::wstring windowName) {
    EnumWindowParam param;
    param.windowName = windowName;
    EnumWindows(EnumWindowsProc, (LPARAM)&param);
    return param.handles;
}

int main() {
    HANDLE hToken;
    DWORD processID = 23304; // Ŀ����̵�PID���������Ϊ1234������Ҫ�滻��ʵ�ʵ�PID
    std::vector<HANDLE> processHandles;
    SIZE_T bytesWritten;
    // BYTE buffer[] = { 0x90, 0x90, 0x90 }; // NOPָ��
    BYTE buffer[] = { 0, 0, 0, 0 };
    constexpr const char* processName = "target.exe";

    // �򿪵�ǰ���̵�����
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "OpenProcessToken error: " << GetLastError() << std::endl;
        return 1;
    }

    // ����SeDebugPrivilegeȨ��
    if (!EnablePrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
        CloseHandle(hToken);
        std::cerr << "��Ȩʧ��" << std::endl;
        return 1;
    }

    // ��Ŀ�����
    // processHandles = GetHandle(processID);
    processHandles = GetHandle(processName);
    if (processHandles.size() == 0) {
        CloseHandle(hToken);
        std::cerr << "cannot find process " << processName << std::endl;
        return 1;
    }

    for (auto handle : processHandles) {

        DWORD64 baseAddress = GetModule(handle, processName);
        std::cout << "Base Address: " << std::hex << std::uppercase << "0x" << baseAddress << "\n";

        // ��������֪��Ҫд����ڴ��ַ
        // uint64_t baseAddress = 0x14000C170;
        // uint64_t baseAddress = 0x14000159A;
        // uint64_t BASE = 0x140000000;
        DWORD64 print_func_offset = 0x159A;

        // д������ڴ�
        /*
        // result = WriteProcessMemory(handle, (LPVOID)(baseAddress - BASE), buffer, sizeof(buffer), &bytesWritten);
        result = WriteProcessMemory(handle, (LPVOID)(baseAddress + print_func_offset), buffer, sizeof(buffer), &bytesWritten);
        if (result == FALSE) {
            std::cerr << "WriteProcessMemory error: " << GetLastError() << std::endl;
        }
        else {
            std::cout << "Wrote " << bytesWritten << " bytes." << std::endl;
        }
        */


        SIZE_T bytesRead;
        uint8_t buffer[7] = { 0 };

        // ��ȡ�ڴ�����
        BOOL rpmResult = ReadProcessMemory(handle, (LPVOID)(baseAddress + print_func_offset), buffer, sizeof(buffer), &bytesRead);

        if (rpmResult == FALSE) {
            std::cerr << "��ȡ�ڴ�ʧ�ܣ��������: " << GetLastError() << std::endl;
        }
        else {
            std::cout << "��ȡ�ڴ�ɹ�������ȡ " << bytesRead << " �ֽ����ݡ�" << std::endl;
            // ������������Ҫ���Ĵ������������ȡ������
            for (SIZE_T i = 0; i < bytesRead; ++i) {
                std::cout << std::hex << static_cast<uint32_t>(buffer[i]) << " ";
            }
            std::cout << std::endl;
        }

        /*
        HMODULE hMods[1024];
        DWORD cbNeeded;

        // ö�ٽ����е�����ģ��
        if (EnumProcessModules(handle, hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                char szModName[MAX_PATH];
                // ��ȡģ�������
                if (GetModuleBaseNameA(handle, hMods[i], szModName, sizeof(szModName)) > 0) {
                    // ���ģ������ƺͻ���ַ
                    std::cout << "ģ������: " << szModName << ", ����ַ: " << std::hex << (uintptr_t)hMods[i] << std::endl;
                }
            }
        }
        */

        // �رվ��
        CloseHandle(handle);
    }
    CloseHandle(hToken);
    getchar();

    return 0;
}
