#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

DWORD GetServiceProcessId(const char* serviceName) {
    SC_HANDLE scmHandle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (scmHandle == NULL) {
        return 0;
    }

    SC_HANDLE serviceHandle = OpenServiceA(scmHandle, serviceName, SERVICE_QUERY_STATUS);
    if (serviceHandle == NULL) {
        CloseServiceHandle(scmHandle);
        return 0;
    }

    SERVICE_STATUS_PROCESS status;
    DWORD bytesNeeded;
    if (!QueryServiceStatusEx(serviceHandle, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded)) {
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return 0;
    }

    DWORD processId = status.dwProcessId;
    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);

    return processId;
}

HANDLE GetProcessHandle(DWORD pid) {
    // Use PROCESS_ALL_ACCESS to ensure the process handle is sufficient
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
    }
    return hProcess;
}

void StartTrustedInstallerService() {
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwBytesNeeded;
    BOOL bResult = FALSE;

    // Open a handle to the Service Control Manager
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        printf("OpenSCManager failed: %u\n", GetLastError());
        return;
    }

    // Open a handle to the TrustedInstaller service
    hService = OpenService(hSCManager, TEXT("TrustedInstaller"), SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);
    if (hService == NULL) {
        printf("OpenService failed: %u\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return;
    }

    // Query the service status
    bResult = QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
    if (!bResult) {
        printf("QueryServiceStatusEx failed: %u\n", GetLastError());
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return;
    }

    // If the service is running, stop it
    if (ssp.dwCurrentState == SERVICE_RUNNING) {
        printf("Stopping the TrustedInstaller service...\n");
        if (!ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
            printf("ControlService failed: %u\n", GetLastError());
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return;
        }

        // Wait for the service to stop
        printf("Waiting for the TrustedInstaller service to stop...\n");
        Sleep(3000);  // Wait for 3 seconds; adjust as needed for your environment

        // Check the service status again
        bResult = QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
        if (!bResult) {
            printf("QueryServiceStatusEx failed: %u\n", GetLastError());
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return;
        }
        if (ssp.dwCurrentState != SERVICE_STOPPED) {
            printf("Service did not stop in a timely manner.\n");
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return;
        }
    }

    // Start the TrustedInstaller service
    printf("Starting the TrustedInstaller service...\n");
    bResult = StartService(hService, 0, NULL);
    if (!bResult) {
        printf("StartService failed: %u\n", GetLastError());
    }
    else {
        printf("TrustedInstaller service started successfully.\n");
    }

    // Clean up
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}

int CreateChildProcess(HANDLE parentHandle) {
    STARTUPINFOEXA si;
    PROCESS_INFORMATION pi;
    SIZE_T size;

    ZeroMemory(&si, sizeof(STARTUPINFOEXA));
    si.StartupInfo.cb = sizeof(STARTUPINFOEXA);

    // Initialize the attribute list for the parent process
    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, size);
    InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);

    // Set the parent process attribute
    UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &parentHandle, sizeof(HANDLE), NULL, NULL);

    // Create the child process (pwsh.exe in a new console window)
    BOOL success = CreateProcessA(
        NULL,                    // No module name (use command line)
        "pwsh.exe",              // Command line
        NULL,                    // Process handle not inheritable
        NULL,                    // Thread handle not inheritable
        FALSE,                   // Set handle inheritance to FALSE
        EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE,  // Creation flags
        NULL,                    // Use parent's environment block
        NULL,                    // Use parent's starting directory 
        &si.StartupInfo,         // Pointer to STARTUPINFOEXA structure
        &pi                      // Pointer to PROCESS_INFORMATION structure
    );

    if (!success) {
        printf("CreateProcess failed. Error: %lu\n", GetLastError());
        DeleteProcThreadAttributeList(si.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
        return 1;
    }

    // Close handles to the newly created process and its main thread
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Clean up the attribute list
    DeleteProcThreadAttributeList(si.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, si.lpAttributeList);

    return 0;
}
