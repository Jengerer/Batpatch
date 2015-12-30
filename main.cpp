#include <algorithm>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include "common.h"
#include "file_helpers.h"
#include "patching.h"

const WCHAR* NewAppId = TEXT("200260");
const WCHAR* OldAppId = TEXT("57400");

bool Run()
{
    std::wcout << TEXT("Welcome to Batpatch: GOTY Edition!\n");
    std::wcout << TEXT("Attempting to read registry to find install path...\n");

    // Find where game is installed...
    if (!(OpenRegistryKey() && ReadInstallPath()))
    {
        std::wcout << TEXT("Failed to read registry to get game install path!\n");
        return false;
    }

    if (CheckPatched())
    {
        std::wcout << TEXT("Looks like you're already patched. Press any key to revert...\n");
        _getch();
        return WriteAppId(NewAppId) && RevertExe();
    }
    else
    {
        std::wcout << TEXT("Ready to apply patch. Press any key to continue...\n");
        _getch();
        return WriteAppId(OldAppId) && ReadInputExe() && ProcessHeader() && ExtractCode() && PatchAssembly() && BackupExe() && WriteOutputExe();
    }
}

int main(int argc, char** argv)
{
    Run();
    std::wcout << TEXT("Press any key to exit.\n");
    delete[] exeBuffer;
    _getch();
}