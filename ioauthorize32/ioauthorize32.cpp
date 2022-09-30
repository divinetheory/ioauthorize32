// IOAUTHORIZE32.EXE -- by Justin Johnson
// Compile:    Use MSBUILD
// Purpose:    Give direct port I/O access to a user mode process.

#include <iostream>
#include <string>
#include <windows.h>

#define DEVICE_NAME_STRING L"ioauthorize32"
#define IOAUTHORIZE32_CODE CTL_CODE(0x8000,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)

int wmain(int argc, wchar_t* argv[], wchar_t* envp[]) {
	DWORD bytesReturned, inputBuffer, outputBuffer;
	HANDLE hDevice;
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;
	size_t doubleWordSize, startupInfoSize;
	std::wstring commandLine;
	try {
		ZeroMemory(&hDevice, sizeof(HANDLE));
		ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
		startupInfoSize = sizeof(STARTUPINFOW);
		ZeroMemory(&startupInfo, startupInfoSize);
		startupInfo.cb = (DWORD)startupInfoSize;
		if (!argv) {
			throw 0;
		}
		if (argc < 2) {
			throw 1;
		}
		if ((hDevice = CreateFileW(L"\\\\.\\" DEVICE_NAME_STRING, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
			throw 2;
		}
		commandLine = (std::wstring)GetCommandLineW();
		commandLine.erase((size_t)0, wcslen(argv[0]) + (size_t)1);
		if (CreateProcessW(NULL, _wcsdup(commandLine.c_str()), NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, &startupInfo, &processInfo) != 0) {
			bytesReturned = 0;
			inputBuffer = processInfo.dwProcessId;
			outputBuffer = 0;
			doubleWordSize = sizeof(DWORD);
			if (DeviceIoControl(hDevice, IOAUTHORIZE32_CODE, &inputBuffer, (DWORD)doubleWordSize, &outputBuffer, (DWORD)doubleWordSize, &bytesReturned, NULL) != 0) {
				if (outputBuffer == processInfo.dwProcessId) {
					ResumeThread(processInfo.hThread);
				}
				else {
					throw 5;
				}
			}
			else {
				throw 4;
			}
		}
		else {
			throw 3;
		}
	}
	catch (int e0) {
		switch (e0) {
		case 0:
			wprintf(L"\nError: Argument value array is null!\n");
			break;
		case 1:
			wprintf(L"\nError: Must pass a path to a child process as the first argument!\n");
			break;
		case 2:
			wprintf(L"\nError: Failed to open communication with the " DEVICE_NAME_STRING L" device driver!\n");
			break;
		case 3:
			wprintf(L"\nError: Failed to create the child process!\n");
			break;
		case 4:
			wprintf(L"\nError: Failed to communicate with the " DEVICE_NAME_STRING L" device driver!\n");
			break;
		case 5:
			wprintf(L"\nError: Failed to authorize the child process!\n");
			break;
		default:
			wprintf(L"\nError: An unknown error has occured!\n");
			break;
		}
		switch (e0) {
		case 4:
		case 5:
			try {
				if (TerminateProcess(processInfo.hProcess, 0) == 0) {
					throw 0;
				}
			}
			catch (int e1) {
				switch (e1) {
				case 0:
					wprintf(L"\nError: Failed to terminate the child process!\n");
					break;
				default:
					wprintf(L"\nError: An unknown error has occured while attempting to terminate the child process!\n");
					break;
				}
			}
			break;
		default:
			break;
		}
	}
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);
	CloseHandle(startupInfo.hStdInput);
	CloseHandle(startupInfo.hStdOutput);
	CloseHandle(startupInfo.hStdError);
	CloseHandle(hDevice);
	return 0;
}