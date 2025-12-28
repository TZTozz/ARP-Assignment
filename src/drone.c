#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <signal.h>
#include "logger.h"
#include "parameter_file.h"

volatile sig_atomic_t watchdogPid = 0;

void WritePid() 
{
    int fd;
    char buffer[32];
    pid_t pid = getpid();

    //Open the file
    fd = open(FILENAME_PID, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("Error open");
        exit(EXIT_FAILURE);
    }

    //Lock the file
    if (flock(fd, LOCK_EX) == -1) {
        perror("Errore flock lock");
        close(fd);
        exit(EXIT_FAILURE);
    }

    //Write
    snprintf(buffer, sizeof(buffer), "drone: %d\n", pid);
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Error write");
    } else {
        log_debug("Process %d: written on file", pid);
    }

    //Flush
    if (fsync(fd) == -1) {
        perror("Error fsync");
    }

    //Unlock
    if (flock(fd, LOCK_UN) == -1) {
        perror("Error flock unlock");
    }

    //Close
    close(fd);
}

void ping_handler(int sig) 
{
    kill(watchdogPid, SIG_HEARTBEAT);
}

/// @brief Calcola la nuova posizione del drone sulla base delle forze che riceve come input e calcola le forze
///         sulla base delle posizioni degli ostacoli e target
/// @param argc 
/// @param argv 
/// @return 
int main(int argc, char *argv[])
{
    //Handles the pipes
    int fd_r_drone, fd_w_drone;
    sscanf(argv[1], "%d %d %d", &fd_r_drone, &fd_w_drone, &watchdogPid);

    log_config("../files/simple.log", LOG_DEBUG);

    WritePid();
    kill(watchdogPid, SIG_WRITTEN);
    
    //Protocol messages
    Msg_int msg_int_in;
    Msg_float msg_float_in, msg_float_out;

    //Signal from watchdog
    struct sigaction sa_ping;
    sa_ping.sa_handler = ping_handler;
    sa_ping.sa_flags = SA_RESTART;
    sigemptyset(&sa_ping.sa_mask);
    
    if (sigaction(SIG_PING, &sa_ping, NULL) == -1) {
        perror("Error in ping_handler");
        exit(EXIT_FAILURE);
    }


    winDimension size;
    size.height = 20;
    size.width = 100;
    
    drone drn;
    
    bool exiting = false;

    //Initializzation drone proprieties
    drn.x=init_x;
    drn.y=init_y;
    drn.Fx=0;
    drn.Fy=0;
    drn.x_1=init_x;
    drn.y_1=init_y;
    drn.x_2=init_x;
    drn.y_2=init_y;
    
	
    //Read the initial dimension of the window
	read(fd_r_drone, &msg_int_in, sizeof(msg_int_in));
    size.height = msg_int_in.a;
    size.width = msg_int_in.b;


    while(1)
    {
        //Send to the blackboard the position of the drone
        Set_msg(msg_float_out, 'f', drn.x, drn.y);
        write(fd_w_drone, &msg_float_out, sizeof(msg_float_out));
        
        read(fd_r_drone, &msg_float_in, sizeof(msg_float_in));
        log_debug("Ch = %c", msg_float_in.type);
        switch(msg_float_in.type)
        {

            case 'q':               //Quitting
                exiting = true;
                break;
            case 'f':               //New forces applied to the drone
                drn.Fx = msg_float_in.a;
                drn.Fy = msg_float_in.b;
                break; 
            case 's':               //The window has been resized
                size.height = msg_float_in.a;
                size.width = msg_float_in.b;
                read(fd_r_drone, &msg_float_in, sizeof(msg_float_in));
                //log_debug("Letto forza post resize %c", msg_int_in.type);
                drn.Fx = msg_float_in.a;
                drn.Fy = msg_float_in.b;
                break;
            default:
                log_error("Format error");
                perror("Format not correct");
                break;
        }
        
        if (exiting) break;

        
        //Compute the drone dinamics
        drn.x_2=drn.x_1;
        drn.y_2=drn.y_1;
        drn.x_1 = drn.x;
        drn.y_1 = drn.y;
        drn.x=(T*T*drn.Fx-drn.x_2+(2+T*K)*drn.x_1)/(K*T+1);
        drn.y=(T*T*drn.Fy-drn.y_2+(2+T*K)*drn.y_1)/(K*T+1);

        //log_debug("Fx: %d, Fy: %d", drn.Fx, drn.Fy);

        //Set boundaries
        if (drn.x < 1) drn.x = 1;
        if (drn.x > size.width - 2) drn.x = size.width - 2;
        if (drn.y < 1) drn.y = 1;
        if (drn.y > size.height - 2) drn.y = size.height - 2;

        log_debug("Position: X: %f Y: %f", drn.x, drn.y);
        log_debug("Velocity: X: %f Y: %f", (drn.x - drn.x_1), (drn.y - drn.y_1));
        usleep(SleepTime);
    }

    close(fd_r_drone);
    close(fd_w_drone);
    return 0;
}