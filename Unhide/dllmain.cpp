//UNHIDER Dll Code
#define WIN32_LEAN_AND_MEAN

#include "Windows.h"

// Method to Unhide from Screenshare
void setDAForWindows() {
	HWND windowHandle = NULL;
	do {
		windowHandle = FindWindowEx(NULL, windowHandle, NULL, NULL);
		if (windowHandle) {
			// Restore normal display affinity
			SetWindowDisplayAffinity(windowHandle, WDA_NONE);
		}
	} while (windowHandle);
}



BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		setDAForWindows();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return FALSE; 
}

