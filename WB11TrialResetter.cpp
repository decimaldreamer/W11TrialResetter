#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <filesystem>
#include <cstdlib>

bool IsAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return isAdmin;
}

void RequestAdminPrivileges() {
    if (!IsAdmin()) {
        std::wstring appPath = std::filesystem::absolute(__argv[0]).wstring();
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = appPath.c_str();
        sei.hwnd = NULL;
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteEx(&sei)) {
            DWORD error = GetLastError();
            std::cerr << "Failed to request admin privileges. Error code: " << error << "\n";
            exit(error);
        }
        exit(0);
    }
}

void KillProcess(const std::string& processName) {
    std::string command = "taskkill /F /IM " + processName + " > nul 2>&1";
    system(command.c_str());
}

void DeleteRegistryKey(HKEY hKeyRoot, const std::string& subKey) {
    LONG result = RegDeleteTreeA(hKeyRoot, subKey.c_str());
    if (result == ERROR_SUCCESS) {
        std::cout << "Registry key deleted: " << subKey << "\n";
    }
    else if (result == ERROR_FILE_NOT_FOUND) {
        std::cerr << "Registry key not found: " << subKey << "\n";
    }
    else {
        std::cerr << "Failed to delete registry key: " << subKey << ". Error code: " << result << "\n";
    }
}

void DetectAndResetStardockTrial() {
    bool resetTrial = false;
    std::string stardockPaths[] = {
        "C:\\ProgramData\\Stardock\\WindowBlinds",
        "C:\\ProgramData\\Stardock\\Start11"
    };

    for (const auto& path : stardockPaths) {
        std::string processName = (path.find("WindowBlinds") != std::string::npos) ? "WB11Config.exe" : "Start11Config.exe";
        std::string productName = (path.find("WindowBlinds") != std::string::npos) ? "WindowBlinds 11" : "Start11";

        if (std::filesystem::exists(path)) {
            std::cout << productName << " detected.\n";
            KillProcess(processName);
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
            if (!ec) {
                std::cout << productName << " trial license has been reset.\n";
                resetTrial = true;
            }
            else {
                std::cerr << "Error deleting folder: " << path << " - " << ec.message() << "\n";
            }
        }
    }
    DeleteRegistryKey(HKEY_CURRENT_USER, "Software\\Stardock");

    if (!resetTrial) {
        std::cout << "No Stardock installations detected or trial reset was unsuccessful.\n";
    }
    else {
        std::cout << "Done.";
    }
}

int main() {
    RequestAdminPrivileges();
    std::cout << "Detecting Stardock Installations...\n";

    DetectAndResetStardockTrial();

    system("pause");
    return 0;
}
