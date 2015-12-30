#pragma once

#include <Windows.h>

bool OpenRegistryKey();
bool ReadInstallPath();
bool CheckPatched();
bool BackupExe();
bool ReadInputExe();
bool WriteOutputExe();
bool RevertExe();
bool WriteAppId(const WCHAR* appId);