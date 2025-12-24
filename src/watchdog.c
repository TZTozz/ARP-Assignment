#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "logger.h"

#define SIG_HEARTBEAT SIGRTMIN
#define SIG_WRITTEN   (SIGRTMIN + 1)

pid_t PID_DRONE = 0;
pid_t PID_OBSTACLE = 0;
pid_t PID_BB = 0;
pid_t PID_INPUT = 0;
pid_t PID_TARGETS = 0;



volatile sig_atomic_t request_reload = 0;
volatile sig_atomic_t num_process = 0;

void Read_PID() 
{
    log_warn("Leggo");
    FILE *fp = fopen("../files/PID_file", "r");
    char line[256];

    if (fp == NULL) {
        perror("Errore: Impossibile leggere il file");
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
    if (num_process == 4)
    {
        request_reload = 1;
    }
}

void action_drone()
{
    log_warn("Polling from drone");
}

void action_obstacles()
{
    log_warn("Polling from obstacles");
}

void action_bb()
{
    log_warn("Polling from bb");
}

void action_targets()
{
    log_warn("Polling from targets");
}

void heartbeat_handler(int sig, siginfo_t *info, void *context) 
{
    log_warn("Arrivato un segnale");

    if (sig == SIG_WRITTEN) {
        action_writtenPid();
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

int main() 
{
    struct sigaction sa;

    log_config("simple.log", LOG_DEBUG);

    log_warn("Sono vivo");
    sa.sa_sigaction = heartbeat_handler;
    //sa.sa_flags = SA_RESTART;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    
    //Read_PID();

    if (sigaction(SIG_HEARTBEAT, &sa, NULL) == -1) {
        perror("Errore in sigaction heartbeat");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIG_WRITTEN, &sa, NULL) == -1) {
        perror("Errore in sigaction reads PID");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pause();

        if (request_reload) {
            Read_PID();
            request_reload = 0;
        }
    }

    return 0;

}
