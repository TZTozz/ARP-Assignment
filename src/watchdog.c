#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "logger.h"
#include "parameter_file.h"

pid_t PID_DRONE = 0;
pid_t PID_OBSTACLE = 0;
pid_t PID_BB = 0;
pid_t PID_INPUT = 0;
pid_t PID_TARGETS = 0;

volatile sig_atomic_t request_reload = 0;
volatile sig_atomic_t num_process = 0;
volatile sig_atomic_t exiting = 0;

volatile sig_atomic_t recv_drone = 0;
volatile sig_atomic_t recv_obstacles = 0;
volatile sig_atomic_t recv_bb = 0;
volatile sig_atomic_t recv_targets = 0;

void Read_PID() 
{
    log_debug("Reading the PIDs");
    FILE *fp = fopen("../files/PID_file", "r");
    char line[256];

    if (fp == NULL) 
    {
        log_error("Error: impossible reading the file");
        perror("Error: impossible reading the file");
        return;
    }

    while (fgets(line, sizeof(line), fp)) 
    {
        if (strncmp(line, "drone:", 6) == 0) 
        {
            sscanf(line, "drone: %d", &PID_DRONE);
        }
        else if (strncmp(line, "obstacles:", 10) == 0) 
        {
            sscanf(line, "obstacles: %d", &PID_OBSTACLE);
        }
        else if (strncmp(line, "BB:", 3) == 0) 
        {
            sscanf(line, "BB: %d", &PID_BB);
        }
        else if (strncmp(line, "input:", 6) == 0) 
        {
            sscanf(line, "input: %d", &PID_INPUT);
        }
        else if (strncmp(line, "targets:", 8) == 0) 
        {
            sscanf(line, "targets: %d", &PID_TARGETS);
        }
        else
        {
            log_error("Unkonow process");
        }
    }
                
    fclose(fp);
}

void action_writtenPid()
{
    num_process++;
    log_debug("Num_process: %d", num_process);
    if (num_process == 5)
    {
        request_reload = 1;
    }
}

void action_stop()
{
    exiting = 1;
}

void action_drone()
{
    recv_drone = 1;
    //log_warn("Polling from drone");
}

void action_obstacles()
{
    recv_obstacles = 1;
    //log_warn("Polling from obstacles");
}

void action_bb()
{
    recv_bb = 1;
    //log_warn("Polling from bb");
}

void action_targets()
{
    recv_targets = 1;
    //log_warn("Polling from targets");
}

void heartbeat_handler(int sig, siginfo_t *info, void *context) 
{
    //log_warn("Arrivato un segnale");

    if (sig == SIG_WRITTEN) {
        action_writtenPid();
        return;
    }

    if (sig == SIG_STOP) {
        action_stop();
        return;
    }
    
    
    // info->si_pid contiene il PID del processo che ha mandato il segnale 
    
    int sender_pid = info->si_pid;

    if (sender_pid == PID_DRONE) {
        action_drone();
    } 
    else if (sender_pid == PID_OBSTACLE) {
        action_obstacles();
    }
    else if (sender_pid == PID_BB) {
        action_bb();
    }
    else if (sender_pid == PID_TARGETS) {
        action_targets();
    }
    else {
        //gestisci_processo_sconosciuto(sender_pid);
        log_error("Processo sconosciuto: %d", sender_pid);;
    }

}

void watchdog_routine(int sig) 
{
    log_debug("--- WATCHDOG CHECK (5s) ---");
    int saved_errno = errno;

    if (PID_DRONE > 0) {
        if (recv_drone == 0) {
            log_error("ALERT: Drone dosen't respond!");
        }
        recv_drone = 0; 
        kill(PID_DRONE, SIG_PING);
    }

    if (PID_OBSTACLE > 0) {
        if (recv_obstacles == 0) {
            log_error("ALERT: Obstacle dosen't respond!");
        }
        recv_obstacles = 0;
        kill(PID_OBSTACLE, SIG_PING);
    }

    if (PID_BB > 0) {
        if (recv_bb == 0) {
            log_error("ALERT: BB dosen't respond!");
        }
        recv_bb = 0;
        kill(PID_BB, SIG_PING);
    }

    if (PID_TARGETS > 0) {
        if (recv_targets == 0) {
            log_error("ALERT: Target dosen't respond!");
        }
        recv_targets = 0;
        kill(PID_TARGETS, SIG_PING);
    }

    alarm(TIMER_WATCHDOG);
    
    errno = saved_errno;
}

int main() 
{
    
    log_config("../files/Watchdog_log.log", LOG_DEBUG);
    
    struct sigaction sa;
    sa.sa_sigaction = heartbeat_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    struct sigaction sa_alarm;
    sa_alarm.sa_handler = watchdog_routine;
    sa_alarm.sa_flags = SA_RESTART;
    sigemptyset(&sa_alarm.sa_mask);

    recv_drone = 1;
    recv_obstacles = 1;
    recv_bb = 1;
    recv_targets = 1;


    if (sigaction(SIG_HEARTBEAT, &sa, NULL) == -1) {
        perror("Errore in sigaction heartbeat");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIG_WRITTEN, &sa, NULL) == -1) {
        perror("Errore in sigaction reads PID");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIG_STOP, &sa, NULL) == -1) {
        perror("Errore in sigaction reads PID");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGALRM, &sa_alarm, NULL) == -1) {
        perror("Errore sigaction alarm");
        exit(EXIT_FAILURE);
    }

    alarm(TIMER_WATCHDOG + 1);

    while (1) {
        pause();

        if (request_reload) {
            Read_PID();
            request_reload = 0;
        }

        if (exiting) break;
    }

    return 0;

}
