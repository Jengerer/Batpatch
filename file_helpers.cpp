#include "file_helpers.h"
#include "common.h"
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

// Registry values.
HKEY key;
std::wstring gamePath;

// Paths to files.
const WCHAR* AppIdPath = TEXT("\\Binaries\\Win32\\steam_appid.txt");
const WCHAR* ExePath = TEXT("\\Binaries\\Win32\\BatmanAC.exe");
const WCHAR* BackupExePath = TEXT("\\Binaries\\Win32\\BatmanOG.exe");

// Read data.
char* exeBuffer;
size_t exeSize;

bool OpenRegistryKey()
{
    // Try both 32 and 64-bit.
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 200260"), 0, KEY_READ, &key);
    if (result != ERROR_SUCCESS)
    {
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 200260"), 0, KEY_READ, &key);
        if (result != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            return false;
        }
    }
    return true;
}

bool ReadInstallPath()
{
    WCHAR buffer[MAX_PATH];
    DWORD size = sizeof(buffer);
    LSTATUS status = RegQueryValueExW(key, TEXT("InstallLocation"), 0, NULL, (LPBYTE)buffer, &size);
    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return false;
    }
    gamePath = buffer;
    RegCloseKey(key);
    return true;
}

bool CheckPatched()
{
    // Check if the backup exists.
    const std::wstring FullBackupExePath = gamePath + BackupExePath;
    return std::experimental::filesystem::exists(FullBackupExePath);
}

bool BackupExe()
{
    std::wcout << TEXT("Backing up original executable...\n");

    const std::wstring FullExePath = gamePath + ExePath;
    const std::wstring FullBackupExePath = gamePath + BackupExePath;
    if (!MoveFile(FullExePath.c_str(), FullBackupExePath.c_str()))
    {
        std::wcout << TEXT("Failed to back up ") << FullExePath << TEXT("!\n");
        return false;
    }
    return true;
}

bool RevertExe()
{
    std::wcout << TEXT("Reverting original executable...\n");

    const std::wstring FullExePath = gamePath + ExePath;
    const std::wstring FullBackupExePath = gamePath + BackupExePath;
    
    // Backup should be there since we're in here, so just proceed with delete and move.
    if (std::experimental::filesystem::exists(FullExePath) && !DeleteFile(FullExePath.c_str()))
    {
        std::wcout << TEXT("Failed to delete patched executable. Make sure the game isn't running.\n");
        return false;
    }
    if (!MoveFile(FullBackupExePath.c_str(), FullExePath.c_str()))
    {
        std::wcout << TEXT("Failed to revert original executable's name.\n");
        return false;
    }
    
    std::wcout << TEXT("Reverted executable successfully!\n");
    return true;
}

bool ReadInputExe()
{
    const std::wstring FullInputExePath = gamePath + ExePath;
    std::ifstream file(FullInputExePath, std::ios::binary);
    if (file.bad())
    {
        std::wcout << TEXT("Failed to open ") << FullInputExePath << TEXT(" for reading.\n");
        return false;
    }

    // Get length.
    size_t length = static_cast<size_t>(file.seekg(0, file.end), file.tellg());
    file.clear();
    file.seekg(0);

    // Read it in.
    char* buffer = new char[length];
    file.read(buffer, length);
    if (file.gcount() != length)
    {
        delete[] buffer;
        file.close();
        std::wcout << TEXT("Failed to read ") << FullInputExePath << TEXT("!\n");
        return false;
    }
    file.close();
    exeBuffer = buffer;
    exeSize = length;
    return true;
}

bool WriteOutputExe()
{
    const std::wstring FullOutputExePath = gamePath + ExePath;
    std::ofstream exeOut(FullOutputExePath, std::ios::binary);
    if (exeOut.bad())
    {
        std::wcout << TEXT("Failed to open ") << FullOutputExePath << TEXT(" for writing. May require administrator permissions.\n");
        return false;
    }
    exeOut.write(exeBuffer, exeSize);
    if (exeOut.bad())
    {
        std::wcout << TEXT("Failed to write new executable to ") << FullOutputExePath << TEXT(".\n");
        exeOut.close();
        return false;
    }
    exeOut.close();
    std::wcout << TEXT("Patched executable written successfully!\n");
    std::wcout << TEXT("You must run the executable directly at this location:\n\n");
    std::wcout << FullOutputExePath << TEXT("\n\n");
    std::wcout << TEXT("Do NOT run it through Steam or it'll try to launch the non-GOTY edition!\n");
    std::wcout << TEXT("You may run this executable again to revert the patch. Happy playing!\n");
    return true;
}

bool WriteAppId(const WCHAR* appId)
{
    // Write non-GOTY App ID.
    const std::wstring FullAppIdPath = gamePath + AppIdPath;
    std::wcout << TEXT("Writing App ID ") << appId << TEXT("...\n");

    std::wofstream idFile(FullAppIdPath);
    if (idFile.bad())
    {
        std::wcout << TEXT("Failed to open ") << FullAppIdPath << TEXT(" to change App ID.\n");
        return false;
    }
    idFile << appId;
    if (idFile.bad())
    {
        idFile.close();
        std::wcout << TEXT("Failed to write new App ID to ") << FullAppIdPath << TEXT(".\n");
        return false;
    }
    idFile.close();

    return true;
}