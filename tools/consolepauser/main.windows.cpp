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
using std::string;
#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <conio.h>
#include <stdbool.h>
#include <versionhelpers.h>

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


string GetErrorMessage() {
    string result(MAX_ERROR_LENGTH,0);
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),&result[0],result.size(),NULL);

    // Clear newlines at end of string
    while (!result.empty() && (result.back() == 0 || isspace(result.back())))
        result.pop_back();
    return result;
}

void PauseExit(int exitcode, bool reInp) {
    if (pauseBeforeExit) {
        HANDLE hInp=NULL;
        if (reInp) {
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = NULL;
            sa.bInheritHandle = TRUE;

            HANDLE hInp = CreateFileA("CONIN$", GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ , &sa, OPEN_EXISTING, /*FILE_ATTRIBUTE_NORMAL*/0, NULL);
                //si.hStdInput = hInp;
            SetStdHandle(STD_INPUT_HANDLE,hInp);
            freopen("CONIN$","r",stdin);
        }
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        fflush(stdin);
        printf("\n");
        printf("Press ANY key to exit...");
        getch();
        if (reInp) {
            CloseHandle(hInp);
        }
    }
    exit(exitcode);
}

string GetCommand(int argc,char** argv,bool &reInp, bool &enableVisualTerminalSeq) {
    string result;
    int flags = atoi(argv[1]);
    reInp = flags & RPF_REDIRECT_INPUT;
    pauseBeforeExit = flags & RPF_PAUSE_CONSOLE;
    enableVisualTerminalSeq = flags & RPF_ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    for(int i = 3;i < argc;i++) {
        // Quote the argument in case the path name contains spaces
        result += string("\"") + string(argv[i]) + string("\"");

        // Add a space except for the last argument
        if(i != (argc-1)) {
            result += string(" ");
        }
    }

    if(result.length() > MAX_COMMAND_LENGTH) {
        printf("\n--------------------------------");
        printf("\nError: Length of command line string is over %d characters\n",MAX_COMMAND_LENGTH);
        PauseExit(EXIT_COMMAND_TOO_LONG,reInp);
    }

    return result;
}

DWORD ExecuteCommand(string& command,bool reInp, LONGLONG &peakMemory, LONGLONG &execTime) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si,0,sizeof(si));
    si.cb = sizeof(si);
    memset(&pi,0,sizeof(pi));

    DWORD dwCreationFlags = CREATE_BREAKAWAY_FROM_JOB;


    if(!CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, true, dwCreationFlags, NULL, NULL, &si, &pi)) {
        printf("\n--------------------------------");
        printf("\nFailed to execute \"%s\":",command.c_str());
        printf("\nError %lu: %s\n",GetLastError(),GetErrorMessage().c_str());
        PauseExit(EXIT_CREATE_PROCESS_FAILED,reInp);
    }
    if (enableJobControl) {
        WINBOOL bSuccess = AssignProcessToJobObject( hJob, pi.hProcess );
        if ( bSuccess == FALSE ) {
            printf( "AssignProcessToJobObject failed: error %lu\n", GetLastError() );
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

int main(int argc, char** argv) {

    const char *sharedMemoryId;
    // First make sure we aren't going to read nonexistent arrays
    if(argc < 4) {
        printf("\n--------------------------------");
        printf("\nUsage: consolepauser.exe <0|1> <shared_memory_id> <filename> <parameters>\n");
        printf("\n 1 means the STDIN is redirected by Red Panda C++; 0 means not\n");
        PauseExit(EXIT_WRONG_ARGUMENTS,false);
    }

    // Make us look like the paused program
    SetConsoleTitleA(argv[3]);

    sharedMemoryId = argv[2];

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    bool reInp;
    bool enableVisualTerminalSeq;
    // Then build the to-run application command
    string command = GetCommand(argc, argv, reInp, enableVisualTerminalSeq);

    if (enableJobControl) {
        hJob= CreateJobObject( &sa, NULL );

        if ( hJob == NULL ) {
            printf( "CreateJobObject failed: error %lu\n", GetLastError() );
            PauseExit(EXIT_CREATE_JOB_OBJ_FAILED,reInp);
        }

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
        memset(&info,0,sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        WINBOOL bSuccess = SetInformationJobObject( hJob, JobObjectExtendedLimitInformation, &info, sizeof( info ) );
        if ( bSuccess == FALSE ) {
            printf( "SetInformationJobObject failed: error %lu\n", GetLastError() );
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
    hSharedMemory = OpenFileMappingA(
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
//    } else {
//        printf("can't open shared memory!\n");
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
    printf("\n--------------------------------");
    printf("\nProcess exited after %.4g seconds with return value %lu (%.4g ms cpu time, %lld KB mem used).\n",seconds,returnvalue, execSeconds, peakMemory);
    PauseExit(returnvalue,reInp);
    return 0;
}

