/* 
Written by Alex Garrett for UVIC CSC 360 2024

Netlink ID: V00994221
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include "linkedls.h"
#define BUFFER 1024
#define MAX_ARGS 15 

Node *head = NULL;

// extract arguments from command
void parseArgs(char *command, char *args[]) {
    int i = 0;
    args[i] = strtok(command, " \n");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        args[++i] = strtok(NULL, " \n");
    }
    args[i] = NULL;
}

// forks and runs new process in child. Adds new PID to linked list
void bg(char *command){

    char *args[MAX_ARGS];
    parseArgs(command, args);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
    } else if (pid == 0) { // CHILD PROCESS
        close(pipefd[0]);
        if (execvp(args[0], args) < 0) { // run command in new child process
            perror("Exec failed");
            write(pipefd[1], "1", 1); // write error status to pipe
            close(pipefd[1]);
            exit(1);
        }
        close(pipefd[1]);
    } 
    else { // PARENT PROCESS
        close(pipefd[1]);
        fd_set set;
        struct timeval timeout;
        int selRet;
        char buf = 0;

        FD_ZERO(&set); // clear the set
        FD_SET(pipefd[0], &set); // add the pipe read end to the set

        timeout.tv_sec = 1; // timeout
        timeout.tv_usec = 0;

        selRet = select(pipefd[0] + 1, &set, NULL, NULL, &timeout);
        if (selRet == -1) {
            perror("select");
        } else if (selRet == 0) {
            // assume success after timeout
            head = newNode(head, pid, args[0]);
            printf("Started background process with PID %d\n", pid);
        } else {
            read(pipefd[0], &buf, 1); // read from pipe
        }
        close(pipefd[0]);
    }
}

// kill given process
void bgkill(int pid){
    if (pidExist(head, pid) == 1){
        if (kill(pid, SIGKILL) == 0) { // send SIGKILL
            head = deleteNode(head, pid);
            printf("Process %d has been killed\n", pid);
        } else {
            perror("Failed to kill process");
        }
    }
}

// stop given process
void bgstop(int pid){
    if (pidExist(head, pid) == 1){
        if (kill(pid, SIGSTOP) == 0) {
            printf("Process %d stopped\n", pid);
        } else {
            perror("Failed to stop process");
        }
    }
}

// start given process
void bgstart(int pid){
    if (pidExist(head, pid) == 1){
        if (kill(pid, SIGCONT) == 0) {
            printf("Process %d started\n", pid);
        } else {
            perror("Failed to start process");
        }
    }
}

// opens /proc/[pid]/stat file and tokenizes all items
char ** parseProcStat(int pid, int *count){
    char path[BUFFER], buffer[BUFFER];
    FILE *file;
    char *tokens[64];
    int tcount = 0;


    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!fgets(buffer, BUFFER, file)){
        fprintf(stderr, "Error reading /proc/%d/stat", pid);
    };
    fclose(file);

    // tokenize "/stat" cat with " " delim
    char *token = strtok(buffer, " ");
    while (token != NULL) {
        tokens[tcount++] = token;
        token = strtok(NULL, " ");
    }

    // allocate memory for tokens
    char **result = malloc(tcount * sizeof(char*));
    for (int i = 0; i < tcount; i++) {
        result[i] = strdup(tokens[i]);
    }

    *count = tcount;
    return result;
}

// opens /proc/[pid]/status file and extracts voluntary and nonvoluntary ctxt switches
int parseProcStatus(int pid, long *voluntary, long *nonvoluntary){

    char path[BUFFER], buffer[BUFFER];
    FILE *file;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) return -1;

    *voluntary = -1; 
    *nonvoluntary = -1;

    while (fgets(buffer, BUFFER, file)) {
        sscanf(buffer, "voluntary_ctxt_switches: %ld", voluntary);
        sscanf(buffer, "nonvoluntary_ctxt_switches: %ld", nonvoluntary);
    }

    fclose(file);

    if (*voluntary == -1 || *nonvoluntary == -1) {
        fprintf(stderr, "Error reading /proc/%d/stat", pid);
        return -1;
    }

    return 1;
}

/* returns status information about given pid
comm: (filename)
state
utime
rss
voluntary ctxt switches
nonvoluntary ctxt switches */
void pstat(int pid){
    if (!pidExist(head, pid)) return;
    
    int count = 0;
    char **tokens = parseProcStat(pid, &count);
    if (!tokens) return;

    long voluntary, nonvoluntary;
    parseProcStatus(pid, &voluntary, &nonvoluntary);

    printf("comm: %s\n",tokens[1]);
    printf("state: %c\n",tokens[2][0]);
    printf("utime: %llu\n",strtoll(tokens[13], NULL , 10)/sysconf(_SC_CLK_TCK));
    printf("stime: %llu\n",strtoll(tokens[14], NULL , 10)/sysconf(_SC_CLK_TCK));
    printf("rss: %ld\n",strtol(tokens[23], NULL, 10));
    printf("voluntary_ctxt_switches: %ld\n", voluntary);
    printf("nonvoluntary_ctxt_switches: %ld\n", nonvoluntary);

    // Free allocated memory for tokens
    for (int i = 0; i<count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// check for terminated background processes
void checkTerm() {
    Node *tmp = head;
    int status;
    int val;

    // iterate through all nodes and call waitpid()
    while (tmp){
        val = waitpid(tmp->pid, &status, WNOHANG);
        if (val > 0){
            head = deleteNode(head, tmp->pid);
            printf("Process %d has terminated.\n", tmp->pid);
        } else if (val == -1){
            perror("waitpid error");
        }
        
        tmp = tmp->next;
    }
}

int main() {
    char input[256];
    char command[256];
    pid_t pid;

    while (1) {
        checkTerm();

        printf("PMan: > ");
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0';

        if (strcmp(input, "") == 0){ // if user only presses enter
            continue;
        }

        // bg ... 
        else if (strncmp(input, "bg ", 3) == 0 || strcmp(input, "bg") == 0) {
            if (strcmp(input, "bg ") == 0 || strcmp(input, "bg") == 0){
                printf("Usage: bg ./[executable/path] [arg_1] ... [arg_n]\n");
            }
            strcpy(command, input + 3);
            bg(command);
        }
        
        // bglist
        else if (strcmp(input, "bglist") == 0) {
            printList(head);
        }

        // pstat
        else if (strncmp(input, "pstat", 5) == 0){
            pid = atoi(input + 5);
            pstat(pid);
        }

        // bgkill PID
        else if (strncmp(input, "bgkill ", 7) == 0) {
            pid = atoi(input + 7);
            bgkill(pid);
        }
        
        // bgstop PID
        else if (strncmp(input, "bgstop ", 7) == 0) {
            bgstop(atoi(input + 7));
        }

        // bgstart PID
        else if (strncmp(input, "bgstart ", 8) == 0) {
            bgstart(atoi(input + 8));
        }

        else {
            printf("'%s': command not found\n", input);
        }
    }

    return 0;
}



