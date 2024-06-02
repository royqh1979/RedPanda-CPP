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
#include <vector>
using std::string;
using std::vector;
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <chrono>
#include <sys/time.h>
#include <sys/resource.h>
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

vector<string> GetCommand(int argc,char** argv,bool &reInp,bool &pauseAfterExit) {
    vector<string> result;
    int flags = atoi(argv[1]);
    reInp = flags & RPF_REDIRECT_INPUT;
    pauseAfterExit = flags & RPF_PAUSE_CONSOLE;
    for(int i = 3;i < argc;i++) {
        //result += string("\"") + string(argv[i]) + string("\"");
        std::string s(argv[i]);

        if (i==3 || (reInp && i==4 ))
        if (s.length()>2 && s[0]=='\"' && s[s.length()-1]=='\"') {
            s = s.substr(1,s.length()-2);
        }
        result.push_back(s);
    }

    return result;
}

string unescapeSpaces(const string& s) {
    string result;
    size_t i=0;
    while(i<s.length()) {
        if (s[i]=='%' && (i+2)<s.length() && s[i+1]=='2' && s[i+2]=='0') {
            result.push_back(' ');
            i+=3;
        } else {
            result.push_back(s[i]);
            i++;
        }
    }
    return result;
}

int ExecuteCommand(vector<string>& command,bool reInp, long int &peakMemory) {
    peakMemory = 0;
    pid_t pid = fork();
    if (pid == 0) {
        string path_to_command;
        char * * argv;
        int command_begin;
        int command_size;
        if (reInp) {
            if (command.size()<2) {
                fprintf(stderr,"not enough arguments1!\n");
                exit(-1);
            }
            freopen(unescapeSpaces(command[0]).c_str(),"r",stdin);
            path_to_command = unescapeSpaces(command[1]);
            command_size = command.size()+1;
            command_begin = 1;
        } else {
            if (command.size()<1) {
                fprintf(stderr,"not enough arguments2!\n");
                exit(-1);
            }
            path_to_command = unescapeSpaces(command[0]);
            command_size = command.size()+1;
            command_begin = 0;
        }
        argv = (char * *)malloc(sizeof(char *)*command_size);
        for (size_t i=command_begin;i<command.size();i++) {
            argv[i-command_begin] = (char *)command[i].c_str();
        }
        argv[command.size()-command_begin]=NULL;
        //child process
        int pos = path_to_command.find_last_of('/');
        std::string file = path_to_command;
        if (pos>=0) {
            file = path_to_command.substr(pos+1);
        }
        argv[0]=(char *)file.c_str();
        int result=execv(path_to_command.c_str(),argv);
        if (result) {
            fprintf(stderr,"Failed to start command %s %s!\n",path_to_command.c_str(), file.c_str());
            fprintf(stderr,"errno %d: %s\n",errno,strerror(errno));
            char* current_dir = getcwd(nullptr, 0);
            fprintf(stderr,"current dir: %s",current_dir);
            free(current_dir);
            exit(-1);
        }
        free(argv);
    } else {
        int status;
        pid_t w;
        struct rusage usage;
        w = wait4(pid, &status, WUNTRACED | WCONTINUED, &usage);
        if (w==-1) {
            fprintf(stderr,"wait4 failed!");
            exit(EXIT_FAILURE);
        }
        peakMemory = usage.ru_maxrss;
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return status;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    char* sharedMemoryId;
    // First make sure we aren't going to read nonexistent arrays
    if(argc < 4) {
        fprintf(stderr,"\n--------------------------------");
        fprintf(stderr,"\nUsage: consolepauser <0|1> <shared_memory_id> <filename> <parameters>\n");
        fprintf(stderr,"\n 1 means the STDIN is redirected by Red Panda C++; 0 means not\n");
        PauseExit(EXIT_SUCCESS,false);
    }

    // Make us look like the paused program
    //SetConsoleTitleA(argv[3]);
    sharedMemoryId = argv[2];

    bool reInp;
    bool pauseAfterExit;
    // Then build the to-run application command
    vector<string> command = GetCommand(argc,argv,reInp, pauseAfterExit);
    if (reInp) {
        freopen("/dev/tty","w+",stdout);
        freopen("/dev/tty","w+",stderr);
    } else {
        fflush(stdin);
    }

    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    int fd_shm = shm_open(sharedMemoryId,O_RDWR,S_IRWXU);
    if (fd_shm==-1) {
        //todo: handle error
        fprintf(stderr,"shm open failed %d:%s\n",errno,strerror(errno));
    } else {
        // `ftruncate` has already done in RedPandaIDE
        pBuf = (char*)mmap(NULL,BUF_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm,0);
        if (pBuf == MAP_FAILED) {
            fprintf(stderr,"mmap failed %d:%s\n",errno,strerror(errno));
            pBuf = nullptr;
        }
    }

    // Save starting timestamp
    auto starttime = std::chrono::high_resolution_clock::now();

    // Execute the command
    long int peakMemory;
    int returnvalue = ExecuteCommand(command,reInp, peakMemory);

    // Get ending timestamp
    auto endtime = std::chrono::high_resolution_clock::now();
    auto difftime = endtime - starttime;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(difftime);
    double seconds = milliseconds.count()/1000.0;

    if (pBuf) {
        strcpy(pBuf,"FINISHED");
        munmap(pBuf,BUF_SIZE);
    }
    if (fd_shm!=-1) {
        shm_unlink(sharedMemoryId);
    }

    // Done? Print return value of executed program
    printf("\n--------------------------------");
    printf("\nProcess exited after %.4g seconds with return value %d, %ld KB mem used.\n",seconds,returnvalue,peakMemory);
    if (pauseAfterExit)
        PauseExit(returnvalue,reInp);
    return 0;
}

