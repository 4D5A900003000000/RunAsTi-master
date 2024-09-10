#pragma once
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	BOOL RunAsSystem(const WCHAR* cmd, const WCHAR* runFromDir, DWORD* procDoneRetCode);

#ifdef __cplusplus
}
#endif
