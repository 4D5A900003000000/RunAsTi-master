#include <windows.h>
#include <stdio.h>
#include <Strsafe.h>
#include "RunAsSystem.h"

int main() {
    const char* serviceName = "TrustedInstaller";

    StartTrustedInstallerService();

    BOOL result = RunAsSystem(L"", L"", &(DWORD){0});
    DWORD pid = GetServiceProcessId(serviceName);

    HANDLE parentHandle = GetProcessHandle(pid);
    if (parentHandle == NULL) {
        return 1;
    }

    if (CreateChildProcess(parentHandle) == 0) {
    }
    else {
    }

    CloseHandle(parentHandle);
    return 0;
}
