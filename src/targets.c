#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <signal.h>
#include "logger.h"
#include "parameter_file.h"

void WritePid() {
    int fd;
    char buffer[32];
    pid_t pid = getpid();

    // 1. OPEN: Apre il file (o lo crea) in modalità append
    fd = open(FILENAME_PID, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("Errore open");
        exit(EXIT_FAILURE);
    }

    // 2. LOCK: Acquisisce il lock esclusivo
    // Il processo si blocca qui se un altro sta già scrivendo.
    if (flock(fd, LOCK_EX) == -1) {
        perror("Errore flock lock");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 3. WRITE: Scrive il PID nel buffer e poi nel file
    snprintf(buffer, sizeof(buffer), "targets: %d\n", pid);
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Errore write");
    } else {
        log_debug("Processo %d: scritto su file", pid);
    }

    // 4. FLUSH: Forza la scrittura dalla cache al disco
    // Questo svuota la cache interna del sistema operativo.
    if (fsync(fd) == -1) {
        perror("Errore fsync");
    }

    // 5. RELEASE LOCK: Rilascia il lock
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore flock unlock");
    }

    // 6. CLOSE: Chiude il file descriptor
    close(fd);
}

void ClearArray(bool array[MaxHeight][MaxWidth])
{
    for(int i; i < MaxHeight; i++)
    {
        for(int j; j < MaxWidth; j++)
        {
            array[i][j] = false;
        }
    }
}

void Positioning(bool obsta[][MaxWidth], bool targ[][MaxWidth], int height, int width)
{
    int dim = (height - 2) * (width - 2);        //-2 cause the targets must be inside the window
    int numTargets = (int)(densityTargets * dim);

    log_debug("Width: %d, Height: %d", width, height);
    bool targetVector[dim], obstaVector[dim];
    
    //Inizialize the vector with all false
    for(int i = 0; i < dim; i++)
    {
        targetVector[i] = false;
    }

    //From array to vector
    for (int r = 0; r < height - 2; r++) 
    {
        for (int c = 0; c < width - 2; c++) 
        {
            int i = r * (width - 2) + c;
            obstaVector[i] = obsta[r + 1][c + 1];
        }
    }

    //Fill the vector with a number of targets as numTargets
    int j;
    for(int i = 0; i < numTargets; i++)
    {
        do
        {
            j = rand() % dim;

        }while(obstaVector[j] || targetVector[j]);      //Check for another target or obtacle in that position
        targetVector[j] = true;
    }

    //From vector to matrix
    for (int i = 0; i < dim; i++) 
    {
        int r = i / (width - 2);  //Row
        int c = i % (width - 2);  //Column
        targ[r+1][c+1] = targetVector[i];
    }

}

int main(int argc, char *argv[])
{
    int fd_r_target, fd_w_target, watchdog_pid;
    sscanf(argv[1], "%d %d %d", &fd_r_target, &fd_w_target, &watchdog_pid);

    log_config("simple.log", LOG_DEBUG);
    WritePid();
    kill(watchdog_pid, SIG_WRITTEN);
    
    bool target[MaxHeight][MaxWidth];     //Max dimension of the screen
    bool obstacle[MaxHeight][MaxWidth];
    
    bool exiting = false;

    //Msg_int msg_int_out;
    Msg_float msg_float_in;

    //Signal setup
    union sigval value;
    value.sival_int = 1;

    int h_Win, w_Win;

    while(1)
    {
        read(fd_r_target, &msg_float_in, sizeof(msg_float_in));
        log_debug("The char is %c", msg_float_in.type);
        switch (msg_float_in.type)
        {
            case 'q':
                exiting = true;
                break;
            case 't':       //The BB wants to know the position of the targets
                h_Win = (int)msg_float_in.a;
                w_Win = (int)msg_float_in.b;
                read(fd_r_target, obstacle, sizeof(obstacle));
                ClearArray(target);
                Positioning(obstacle, target, h_Win, w_Win);
                write(fd_w_target, target, sizeof(target));
                break;
            case 'f':       //The BB wants to know the forces applied by targets
                //-------TO BE IMPLEMENTED------
                // Fx = 0;
                // Fy = 0;
                // CheckObNear(target, msg_float_in.a, msg_float_in.b, &Fx, &Fy);
                // Set_msg(msg_float_out, 'f', Fx, Fy);
                // write(fd_w_target, &msg_float_out, sizeof(msg_float_out));
                break;
            default:
                log_error("Format error");
                perror("Format not correct");
                break;
        }

        if (exiting) break;

        kill(watchdog_pid, SIG_HEARTBEAT);
        
    }

    close(fd_r_target);
    close(fd_w_target);
    return 0;

}

