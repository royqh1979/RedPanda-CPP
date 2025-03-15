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
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// C and POSIX
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <limits.h>

// ours
#include "argparser.hpp"

namespace chrono = std::chrono;
namespace fs = std::filesystem;
using std::chrono::high_resolution_clock;
using std::chrono::steady_clock;
using std::string;
using std::string_view;
using std::vector;

using AP = ArgParser<char>;
#define _(KEY) AP::GetText(#KEY, AP::TextItem::KEY).data()

void PrintSplitLine(FILE *stream)
{
    int width = 20;
//    struct winsize ws;
//    if (ioctl(fileno(stream), TIOCGWINSZ, &ws) == 0)
//        width = ws.ws_col;
    fputc('\n', stream);
    for (int i = 0; i < width; i++)
        fputc('-', stream);
    fputc('\n', stream);
}

void PrintCApiError(const char *function)
{
    int errorCode = errno;
    fprintf(stderr, "%s failed with error code %d: %s\n", function, errorCode, strerror(errorCode));
}

void ClearStdinBuffer()
{
    tcflush(fileno(stdin), TCIFLUSH);
}

void PauseExit(int exitcode)
{
    ClearStdinBuffer();

    printf("\n");
    printf("%s", _(EXIT));
    fflush(stdout);

    // set console to raw mode so we can read a single key
    struct termios termios, saved;
    int getResult;
    if ((getResult = tcgetattr(fileno(stdin), &termios)) == 0) {
        saved = termios;
        cfmakeraw(&termios);
        tcsetattr(fileno(stdin), TCSANOW, &termios);
    }

    getchar();

    // restore console mode, in case someone run it in existing terminal
    if (getResult == 0)
        tcsetattr(fileno(stdin), TCSANOW, &saved);

    exit(exitcode);
}

int ExecuteCommand(string_view prog, const vector<string_view> &args, const string &redirectedInputFile, struct rusage *usage)
{
    string basename = fs::path(prog).filename().string();
    vector<const char *> argv = {basename.data()};
    for (string_view arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        if (!redirectedInputFile.empty()) {
            int fd = open(redirectedInputFile.c_str(), O_RDONLY);
            if (fd < 0) {
                PrintCApiError("open");
                fprintf(stderr, "Failed to opening file for redirection.\n");
                exit(-1);
            }
            int newfd = dup2(fd, STDIN_FILENO);
            if (newfd < 0) {
                PrintCApiError("dup2");
                fprintf(stderr, "Failed to redirect stdin.\n");
                exit(-1);
            }
            close(fd);
        }

        execv(prog.data(), (char **)argv.data());

        PrintCApiError("execv");
        if (!args.empty()) {
            fprintf(stderr, "Failed to start program %s with arguments:\n", prog.data());
            for (size_t i = 0; i < args.size(); ++i)
                fprintf(stderr, "  [%zu] = %s\n", i + 1, args[i].data());
        } else {
            fprintf(stderr, "Failed to start program %s without argument.\n", prog.data());
        }
        char buf[PATH_MAX];
        char *cwd = getcwd(buf, PATH_MAX);
        if (cwd)
            fprintf(stderr, "working directory = %s\n", cwd);
        else
            PrintCApiError("getcwd");
        exit(-1);
    } else {
        int status;
        pid_t w;
        w = wait4(pid, &status, WUNTRACED | WCONTINUED, usage);
        if (w==-1) {
            PrintCApiError("wait4");
            PauseExit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return status;
        }
    }
    return 0;
}

chrono::microseconds TimevalToDuration(const struct timeval &tv)
{
    return chrono::seconds(tv.tv_sec) + chrono::microseconds(tv.tv_usec);
}

template <typename Duration>
double DurationToSeconds(const Duration &d)
{
    return 1.0 * d.count() * Duration::period::num / Duration::period::den;
}

int main(int argc, char** argv){
    try {
        AP::gArgs = ArgParser<char>::ParseArgs(argv);
    } catch (string &s) {
        fprintf(stderr, "Parse argument error: %s\n", s.c_str());
        PrintSplitLine(stderr);
        fprintf(stderr, "%s", ArgParser<char>::HelpMessage().c_str());
        PauseExit(EXIT_FAILURE);
    }

    if (AP::gArgs.redirectInput == "-") {
        // this should not happen:
        // console pauser is expected to attach to a newly created tty, QProcess pipe does not work
        fprintf(stderr, "Sorry, not implemented: stdin redirected to pipe.");
        PauseExit(EXIT_FAILURE);
    } else {
        ClearStdinBuffer();
    }

    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    int fd_shm = -1;

    if (!AP::gArgs.sharedMemory.empty()) {
        fd_shm = shm_open(AP::gArgs.sharedMemory.c_str(), O_RDWR,S_IRWXU);
        if (fd_shm == -1) {
            //todo: handle error
            PrintCApiError("shm_open");
        } else {
            // `ftruncate` has already done in RedPandaIDE
            pBuf = (char*)mmap(NULL,BUF_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm,0);
            if (pBuf == MAP_FAILED) {
                PrintCApiError("mmap");
                pBuf = nullptr;
            }
        }
    }

    // Save starting timestamp
    auto starttime = steady_clock::now();

    // Execute the command
    struct rusage usage{};
    int returnvalue = ExecuteCommand(AP::gArgs.program, AP::gArgs.args, AP::gArgs.redirectInput, &usage);

    // Get ending timestamp
    auto endtime = steady_clock::now();
    auto difftime = endtime - starttime;
    double realSeconds = DurationToSeconds(difftime);
    auto cpuTime = TimevalToDuration(usage.ru_utime) + TimevalToDuration(usage.ru_stime);
    double cpuMilliSeconds = DurationToSeconds(cpuTime) * 1000;

    if (pBuf) {
        strcpy(pBuf,"FINISHED");
        munmap(pBuf,BUF_SIZE);
    }
    if (fd_shm!=-1) {
        shm_unlink(AP::gArgs.sharedMemory.c_str());
    }

    // Done? Print return value of executed program
    PrintSplitLine(stdout);
    printf("%s %.4g s. ", _(USAGE_HEADER), realSeconds);
    printf("%s: %d; ", _(USAGE_RETURN_VALUE), returnvalue);
    printf("%s: %.4g ms; ", _(USAGE_CPU_TIME), cpuMilliSeconds);
    printf("%s: %ld KiB.\n", _(USAGE_MEMORY), usage.ru_maxrss);
    if (AP::gArgs.pauseConsole)
        PauseExit(returnvalue);
    return 0;
}

