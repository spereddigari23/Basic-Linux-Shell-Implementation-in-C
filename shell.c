#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
typedef struct Job{
    char *job_status;
    char *job_id;
    int process_id;
} Job;
int NUM_PROCESSES = 0;
Job list_jobs[100];
int exec_abs_path(char **args){
    int child_status;
    if (access(args[0], F_OK) == 0){
        pid_t cpid = fork();
        // GET CHILD'S PID AND SET TO JOBS OBJECT
        if (cpid == 0){ // child process
            list_jobs[NUM_PROCESSES-1].process_id=getpid();
            if (execve(args[0], args, NULL) == -1){
                perror("Execve failed\n");
                return -1;
            }
        }else if (cpid > 0){ // parent process
            pid_t wpid;
            do{
                waitpid(cpid, &child_status, 0);
            } while (WIFEXITED(child_status));
            return 1;
        }
    }
    return -1;
}
int check_built_in(char **args){
    if (strcmp(args[0], "cd") == 0){
        if (args[1] == NULL){
            chdir(getenv("HOME"));
        }
        else{
            if (chdir(args[1]) != 0){
                perror("Argument error\n");
            }
        }
        char temp[200];
        getcwd(temp, 200);
        setenv("PWD", temp, 1);
        getcwd(temp, 200);
        printf("Current working directory is: %s\n", temp);
        return 1;
    }
    else{
        const char *temp = "../usr/bin/";
        char *result = (char*)malloc(strlen(temp) + strlen(args[0]) + 1);
        strcat(result, temp);
        strcat(result, args[0]);
        char *path = (char *)malloc(PATH_MAX);
        realpath(result, path);
        char *origarg = args[0];
        args[0] = path;
        if (exec_abs_path(args) == -1){
            const char *temp = "/bin/";
            char *result = (char*)malloc(strlen(temp) + strlen(args[0]) + 1);
            strcat(result, temp);
            strcat(result, origarg);
            char *path = (char *)malloc(PATH_MAX);
            realpath(result, path);
            args[0] = path;
            int ret = exec_abs_path(args);
            if (ret == -1){
                printf("%s: command not found\n", origarg);
            }
        }
    }
    return -1;
}
char **split(char *line){
    int bufsize = 100, index = 0;
    const char *delim = " \t\r\n\a";
    char **arr = (char**)malloc(bufsize * sizeof(char *));
    char *idx;
    if (!arr){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    idx = strtok(line, delim);
    while (idx != NULL){
        arr[index] = idx;
        index++;
        if (index >= bufsize){
            bufsize += 100;
            arr =(char**) realloc(arr, bufsize * sizeof(char *));
            if (!arr){
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        idx = strtok(NULL, delim);
    }
    arr[index] = NULL;
    return arr;
}
int main(int argc, char const *argv[]){
    size_t bufsize = 0;
    int status = 1;
    int child_status;
    while (status == 1){
        char *str;
        char **args;
        printf("> ");
        getline(&str, &bufsize, stdin);
        args = split(str);
        if (args[0][0] == '/'){
            int ret = exec_abs_path(args);
            if (ret != 1){
                printf("%s: No such file or directory\n", args[0]);
            }
        }
        else if (args[0][0] == '.'){
            char *path = (char *)malloc(PATH_MAX);
            realpath(args[0], path);
            if (strlen(path) == 0){
                printf("%s: No such file or directory\n", args[0]);
            }
            args[0] = path;
            exec_abs_path(args);
        }
        else{
            int ret = check_built_in(args);
            if (ret != 1){
                perror("%s: command not found\n");
            }
        }
    }
}

