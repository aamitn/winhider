﻿// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <Windows.h>
#include <TlHelp32.h>
#include <AclAPI.h>

using std::cout;
using std::cerr;
using std::endl;

#ifdef _WIN64
const bool iAm64bit = true;
const std::wstring hideScreenshareDllName{ L"./Hide.dll" };
const std::wstring hideTaskbarDllName{ L"./HideTask.dll" };
const std::wstring unhideScreenshareDllName{ L"./Unhide.dll" };
const std::wstring unhideTaskbarDllName{ L"./UnhideTask.dll" };
#else
const bool iAm64bit = false;
const std::wstring hideScreenshareDllName{ L"./Hide_32bit.dll" };
const std::wstring hideTaskbarDllName{ L"./HideTask_32bit.dll" };
const std::wstring unhideScreenshareDllName{ L"./Unhide_32bit.dll" };
const std::wstring unhideTaskbarDllName{ L"./UnhideTask_32bit.dll" };
#endif


const std::wstring exeName{ L"./Winhider.exe" };
const std::wstring exeName32{ L"./Winhider_32bit.exe" };
const std::wstring title1 = L"\x1b[1m\x1b[36mWinHider\x1b[0m\n";

const std::wstring title = LR"(
 _____                                                       _____ 
( ___ )                                                     ( ___ )
 |   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|   | 
 |   |                                                       |   | 
 |   |  __        __ _         _   _  _      _               |   | 
 |   |  \ \      / /(_) _ __  | | | |(_)  __| |  ___  _ __   |   | 
 |   |   \ \ /\ / / | || '_ \ | |_| || | / _` | / _ \| '__|  |   | 
 |   |    \ V  V /  | || | | ||  _  || || (_| ||  __/| |     |   | 
 |   |     \_/\_/   |_||_| |_||_| |_||_| \__,_| \___||_|     |   | 
 |   |                                                       |   | 
 |___|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|___| 
(_____)                                                     (_____)
)";


void showHelp(const wchar_t* argZero) {
	std::wcout << "WinHider - Hide certain windows from screenshares.\n"
		"\n"
		"Usage: " << argZero << " [--hide | --unhide | --hidetask | --unhidetask  ] PID_OR_PROCESS_NAME ...\n"
		"\n"
		"  -h, --hide      Hide the specified applications from screenshare\n"
		"  -u, --unhide    Unhide the applications specified from screenshare\n"
		"  -h, --hidetask      Hide the specified applications from task bar/switcher \n"
		"  -u, --unhidetask    Unhide the applications specified from task bar/switcher\n"
		"      --help      Show this help menu.\n"
		"\n"
		"  PID_OR_PROCESS_NAME The process id or the process name to hide.\n"
		"\n"
		"Examples:\n"
		<< argZero << " 89203\n"
		<< argZero << " firefox.exe\n"
		<< argZero << " --hide discord.exe obs64.exe\n"
		<< argZero << " --unhide discord.exe obs64.exe\n"
		<< argZero << " --hidetask notepad.exe obs64.exe\n"
		<< argZero << " --unhidetask notepad.exe obs64.exe\n";
}

// Most functions have two types - A (ANSI - old) and W (Unicode - new)

// Why do some functions have Ex? This is a new version of the function that has a different API to accomodate new features,
// but has an Ex to prevent breaking old code.


// Method to show last error code with message
std::string GetLastErrorWithMessage() {
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return "No error message (code 0)";

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);

	// Strip newline characters from the message (optional cleanup)
	if (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
		while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
			message.pop_back();
		}
	}

	return " Error Code " + std::to_string(errorMessageID) + ": " + message;
}


std::unordered_set<int> getPIDsFromProcName(std::wstring& searchTerm) {
	std::unordered_set<int> pids;
	std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::towlower);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot) {
		PROCESSENTRY32 pe32{};
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32)) {
			do {
				std::wstring exeFile{ pe32.szExeFile };
				std::transform(exeFile.begin(), exeFile.end(), exeFile.begin(), ::towlower);
				if (searchTerm == exeFile)
					pids.insert(pe32.th32ProcessID);
			} while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return pids;
}

std::map<std::wstring, std::unordered_set<int>> getProcList() {
	std::map<std::wstring, std::unordered_set<int>> pList;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot) {
		PROCESSENTRY32 pe32{};
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32)) {
			do {
				pList[pe32.szExeFile].insert(pe32.th32ProcessID);
			} while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return pList;
}

bool isValidPID(std::wstring& arg) {
	if (arg.empty()) return false;
	for (auto& c : arg)
		if (!isdigit(c))
			return false;
	return true;
}

// From https://stackoverflow.com/questions/3828835/how-can-we-check-if-a-file-exists-or-not-using-win32-program
// szPath - String Zero-terminated Path
bool FileExists(std::wstring& filePath)
{
	// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesw
	DWORD dwAttrib = GetFileAttributes(filePath.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring getFullFilePath(const std::wstring& filename) {
	wchar_t fullPath[1024];
	// DWORD GetFullPathNameW(
	// 	LPCWSTR lpFileName,    - Relative path
	// 	DWORD   nBufferLength, - Buffer size
	// 	LPWSTR  lpBuffer,      - Buffer
	// 	LPWSTR *lpFilePart     - Pointer to the filename part
	// );
	GetFullPathName(filename.c_str(), 256, fullPath, NULL);
	std::wstring strFullPath{ fullPath };
	if (!FileExists(strFullPath)) {
		std::wcerr << "WARNING:" << strFullPath << " not found.";
		return std::wstring{};
	}
	return strFullPath;
};

// LP  - Pointer
// C   - Constant
// T   - TCHAR (or W - WCHAR)
// STR - String

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
	// Check if DLL exists and then store path
	std::wstring hideScreenshareDllPath{ getFullFilePath(hideScreenshareDllName) };
	std::wstring hideTaskbarDllPath{ getFullFilePath(hideTaskbarDllName) };
	std::wstring unhideScreenshareDllPath{ getFullFilePath(unhideScreenshareDllName) };
	std::wstring unhideTaskbarDllPath{ getFullFilePath(unhideTaskbarDllName) };

	// Check if 32-bit version of the executable exists
	std::wstring x86binPath{ getFullFilePath(exeName32) };
	bool x86verExists = x86binPath.length();

	auto inject = [&](DWORD pid, std::wstring& dllFullPath) -> void {
		if (!dllFullPath.length()) return;
		size_t dllPathLen = (dllFullPath.length() + 1) * sizeof(wchar_t);
		// HANDLE OpenProcess(
		// 	DWORD dwDesiredAccess,
		// 	BOOL  bInheritHandle,       - Whether child processes should inherit this handle
		// 	DWORD dwProcessId		   
		// );						   
		// PROCESS_CREATE_THREAD        - Create thread
		// PROCESS_QUERY_INFORMATION    - To query information about the process like exit code, priority, etc (do we need this?)
		// PROCESS_VM_READ              - To read process memory
		// PROCESS_VM_WRITE             - To write process memory
		// PROCESS_VM_OPERATION         - To perform an operation on the memory (needed for writing)
		if (HANDLE procHandle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, pid); procHandle) {
			cerr << "Opened handle for pid " << (DWORD)pid;
			if (!iAm64bit) cerr << " (32 bit)";
			cerr << endl;

			if (BOOL procIs32bit; iAm64bit and IsWow64Process(procHandle, &procIs32bit) and procIs32bit) {
				if (x86verExists) {
					STARTUPINFO si;
					PROCESS_INFORMATION pi;

					ZeroMemory(&si, sizeof(si));
					si.cb = sizeof(si);
					ZeroMemory(&pi, sizeof(pi));

					std::wstring cliargs;
					cliargs += exeName32;
					cliargs += L" ";
					// Pass the DLL path as an argument
					cliargs += L"\"";
					cliargs += dllFullPath;
					cliargs += L"\" ";
					cliargs += std::to_wstring(pid);

					if (CreateProcess(x86binPath.c_str(), &cliargs[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
						WaitForSingleObject(pi.hProcess, INFINITE);
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
					}
					else cerr << "Unknown error occurred when trying to run 32-bit exe." << endl;
				}
				else std::wcerr << "Cannot hide 32-bit process " << pid << " since " << exeName32 << " is missing." << endl;
				CloseHandle(procHandle);
				return;
			}
			// BOOL GetModuleHandleExW(
			// 	DWORD   dwFlags,        - Some random flags you can pass in
			// 	LPCWSTR lpModuleName,   - Module name
			// 	HMODULE *phModule       - Pointer to module handle if successful
			// );
			if (HMODULE libHandle; GetModuleHandleEx(0, L"kernel32.dll", &libHandle)) {
				// cerr << "Kernel32.dll base address is " << libHandle << endl;
				// FARPROC GetProcAddress(
				// 	HMODULE hModule,    - Module handle
				// 	LPCSTR  lpProcName  - Library/Variable you want the address of
				// );
				if (LPVOID libAddr = GetProcAddress(libHandle, "LoadLibraryW"); libAddr) {
					// cerr << "Library Address at " << libAddr << endl;
					// LPVOID VirtualAllocEx(    
					// 	HANDLE hProcess,         - Process handle
					// 	LPVOID lpAddress,        - The starting address of where we want to start allocating
					// 	SIZE_T dwSize,           - The amount we want to allocate (length + 1 null byte)
					// 	DWORD  flAllocationType, - Allocation type
					// 	DWORD  flProtect         - Protections
					// );
					if (LPVOID mem = VirtualAllocEx(procHandle, NULL, dllPathLen, MEM_COMMIT, PAGE_READWRITE); mem) {
						// BOOL WriteProcessMemory(
						// 	HANDLE  hProcess,
						// 	LPVOID  lpBaseAddress,           - where to start writing from
						// 	LPCVOID lpBuffer,                - what to write
						// 	SIZE_T  nSize,                   - how much to write
						// 	SIZE_T * lpNumberOfBytesWritten  - a pointer to store how many bytes were written
						// );
						if (WriteProcessMemory(procHandle, mem, dllFullPath.c_str(), dllPathLen, NULL)) {
							// HANDLE CreateRemoteThreadEx(
							// 	HANDLE                       hProcess,
							// 	LPSECURITY_ATTRIBUTES        lpThreadAttributes, - Security attributes
							// 	SIZE_T                       dwStackSize,        - Stack size (0 - default)
							// 	LPTHREAD_START_ROUTINE       lpStartAddress,     - What address to start thread
							// 	LPVOID                       lpParameter,        - a parameter to pass to the above function (in our case, dll path)
							// 	DWORD                        dwCreationFlags,    - flags such as whether to start suspended, etc
							// 	LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
							// 	LPDWORD                      lpThreadId
							// );
							// cerr << "Successfully wrote dllFullPath: ";
							// std::wcerr << std::wstring(dllFullPath) << endl;
							if (HANDLE remoteThread = CreateRemoteThreadEx(procHandle, NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(libAddr), mem, 0, NULL, NULL); remoteThread) {
								// If we wanted to wait for the dll thread to return
								// WaitForSingleObject(remoteThread, INFINITE);
								if (CloseHandle(remoteThread) and CloseHandle(procHandle))
									cerr << "Success!" << endl;
								else std::cerr << "Injected Dll, but failed to close handles." << GetLastErrorWithMessage() << std::endl;
							}
							else std::cerr << "Failed to create remote thread." << GetLastErrorWithMessage() << std::endl;
						}
						else std::cerr << "Failed to write to allocated memory." << GetLastErrorWithMessage() << std::endl;
					}
					else std::cerr << "Failed to allocate memory." << GetLastErrorWithMessage() << std::endl;
				}
				else std::cerr << "Failed to get address of LoadLibraryW." << GetLastErrorWithMessage() << std::endl;
			}
			else std::cerr << "Failed to acquire handle on kernel32.dll." << GetLastErrorWithMessage() << std::endl;
		}
		else std::cerr << "Failed to acquire handle on process " << pid << "." << GetLastErrorWithMessage() << std::endl;
		};

		if (argc > 1) {
			// Add new DLL path variables at the top of wmain:
			std::wstring hideScreenshareDllPath{ getFullFilePath(hideScreenshareDllName) };
			std::wstring unhideScreenshareDllPath{ getFullFilePath(unhideScreenshareDllName) };
			std::wstring hideTaskbarDllPath{ getFullFilePath(hideTaskbarDllName) };
			std::wstring unhideTaskbarDllPath{ getFullFilePath(unhideTaskbarDllName) };

			// Default to screenshare hide
			enum class HideType { Screenshare, Taskbar } hideType = HideType::Screenshare;
			bool hide = true;

			for (int i = 1; i < argc; i++) {
				std::wstring arg{ argv[i] };
				std::transform(arg.begin(), arg.end(), arg.begin(), ::towlower);

				if ((arg == L"-h" && argc == 2) || arg == L"--help" || arg == L"/?") {
					showHelp(argv[0]);
					return 0;
				}
				else if (arg == L"-h" || arg == L"--hide") {
					hide = true;
					hideType = HideType::Screenshare;
				}
				else if (arg == L"-u" || arg == L"--unhide") {
					hide = false;
					hideType = HideType::Screenshare;
				}
				else if (arg == L"-ht" || arg == L"--hidetask") {
					hide = true;
					hideType = HideType::Taskbar;
				}
				else if (arg == L"-uht" || arg == L"--unhidetask") {
					hide = false;
					hideType = HideType::Taskbar;
				}
				else if (isValidPID(arg)) {
					std::wstring dllPath;
					if (hideType == HideType::Screenshare)
						dllPath = hide ? hideScreenshareDllPath : unhideScreenshareDllPath;
					else
						dllPath = hide ? hideTaskbarDllPath : unhideTaskbarDllPath;
					inject(stoi(arg), dllPath);
				}
				else {
					auto pids = getPIDsFromProcName(arg);

					// If we find no results, append .exe and try again
					if (pids.empty()) pids = getPIDsFromProcName(arg.append(L".exe"));

					if (pids.empty())
						std::wcerr << L"No process found with the name " << argv[i] << endl;
					for (auto& pid : pids) {
						std::wstring dllPath;
						if (hideType == HideType::Screenshare)
							dllPath = hide ? hideScreenshareDllPath : unhideScreenshareDllPath;
						else
							dllPath = hide ? hideTaskbarDllPath : unhideTaskbarDllPath;
						inject(pid, dllPath);
					}
				}
			}
			return 0;
		}

		// Interactive mode
		std::wcout << title << endl;
		std::wcout << "Hey I'm WinHider, here to make windows invisible from screenshare and taskbar to everyone but you ^_^" << endl;
		std::wcout << "Type `help` to get started." << endl;

		int enterPressed{};
		while (true) {
			std::wstring input;
			cout << "> ";
			getline(std::wcin, input);
			if (input.empty()) {
				if (enterPressed++) {
					cout << "Exiting .. Have a great day!";
					return 0;
				}
				cout << "Press Enter again to exit" << endl;
			}
			else {
				enterPressed = 0;

				auto delimPos = input.find(L" ");
				std::wstring command = input.substr(0, delimPos);

				if (command == L"help" || command == L"`help`") {
					std::cout << "Available commands: \n"
						"\n"
						"  hide PROCESS_ID_OR_NAME        Hides from screenshare\n"
						"  unhide PROCESS_ID_OR_NAME      Unhides from screenshare\n"
						"  hidetask PROCESS_ID_OR_NAME    Hides from taskbar/alt-tab\n"
						"  unhidetask PROCESS_ID_OR_NAME  Unhides from taskbar/alt-tab\n"
						"  list                           Lists all applications\n"
						"  help                           Shows this help menu\n"
						"  exit                           Exit\n"
						"\n"
						"Examples:\n"
						"hide notepad.exe\n"
						"hidetask notepad.exe\n"
						"hidetask 12345\n"
						"unhide discord.exe\n"
						"unhidetask discord.exe\n"
						"unhidetask 54321\n";
				}
				else if (command == L"list") {
					std::wcout << std::setw(35) << std::left << "Process name" << "PID" << endl;
					for (auto& [pName, pIDs] : getProcList()) {
						std::wcout << std::setw(35) << std::left << pName;
						for (auto& pID : pIDs) std::cout << pID << " ";
						cout << endl;
					}
				}
				else if (
					command == L"hide" || command == L"unhide" ||
					command == L"hidetask" || command == L"unhidetask"
					) {
					if (delimPos == std::wstring::npos) {
						std::wcout << "Usage: " << command << " PROCESS_ID_OR_NAME\n";
						continue;
					}
					std::wstring arg = input.substr(delimPos + 1);
					std::wstring dllPath;

					if (command == L"hide")
						dllPath = hideScreenshareDllPath;
					else if (command == L"unhide")
						dllPath = unhideScreenshareDllPath;
					else if (command == L"hidetask")
						dllPath = hideTaskbarDllPath;
					else if (command == L"unhidetask")
						dllPath = unhideTaskbarDllPath;

					if (isValidPID(arg)) {
						inject(stoi(arg), dllPath);
					}
					else {
						auto pids = getPIDsFromProcName(arg);

						// If we find no results, append .exe and try again
						if (pids.empty()) pids = getPIDsFromProcName(arg.append(L".exe"));

						if (pids.empty())
							std::wcerr << L"No process found with the name " << input.substr(delimPos + 1) << endl;
						for (auto& pid : pids)
							inject(pid, dllPath);
					}
				}
				else if (command == L"exit" || command == L"quit") {
					cout << "Exiting .. have a good day!\n";
					return 0;
				}
				else {
					cout << "Invalid command. Type `help` for help." << endl;
				}
			}
		}

	return 0;
}
