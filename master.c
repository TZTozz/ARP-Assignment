//Code to call First and Second with Fork() and Execvp()

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main() {

    int fd[2];
    pipe(fd);

    pid_t input = fork();
    if (input == 0) {
        close(fd[0]);
        char fd_write_str[10];
        sprintf(fd_write_str, "%d", fd[1]);
        execlp("konsole", "konsole", "-e", "./I_process", fd_write_str, NULL);
        perror("exec I_process");
        exit(1);
    }

    pid_t drone = fork();
    if (drone == 0) {
        close(fd[1]);
        char fd_read_str[10];
        sprintf(fd_read_str, "%d", fd[0]);
        execlp("konsole", "konsole", "-e", "./drone", fd_read_str, NULL);
        perror("exec drone");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);
    
    
    printf ("Main program exiting...\n");
    return 0;
}