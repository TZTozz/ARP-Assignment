#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

volatile sig_atomic_t watchdogPid = 0;
int reachedTarget = 0;

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
    snprintf(buffer, sizeof(buffer), "targets: %d\n", pid);
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

ssize_t read_exact(int fd, void *buf, size_t count) 
{
    size_t bytes_read = 0;
    while (bytes_read < count) {
        ssize_t res = read(fd, (char *)buf + bytes_read, count - bytes_read);
        if (res > 0) {
            bytes_read += res;
        } else if (res == 0) {
            return bytes_read; // EOF
        } else {
            return -1; // Errore
        }
    }
    return bytes_read;
}

void ping_handler(int sig) 
{
    kill(watchdogPid, SIG_HEARTBEAT);
}


void ClearArray(int array[MaxHeight][MaxWidth])
{
    for(int i = 0; i < MaxHeight; i++)
    {
        for(int j; j < MaxWidth; j++)
        {
            array[i][j] = 0;
        }
    }
}

void Positioning(bool obsta[][MaxWidth], int targ[][MaxWidth], int height, int width)
{
    int dim = (height - 2) * (width - 2);        //-2 cause the targets must be inside the window
    //int numTargets = (int)(densityTargets * dim);

    log_debug("Width: %d, Height: %d", width, height);
    bool obstaVector[dim];
    int targetVector[dim];

    //Inizialize the vector with all false
    for(int i = 0; i < dim; i++)
    {
        targetVector[i] = 0;
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
    for(int i = (reachedTarget + 1); i < NumTargets + 1; i++)
    {
        do
        {
            j = rand() % dim;

        }while(obstaVector[j] || targetVector[j] != 0);      //Check for another target or obtacle in that position
        targetVector[j] = i;
    }

    //From vector to matrix
    for (int i = 0; i < dim; i++) 
    {
        int r = i / (width - 2);  //Row
        int c = i % (width - 2);  //Column
        targ[r+1][c+1] = targetVector[i];
    }

}

int IsTargetReached(int array[][MaxWidth], float x, float y, bool *firstLoss)
{
    
    if (array[(int)y][(int)x] != 0)      //If the drone is in the same square of the target
    {
        if (array[(int)y][(int)x] == reachedTarget + 1)     //The right target is reached
        {
            array[(int)y][(int)x] = 0;
            reachedTarget++;
            return 1;
        }
        else
        {
            if (*firstLoss)
            {
                //*score--;
                *firstLoss = false;
                log_debug("FirstLoss");
            }
            return -1;
        }
    }
    else if (!*firstLoss)
    {
        *firstLoss = true;
        log_warn("exit from the wrong target");
        return -2;
    }
    return 0;
}

void TargetAttraction(int array[][MaxWidth], float x, float y, float *Fx, float *Fy)
{
    int min_c = (int)(x - rho);
    int max_c = (int)(x + rho);
    int min_r = (int)(y - rho);
    int max_r = (int)(y + rho);

    if (min_c < 0) min_c = 0;
    if (min_r < 0) min_r = 0;
    
    for (int r = min_r; r <= max_r; r++)
    {
        for (int c = min_c; c <= max_c; c++)
        {
            if (array[r][c] != 0)
            {
                float dx = x - (c + 0.5f);   //The target is in the center of char space
                float dy = y - (r + 0.5f);   //The target is in the center of char space

                float d = sqrtf(dx*dx + dy*dy);

                //To avoid division per zero
                if (d < 0.001f) d = 0.001f;

                if (d < rho)
                {
                    //Latombe's formula
                    //F = eta * (1/d - 1/rho) * (1/d^2)
                    float term1 = (1.0f / d) - (1.0f / rho);
                    float F = eta * term1 * (1.0f / (d * d));

                    if (F > MaxAttraction) F = MaxAttraction;

                    //(dx/d) is the cosine
                    //(dy/d) is the sine
                    
                    float fx = -F * (dx / d);
                    float fy = -F * (dy / d);

                    log_debug("Target at [%d,%d] Dist: %f -> F: %f %f", r, c, d, fx, fy);
                    
                    *Fx += fx;
                    *Fy += fy;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int fd_r_target, fd_w_target;
    sscanf(argv[1], "%d %d %d", &fd_r_target, &fd_w_target, &watchdogPid);

    log_config(FILENAME_LOG, LOG_DEBUG);
    WritePid();
    kill(watchdogPid, SIG_WRITTEN);

    srand(time(NULL) + 10);
    
    int target[MaxHeight][MaxWidth];     //Max dimension of the screen
    bool obstacle[MaxHeight][MaxWidth];
    
    bool exiting = false;

    //Msg_int msg_int_out;
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

    int h_Win, w_Win;

    float Fx, Fy;
    int score = 0;
    bool firstLoss = true;

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
                read_exact(fd_r_target, obstacle, sizeof(obstacle));
                ClearArray(target);
                Positioning(obstacle, target, h_Win, w_Win);
                write(fd_w_target, target, sizeof(target));
                break;
            case 'f':       //The BB wants to know the forces applied by targets
                Fx = 0;
                Fy = 0;
                TargetAttraction(target, msg_float_in.a, msg_float_in.b, &Fx, &Fy);
                log_debug("Forces from targets: %f %f", Fx, Fy);
                int point = IsTargetReached(target, msg_float_in.a, msg_float_in.b, &firstLoss);
                if (point == 1)
                {
                    Set_msg(msg_float_out, 'w', Fx, Fy);        //win: right target reached
                }
                else if (point == 0)
                {
                    Set_msg(msg_float_out, 'n', Fx, Fy);        //nothing: target not reached
                }
                else if(point == -1)
                {
                    Set_msg(msg_float_out, 'l', Fx, Fy);        //lose: wrong target reached
                }
                else if(point == -2)
                {
                    Set_msg(msg_float_out, 'a', Fx, Fy);        //after loss: the drone is no more in the same square of a wrong target
                }
                write(fd_w_target, &msg_float_out, sizeof(msg_float_out));
                break;
            default:
                log_error("Format error");
                perror("Format not correct");
                break;
        }

        if (exiting) break;
        
    }

    close(fd_r_target);
    close(fd_w_target);
    return 0;

}

