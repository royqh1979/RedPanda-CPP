/*
 *  This file is part of Red Panda C++
 *  Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>
using std::wstring;
#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <conio.h>
#include <stdbool.h>
#include <versionhelpers.h>
#include <stdlib.h>

#ifndef WINBOOL
#define WINBOOL BOOL
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_PROCESSED_OUTPUT
#define ENABLE_PROCESSED_OUTPUT 0x0001
#endif
#define MAX_COMMAND_LENGTH 32768
#define MAX_ERROR_LENGTH 2048

#define EXIT_WRONG_ARGUMENTS           -1
#define EXIT_COMMAND_TOO_LONG          -2
#define EXIT_CREATE_JOB_OBJ_FAILED     -3
#define EXIT_SET_JOB_OBJ_INFO_FAILED   -4
#define EXIT_CREATE_PROCESS_FAILED     -5
#define EXIT_ASSGIN_PROCESS_JOB_FAILED -6


enum RunProgramFlag {
    RPF_PAUSE_CONSOLE =     0x0001,
    RPF_REDIRECT_INPUT =    0x0002,
    RPF_ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
};

HANDLE hJob;
bool enableJobControl = IsWindowsXPOrGreater();

bool pauseBeforeExit = false;

LONGLONG GetClockTick() {
    LARGE_INTEGER dummy;
    QueryPerformanceCounter(&dummy);
    return dummy.QuadPart;
}

LONGLONG GetClockFrequency() {
    LARGE_INTEGER dummy;
    QueryPerformanceFrequency(&dummy);
    return dummy.QuadPart;
}

void PrintToStream(HANDLE hStream, const wchar_t *fmt, va_list args)
{
    constexpr size_t buffer_size = 64 * 1024;
    static wchar_t buffer[buffer_size];
    size_t length = _vswprintf(buffer, fmt, args);
    WriteConsoleW(hStream, buffer, length, NULL, NULL);
}

void PrintToStdout(const wchar_t *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PrintToStream(GetStdHandle(STD_OUTPUT_HANDLE), fmt, args);
    va_end(args);
}

void PrintToStderr(const wchar_t *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PrintToStream(GetStdHandle(STD_ERROR_HANDLE), fmt, args);
    va_end(args);
}

void PrintSplitLine(HANDLE hStream)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    int width = 80;
    if (GetConsoleScreenBufferInfo(hStream, &info))
        width = info.dwSize.X;
    wstring content;
    content.push_back(L'\n');
    content.append(width, L'-');
    content.push_back(L'\n');
    WriteConsoleW(hStream, content.c_str(), content.size(), NULL, NULL);
}

void PrintSplitLineToStdout()
{
    PrintSplitLine(GetStdHandle(STD_OUTPUT_HANDLE));
}

void PrintSplitLineToStderr()
{
    PrintSplitLine(GetStdHandle(STD_ERROR_HANDLE));
}

const wchar_t* getMessageFromEnv(const char* name, const wchar_t* defaultValue) {
    constexpr size_t buffer_size = 64 * 1024;
    static wchar_t buffer[buffer_size];
    const char* msg = getenv(name);
    if (msg != NULL) {
        size_t msg_len = strlen(msg);
        size_t newsize = MultiByteToWideChar(CP_ACP,0,msg,msg_len,NULL,0);
        if (newsize>0) {
            int wstrlen = MultiByteToWideChar(CP_ACP,0,msg,msg_len,buffer,newsize);
            buffer[wstrlen]=(wchar_t)0;
            return buffer;
        }
    }
    return defaultValue;
}

wstring GetErrorMessage(DWORD errorCode) {
    wstring result(MAX_ERROR_LENGTH, 0);
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, LANG_USER_DEFAULT, result.data(), result.size(), NULL);

    // Clear newlines at end of wstring
    while (!result.empty() && (result.back() == 0 || iswspace(result.back())))
        result.pop_back();
    return result;
}

void PrintWin32ApiError(const wchar_t *function)
{
    DWORD errorCode = GetLastError();
    PrintToStderr(L"%ls failed with error code %lu: %ls\n", function, errorCode, GetErrorMessage(errorCode).c_str());
}

void PauseExit(int exitcode, bool reInp) {
    if (pauseBeforeExit) {
        HANDLE hInp=NULL;
        if (reInp) {
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = NULL;
            sa.bInheritHandle = TRUE;

            hInp = CreateFileA("CONIN$", GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ , &sa, OPEN_EXISTING, /*FILE_ATTRIBUTE_NORMAL*/0, NULL);
        } else {
            hInp = GetStdHandle(STD_INPUT_HANDLE);
        }
        FlushConsoleInputBuffer(hInp);
        const wchar_t* pause_msg = getMessageFromEnv("RCP_EXIT_MSG",L"Press ANY key to exit...");
        PrintToStdout(L"\n");
        PrintToStdout(pause_msg);
        wchar_t buffer[2];
        DWORD nRead;
        ReadConsoleW(hInp, buffer, 1, &nRead, NULL);
        if (reInp) {
            CloseHandle(hInp);
        }
    }
    exit(exitcode);
}

wstring EscapeArgument(const wstring &arg)
{
    // reduced version of `escapeArgumentImplWindowsCreateProcess` in `RedPandaIDE/utils/escape.cpp`
    // see also https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way .

    if (!arg.empty() &&
        arg.find_first_of(L" \t\n\v\"") == wstring::npos)
        return arg;

    wstring result = L"\"";
    for (auto it = arg.begin(); ; ++it) {
        int nBackSlash = 0;
        while (it != arg.end() && *it == L'\\') {
            ++it;
            ++nBackSlash;
        }
        if (it == arg.end()) {
            // escape all backslashes, but leave the terminating double quote unescaped
            result.append(nBackSlash * 2, L'\\');
            break;
        } else if (*it == '"') {
            // escape all backslashes and the following double quote
            result.append(nBackSlash * 2 + 1, L'\\');
            result.push_back(*it);
        } else {
            // backslashes aren't special here
            result.append(nBackSlash, L'\\');
            result.push_back(*it);
        }
    }
    return result;
}

wstring GetCommand(int argc,wchar_t** argv,bool &reInp, bool &enableVisualTerminalSeq) {
    wstring result;
    int flags = _wtoi(argv[1]);
    reInp = flags & RPF_REDIRECT_INPUT;
    pauseBeforeExit = flags & RPF_PAUSE_CONSOLE;
    enableVisualTerminalSeq = flags & RPF_ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    for(int i = 3;i < argc;i++) {
        result += EscapeArgument(argv[i]);

        // Add a space except for the last argument
        if(i != (argc-1)) {
            result.push_back(L' ');
        }
    }

    if(result.length() > MAX_COMMAND_LENGTH) {
        PrintSplitLineToStderr();
        PrintToStderr(L"Error: Length of command line string is over %d characters\n",MAX_COMMAND_LENGTH);
        PauseExit(EXIT_COMMAND_TOO_LONG,reInp);
    }

    return result;
}

DWORD ExecuteCommand(wstring& command,bool reInp, LONGLONG &peakMemory, LONGLONG &execTime) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    memset(&si,0,sizeof(si));
    si.cb = sizeof(si);
    memset(&pi,0,sizeof(pi));

    DWORD dwCreationFlags = CREATE_BREAKAWAY_FROM_JOB;


    if(!CreateProcessW(NULL, command.data(), NULL, NULL, true, dwCreationFlags, NULL, NULL, &si, &pi)) {
        PrintSplitLineToStderr();
        PrintToStderr(L"Failed to execute \"%ls\":\n", command.c_str());
        PrintWin32ApiError(L"CreateProcessW");
        PauseExit(EXIT_CREATE_PROCESS_FAILED,reInp);
    }
    if (enableJobControl) {
        WINBOOL bSuccess = AssignProcessToJobObject( hJob, pi.hProcess );
        if ( bSuccess == FALSE ) {
            PrintWin32ApiError(L"AssignProcessToJobObject");
            PauseExit(EXIT_ASSGIN_PROCESS_JOB_FAILED,reInp);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE); // Wait for it to finish

    peakMemory = 0;
    PROCESS_MEMORY_COUNTERS counter;
    counter.cb = sizeof(counter);
    if (GetProcessMemoryInfo(pi.hProcess,&counter,
                                 sizeof(counter))){
        peakMemory = counter.PeakPagefileUsage/1024;
    }
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    execTime=0;
    if (GetProcessTimes(pi.hProcess,&creationTime,&exitTime,&kernelTime,&userTime)) {
        execTime=((LONGLONG)kernelTime.dwHighDateTime<<32)
                +((LONGLONG)userTime.dwHighDateTime<<32)
                +(kernelTime.dwLowDateTime)+(userTime.dwLowDateTime);
    }
    DWORD result = 0;
    GetExitCodeProcess(pi.hProcess, &result);
    return result;
}

void EnableVtSequence() {
    DWORD mode;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(hConsole, &mode))
        SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);

    hConsole = GetStdHandle(STD_ERROR_HANDLE);
    if (GetConsoleMode(hConsole, &mode))
        SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
}

int wmain(int argc, wchar_t** argv) {

    const wchar_t *sharedMemoryId;
    // First make sure we aren't going to read nonexistent arrays
    if(argc < 4) {
        PrintSplitLineToStderr();
        PrintToStderr(L"Usage: consolepauser.exe <0|1> <shared_memory_id> <filename> <parameters>\n\n");
        PrintToStderr(L"  1 means the STDIN is redirected by Red Panda C++; 0 means not\n");
        PauseExit(EXIT_WRONG_ARGUMENTS,false);
    }

    // Make us look like the paused program
    SetConsoleTitleW(argv[3]);

    sharedMemoryId = argv[2];

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    bool reInp;
    bool enableVisualTerminalSeq;
    // Then build the to-run application command
    wstring command = GetCommand(argc, argv, reInp, enableVisualTerminalSeq);

    if (enableJobControl) {
        hJob= CreateJobObject( &sa, NULL );

        if ( hJob == NULL ) {
            PrintWin32ApiError(L"CreateJobObject");
            PauseExit(EXIT_CREATE_JOB_OBJ_FAILED,reInp);
        }

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
        memset(&info,0,sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        WINBOOL bSuccess = SetInformationJobObject( hJob, JobObjectExtendedLimitInformation, &info, sizeof( info ) );
        if ( bSuccess == FALSE ) {
            PrintWin32ApiError(L"SetInformationJobObject");
            PauseExit(EXIT_SET_JOB_OBJ_INFO_FAILED,reInp);
        }
    }

    HANDLE hOutput = NULL;
    if (reInp) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        hOutput = CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_WRITE , &sa, OPEN_EXISTING, /*FILE_ATTRIBUTE_NORMAL*/0, NULL);
        SetStdHandle(STD_OUTPUT_HANDLE, hOutput);
        SetStdHandle(STD_ERROR_HANDLE, hOutput);
        freopen("CONOUT$","w+",stdout);
        freopen("CONOUT$","w+",stderr);
    } else {
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    }
    if (enableVisualTerminalSeq) {
        EnableVtSequence();
    }

    HANDLE hSharedMemory=INVALID_HANDLE_VALUE;
    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    hSharedMemory = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        sharedMemoryId
        );
    if (hSharedMemory != NULL)
    {
        pBuf = (char*) MapViewOfFile(hSharedMemory,   // handle to map object
            FILE_MAP_ALL_ACCESS, // read/write permission
            0,
            0,
            BUF_SIZE);
    } else {
        PrintWin32ApiError(L"OpenFileMappingW");
    }

    // Save starting timestamp
    LONGLONG starttime = GetClockTick();

    LONGLONG peakMemory=0;
    LONGLONG execTime=0;
    // Then execute said command
    DWORD returnvalue = ExecuteCommand(command,reInp,peakMemory,execTime);

    // Get ending timestamp
    LONGLONG endtime = GetClockTick();
    double seconds = (endtime - starttime) / (double)GetClockFrequency();
    double execSeconds = (double)execTime/10000;

    if (pBuf) {
        strcpy(pBuf,"FINISHED");
        UnmapViewOfFile(pBuf);
    }
    if (hSharedMemory != NULL && hSharedMemory!=INVALID_HANDLE_VALUE) {
        CloseHandle(hSharedMemory);
    }

    // Done? Print return value of executed program
    PrintSplitLineToStdout();
    const wchar_t* usage_msg = getMessageFromEnv("RCP_USAGE_MSG",
                                                 L"Process exited after %.4f seconds with return value %lu (%.4f ms cpu time, %lld KB mem used).\n");
    PrintToStdout(usage_msg,seconds,returnvalue, execSeconds, peakMemory);
    PrintToStdout(L"\n");
    PauseExit(returnvalue,reInp);
    return 0;
}

