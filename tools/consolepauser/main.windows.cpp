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

// C++
#include <chrono>
#include <string>

// CRT
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

// Win32
#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <versionhelpers.h>
#include <winerror.h>

// ours
#include "argparser.hpp"

namespace chrono = std::chrono;
using std::chrono::high_resolution_clock;
using std::vector;
using std::wstring;
using std::wstring_view;

using hundred_nano = std::ratio<100, 1'000'000'000>;
using win32_filetime_duration = chrono::duration<int64_t, hundred_nano>;

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

namespace WslApi
{
    BOOL (*pWslIsDistributionRegistered)(
        PCWSTR distributionName
    ) = nullptr;
    HRESULT (*pWslRegisterDistribution)(
        _In_ PCWSTR distributionName,
        _In_ PCWSTR tarGzFilename
    ) = nullptr;
    HRESULT (*pWslLaunch)(
        _In_ PCWSTR distributionName,
        _In_opt_ PCWSTR command,
        _In_ BOOL useCurrentWorkingDirectory,
        _In_ HANDLE stdIn,
        _In_ HANDLE stdOut,
        _In_ HANDLE stdErr,
        _Out_ HANDLE *process
    ) = nullptr;

    bool Init()
    {
        // Microsoft say we should detect API set availability before dynamic loading,
        // but we are targetting desktop only, simply load it, avoid dynamic loading too many APIs.
        // (yes, API set detection API `IsApiSetImplemented` requires dynamic loading.)
        HMODULE hModule = LoadLibraryW(L"api-ms-win-wsl-api-l1-1-0.dll");
        if (!hModule)
            return false;

        // if an API set exists, the APIs are all available. further check is not necessary.
        pWslIsDistributionRegistered = (decltype(pWslIsDistributionRegistered))GetProcAddress(hModule, "WslIsDistributionRegistered");
        pWslRegisterDistribution = (decltype(pWslRegisterDistribution))GetProcAddress(hModule, "WslRegisterDistribution");
        pWslLaunch = (decltype(pWslLaunch))GetProcAddress(hModule, "WslLaunch");
        return true;
    }
};

HANDLE hJob;
bool enableJobControl = IsWindowsXPOrGreater();

using AP = ArgParser<wchar_t>;
#define _(KEY) AP::GetText(L"" #KEY, AP::TextItem::KEY).data()

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
//    CONSOLE_SCREEN_BUFFER_INFO info;
    int width = 20;
//    if (GetConsoleScreenBufferInfo(hStream, &info))
//        width = info.dwSize.X;
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
    if (AP::gArgs.pauseConsole) {
        // clean input buffer
        if (reInp) {
            // redirected. stdin does not point to console input, open it explicitly.
            // here we use the same definition as UCRT.
            // UCRT's reference source can be found at Windows SDK, for example:
            // C:\Program Files (x86)\Windows Kits\10\Source\10.0.26100.0\ucrt\conio\initconin.cpp
            HANDLE hInp = CreateFileW(
                L"CONIN$",
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            FlushConsoleInputBuffer(hInp);
            CloseHandle(hInp);
        } else {
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        }

        PrintToStdout(L"\n");
        PrintToStdout(L"%ls", _(EXIT));

        wint_t ch = _getwch();
        if (ch == 0 || ch == 0xE0) {
            // function key or arrow key, the document says should call it twice.
            // however, `system("pause")` doesn't, so we don't do it here.
        }
    }
    exit(exitcode);
}

wstring EscapeArgument(wstring_view arg)
{
    // reduced version of `escapeArgumentImplWindowsCreateProcess` in `RedPandaIDE/utils/escape.cpp`
    // see also https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way .

    if (!arg.empty() &&
        arg.find_first_of(L" \t\n\v\"") == wstring::npos)
        return wstring{arg.data(), arg.size()};

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
    result.push_back('"');
    return result;
}

wstring GetCommand(wstring_view prog, const vector<wstring_view> &args, bool reInp) {
    wstring result = EscapeArgument(prog);
    for (wstring_view arg : args) {
        result.push_back(' ');
        result.append(EscapeArgument(arg));
    }

    if(result.length() > MAX_COMMAND_LENGTH) {
        PrintSplitLineToStderr();
        PrintToStderr(L"Error: Length of command line string is over %d characters\n",MAX_COMMAND_LENGTH);
        PauseExit(EXIT_COMMAND_TOO_LONG,reInp);
    }

    return result;
}

wstring DosToWsl(wstring_view path, bool reInp)
{
    if (path.length() < 2 || path[1] != ':') {
        PrintToStderr(L"Error: program path must be absolute DOS path\n");
        PauseExit(EXIT_WRONG_ARGUMENTS, reInp);
    }
    wchar_t drive = towlower(path[0]);
    if (drive < 'a' || drive > 'z') {
        PrintToStderr(L"Error: program path must be absolute DOS path\n");
        PauseExit(EXIT_WRONG_ARGUMENTS, reInp);
    }
    wstring result = { '/', 'm', 'n', 't', '/', drive };
    for (wchar_t ch : path.substr(2)) {
        if (ch == '\\')
            result.push_back('/');
        else
            result.push_back(ch);
    }
    return result;
}

wstring EscapeWslArgument(wstring_view arg)
{
    wstring result = {'\''};
    for (wchar_t ch : arg) {
        if (ch == '\'') {
            result.push_back('\'');  // terminate single quoting
            result.push_back('\\');  // escape next single quote
            result.push_back('\'');  // the single quote itself
            result.push_back('\'');  // re-start single quoting
        } else {
            result.push_back(ch);
        }
    }
    result.push_back('\'');
    return result;
}

wstring GetWslCommand(wstring_view prog, const vector<wstring_view> &args, bool reInp)
{
    wstring result = EscapeWslArgument(DosToWsl(prog, reInp));
    for (wstring_view arg : args) {
        result.push_back(' ');
        result.append(EscapeWslArgument(arg));
    }
    return result;
}

win32_filetime_duration FiletimeToDuration(const FILETIME &ft)
{
    return win32_filetime_duration(((int64_t)ft.dwHighDateTime << 32) + ft.dwLowDateTime);
}

template <typename Duration>
double DurationToSeconds(const Duration &d)
{
    return 1.0 * d.count() * Duration::period::num / Duration::period::den;
}

DWORD ExecuteCommand(wstring& command,bool reInp, LONGLONG &peakMemory, double &cpuMilliSeconds) {
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
    if (GetProcessTimes(pi.hProcess,&creationTime,&exitTime,&kernelTime,&userTime)) {
        auto cpuDuration = FiletimeToDuration(kernelTime) + FiletimeToDuration(userTime);
        cpuMilliSeconds = DurationToSeconds(cpuDuration) * 1000;
    }
    DWORD result = 0;
    GetExitCodeProcess(pi.hProcess, &result);
    return result;
}

DWORD ExecuteWslCommand(wstring &command, bool reInp, LONGLONG &peakMemory, double &cpuMilliSeconds)
{
    const wchar_t *wslDistro = L"redpanda-cpp-linux-runner-v0";
    const wchar_t *wslDistroName = L"Red Panda C++ Linux Runner V0";
    const wstring_view wslRootfsArchive = L"alpine-minirootfs.tar";

    if (!WslApi::Init()) {
        PrintToStderr(L"Error: WSL is not supported on this system\n");
        PauseExit(EXIT_CREATE_PROCESS_FAILED, reInp);
    }

    HRESULT hr;

    if (!WslApi::pWslIsDistributionRegistered(wslDistro)) {
        PrintToStderr(L"Info: %ls not found\n", wslDistroName);
        wchar_t buffer[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len == 0) {
            PrintWin32ApiError(L"GetModuleFileNameW");
            PauseExit(EXIT_CREATE_PROCESS_FAILED, reInp);
        }
        wstring path(buffer, len);
        while (path.back() != '\\')
            path.pop_back();
        for (wchar_t ch : wslRootfsArchive)
            path.push_back(ch);

        PrintToStderr(L"Info: Try importing from archive: %ls\n", path.c_str());
        hr = WslApi::pWslRegisterDistribution(wslDistro, path.c_str());
        if (FAILED(hr)) {
            PrintSplitLineToStderr();
            PrintToStderr(L"WslRegisterDistribution failed with error 0x%08lx\n", hr);
            PrintToStderr(L"Did you enable WSL? Search \"Turn Windows features on or off\" in Start Menu and enable \"Windows Subsystem for Linux\".");
            PauseExit(EXIT_CREATE_PROCESS_FAILED, reInp);
        }
    }

    HANDLE hProcess;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);

    hr = WslApi::pWslLaunch(wslDistro, command.c_str(), 1, hStdin, hStdout, hStderr, &hProcess);
    if (FAILED(hr)) {
        PrintToStderr(L"WslLaunch failed with error 0x%08lx\n", hr);
        PauseExit(EXIT_CREATE_PROCESS_FAILED, reInp);
    }
    WaitForSingleObject(hProcess, INFINITE); // Wait for it to finish

    peakMemory = 0;
    PROCESS_MEMORY_COUNTERS counter;
    counter.cb = sizeof(counter);
    if (GetProcessMemoryInfo(hProcess,&counter,
                                 sizeof(counter))){
        peakMemory = counter.PeakPagefileUsage/1024;
    }
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    if (GetProcessTimes(hProcess,&creationTime,&exitTime,&kernelTime,&userTime)) {
        auto cpuDuration = FiletimeToDuration(kernelTime) + FiletimeToDuration(userTime);
        cpuMilliSeconds = DurationToSeconds(cpuDuration) * 1000;
    }
    DWORD result = 0;
    GetExitCodeProcess(hProcess, &result);
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

    try {
        AP::gArgs = ArgParser<wchar_t>::ParseArgs(argv);
    } catch (wstring &s) {
        PrintToStderr(L"Parse argument error: %ls\n", s.c_str());
        PrintSplitLineToStderr();
        PrintToStderr(L"%s", ArgParser<wchar_t>::HelpMessage().c_str());
        PauseExit(EXIT_WRONG_ARGUMENTS, true);
    }

    bool reInp = !AP::gArgs.redirectInput.empty();
    if (!AP::gArgs.redirectInput.empty() && AP::gArgs.redirectInput != L"-") {
        PrintToStderr(L"Sorry, not implemented: stdin redirected to file.");
        PauseExit(EXIT_WRONG_ARGUMENTS, true);
    }

    // Make us look like the paused program
    SetConsoleTitleW(AP::gArgs.program.data());

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    // Then build the to-run application command
    wstring command;
    if (AP::gArgs.runInWsl)
        command = GetWslCommand(AP::gArgs.program, AP::gArgs.args, reInp);
    else
        command = GetCommand(AP::gArgs.program, AP::gArgs.args, reInp);

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
    if (AP::gArgs.enableVirtualTerminalSeq) {
        EnableVtSequence();
    }

    HANDLE hSharedMemory=INVALID_HANDLE_VALUE;
    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    if (!AP::gArgs.sharedMemory.empty()) {
        hSharedMemory = OpenFileMappingW(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            AP::gArgs.sharedMemory.c_str()
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
    }

    // Save starting timestamp
    LONGLONG starttime = GetClockTick();

    LONGLONG peakMemory=0;
    double cpuMilliSeconds = 0;
    // Then execute said command
    DWORD returnvalue;
    if (AP::gArgs.runInWsl)
        returnvalue = ExecuteWslCommand(command,reInp,peakMemory,cpuMilliSeconds);
    else
        returnvalue = ExecuteCommand(command,reInp,peakMemory,cpuMilliSeconds);

    // Get ending timestamp
    LONGLONG endtime = GetClockTick();
    double seconds = (endtime - starttime) / (double)GetClockFrequency();

    if (pBuf) {
        strcpy(pBuf,"FINISHED");
        UnmapViewOfFile(pBuf);
    }
    if (hSharedMemory != NULL && hSharedMemory!=INVALID_HANDLE_VALUE) {
        CloseHandle(hSharedMemory);
    }

    // Done? Print return value of executed program
    PrintSplitLineToStdout();
    PrintToStdout(L"%ls %.4f s. ", _(USAGE_HEADER), seconds);
    PrintToStdout(L"%ls: %lu; ", _(USAGE_RETURN_VALUE), returnvalue);
    PrintToStdout(L"%ls: %.4f ms; ", _(USAGE_CPU_TIME), cpuMilliSeconds);
    PrintToStdout(L"%ls: %lld KB.\n", _(USAGE_MEMORY), peakMemory);
    PrintToStdout(L"\n");
    PauseExit(returnvalue,reInp);
    return 0;
}

