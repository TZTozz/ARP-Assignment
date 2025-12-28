#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define SIG_SETUP (SIGRTMIN + 1)
#define FILENAME_PID "../files/PID_file.log"
#define FILENAME_LOG "../files/log_file.log"
#define FILENAME_WATCHDOG "../files/watchdog.log"

int main() {

    int fd_r_drone[2], fd_w_drone[2];           //pipes for drone
    int fd_w_input[2];                          //pipes for input
    int fd_r_obstacle[2], fd_w_obstacle[2];     //pipes for obstacles
    int fd_r_target[2], fd_w_target[2];         //pipes for targets

    pipe(fd_r_drone);
    pipe(fd_w_drone);
    pipe(fd_w_input);
    pipe(fd_r_obstacle);
    pipe(fd_w_obstacle);
    pipe(fd_r_target);
    pipe(fd_w_target);


    pid_t watchdog = fork();
    if (watchdog== 0) 
    {
        execlp("./watchdog", "./watchdog", NULL);
        perror("exec watchdog");
        exit(1);
    }


    pid_t BB = fork();
    if (BB == 0) 
    {
        close(fd_r_drone[0]);
        close(fd_w_drone[1]);
        close(fd_w_input[1]);
        close(fd_r_obstacle[0]);
        close(fd_w_obstacle[1]);
        char fd_str[80];
        sprintf(fd_str, "%d %d %d %d %d %d %d %d",  fd_r_drone[1], fd_w_drone[0],
                                                    fd_w_input[0],
                                                    fd_r_obstacle[1], fd_w_obstacle[0],
                                                    fd_r_target[1], fd_w_target[0],
                                                    watchdog);
        execlp("konsole", "konsole", "-e", "./BB", fd_str, NULL);
        perror("exec BlackBoard");
        exit(1);
    }
    
    pid_t input = fork();
    if (input == 0) 
    {
        close(fd_w_input[0]);
        char fd_str[80];
        sprintf(fd_str, "%d %d", fd_w_input[1], watchdog);
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
        sprintf(fd_str, "%d %d %d", fd_r_drone[0], fd_w_drone[1], watchdog);
        execlp("./drone", "./drone", fd_str, NULL);
        perror("exec drone");
        exit(1);
    }

    pid_t obstacle = fork();
    if (obstacle == 0) 
    {
        close(fd_r_obstacle[1]);
        close(fd_w_obstacle[0]);
        char fd_str[80];
        sprintf(fd_str, "%d %d %d", fd_r_obstacle[0], fd_w_obstacle[1], watchdog);
        execlp("./obstacles", "./obstacles", fd_str, NULL);
        perror("exec obstacles");
        exit(1);
    }

    pid_t target = fork();
    if (target == 0) 
    {
        close(fd_r_target[1]);
        close(fd_w_target[0]);
        char fd_str[80];
        sprintf(fd_str, "%d %d %d", fd_r_target[0], fd_w_target[1], watchdog);
        execlp("./targets", "./targets", fd_str, NULL);
        perror("exec targets");
        exit(1);
    }

    //Creation file with all PID
    FILE *fp = fopen(FILENAME_PID, "w");
    if (fp == NULL) 
    {
        perror("Errore nell'apertura del file");
        return 1;
    }
    fclose(fp);

    //Creation log file
    FILE *fp_l = fopen(FILENAME_LOG, "w");
    if (fp_l == NULL) 
    {
        perror("Errore nell'apertura del file log");
        return 1;
    }
    fclose(fp_l);

    //Creation watchdog log
    FILE *fp_w = fopen(FILENAME_WATCHDOG, "w");
    if (fp_w == NULL) 
    {
        perror("Errore nell'apertura del file");
        return 1;
    }
    fclose(fp_w);

    int status;
    waitpid(watchdog, &status, 0);

    close(fd_r_drone[0]);
    close(fd_r_drone[1]);
    close(fd_w_drone[0]);
    close(fd_w_drone[1]);
    close(fd_w_input[0]);
    close(fd_w_input[1]);
    close(fd_r_obstacle[0]);
    close(fd_r_obstacle[1]);
    close(fd_w_obstacle[0]);
    close(fd_w_obstacle[1]);
    close(fd_r_target[0]);
    close(fd_r_target[1]);
    close(fd_w_target[0]);
    close(fd_w_target[1]);
    
    printf ("Main program exiting...\n");
    return 0;
}