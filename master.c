//Code to call First and Second with Fork() and Execvp()

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main() {

    int fd_r_drone[2], fd_w_drone[2];       //pipes for drone
    int fd_w_input[2];       //pipes for input

    pipe(fd_r_drone);
    pipe(fd_w_drone);
    pipe(fd_w_input);

    pid_t BB = fork();
    if (BB == 0) 
    {
        close(fd_r_drone[0]);
        close(fd_w_drone[1]);
        close(fd_w_input[1]);
        char fd_str[80];
        sprintf(fd_str, "%d %d %d", fd_r_drone[1], fd_w_drone[0], fd_w_input[0]);
        execlp("./BB", "./BB", fd_str, NULL);
        perror("exec BlackBoard");
        exit(1);
    }
    
    pid_t input = fork();
    if (input == 0) 
    {
        close(fd_w_input[0]);
        char fd_str[80];
        sprintf(fd_str, "%d", fd_w_input[1]);
        execlp("konsole", "konsole", "-e", "./I_process", fd_str, NULL);
        perror("exec I_process");
        exit(1);
    }

    pid_t drone = fork();
    if (drone == 0) 
    {
        close(fd_r_drone[1]);
        close(fd_w_drone[0]);
        char fd_str[80];
        sprintf(fd_str, "%d %d", fd_r_drone[0], fd_w_drone[1]);
        execlp("konsole", "konsole", "-e", "./drone", fd_str, NULL);
        perror("exec drone");
        exit(1);
    }

    close(fd_r_drone[0]);
    close(fd_r_drone[1]);
    close(fd_w_drone[0]);
    close(fd_w_drone[1]);
    close(fd_w_input[0]);
    close(fd_w_input[1]);
    
    
    printf ("Main program exiting...\n");
    return 0;
}