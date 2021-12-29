/*
 *  This file is part of Red Panda Dev-C++ 7
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
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <chrono>
#include <sys/wait.h>
#define MAX_COMMAND_LENGTH 32768
#define MAX_ERROR_LENGTH 2048

enum RunProgramFlag {
    RPF_PAUSE_CONSOLE =     0x0001,
    RPF_REDIRECT_INPUT =    0x0002
};


void PauseExit(int exitcode, bool reInp) {
    if (reInp) {
        freopen("/dev/tty","r",stdin);
    }
    fflush(stdin);
    printf("\n");
    printf("Press ANY key to exit...");
    getchar();
    exit(exitcode);
}

string GetCommand(int argc,char** argv,bool &reInp,bool &pauseAfterExit) {
    string result;
    int flags = atoi(argv[1]);
    reInp = flags & RPF_REDIRECT_INPUT;
    pauseAfterExit = flags & RPF_PAUSE_CONSOLE;
    for(int i = 2;i < argc;i++) {
        //result += string("\"") + string(argv[i]) + string("\"");
        std::string s(argv[i]);
        if (s.length()>2 && s[0]=='\"' && s[s.length()-1]=='\"') {
            s = s.substr(1,s.length()-2);
        }
        result += s;

        // Add a space except for the last argument
        if(i != (argc-1)) {
            result += string(" ");
        }
    }

    if(result.length() > MAX_COMMAND_LENGTH) {
        printf("\n--------------------------------");
        printf("\nError: Length of command line string is over %d characters\n",MAX_COMMAND_LENGTH);
        PauseExit(EXIT_FAILURE,reInp);
    }

    return result;
}

int ExecuteCommand(string& command,bool reInp) {
    pid_t pid = fork();
    if (pid == 0) {
        //child process
        int pos = command.find_last_of('/');
        std::string file = command;
        if (pos>=0) {
            file = command.substr(pos+1);
        }
        int result=execl(command.c_str(),file.c_str(),NULL);
        if (result) {
            printf("Failed to start command %s %s!\n",command.c_str(), file.c_str());
            printf("errno %d: %s\n",errno,strerror(errno));
            printf("current dir: %s",get_current_dir_name());
            exit(-1);
        }
    } else {
        int status;
        pid_t w;
        w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
        if (w==-1) {
            perror("waitpid failed!");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return status;
        }
    }
    return 0;
}

int main(int argc, char** argv) {

    // First make sure we aren't going to read nonexistent arrays
    if(argc < 3) {
        printf("\n--------------------------------");
        printf("\nUsage: ConsolePauser.exe <0|1> <filename> <parameters>\n");
        printf("\n 1 means the STDIN is redirected by Dev-CPP;0 means not\n");
        PauseExit(EXIT_SUCCESS,false);
    }

    // Make us look like the paused program
    //SetConsoleTitleA(argv[2]);

    bool reInp;
    bool pauseAfterExit;
    // Then build the to-run application command
    string command = GetCommand(argc,argv,reInp, pauseAfterExit);
    if (reInp) {
        freopen("/dev/tty","w+",stdout);
        freopen("/dev/tty","w+",stderr);
    } else {
        fflush(stdin);
    }

    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    int fd_shm = shm_open("/REDPANDAIDECONSOLEPAUSER20211223",O_RDWR,S_IRWXU);
    if (fd_shm==-1) {
        //todo: handle error
        printf("shm open failed %d:%s",errno,strerror(errno));
    } else {
        if (ftruncate(fd_shm,BUF_SIZE)==-1){
            printf("ftruncate failed %d:%s",errno,strerror(errno));
            //todo: set size error
        } else {
            pBuf = (char*)mmap(NULL,BUF_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm,0);
            if (pBuf == MAP_FAILED) {
                printf("mmap failed %d:%s",errno,strerror(errno));
                pBuf = nullptr;
            }
        }
    }

    // Save starting timestamp
    auto starttime = std::chrono::high_resolution_clock::now();

    // Execute the command
    int returnvalue = ExecuteCommand(command,reInp);

    // Get ending timestamp
    auto endtime = std::chrono::high_resolution_clock::now();
    auto difftime = endtime - starttime;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(difftime);
    double seconds = milliseconds.count()/1000;

    if (pBuf) {
        strcpy(pBuf,"FINISHED");
        munmap(pBuf,BUF_SIZE);
    }
    if (fd_shm!=-1) {
        shm_unlink("/REDPANDAIDECONSOLEPAUSER20211223");
    }

    // Done? Print return value of executed program
    printf("\n--------------------------------");
    printf("\nProcess exited after %.4g seconds with return value %lu\n",seconds,returnvalue);
    if (pauseAfterExit)
        PauseExit(returnvalue,reInp);
    return 0;
}

