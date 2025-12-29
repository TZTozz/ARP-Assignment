#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "logger.h"
#include "parameter_file.h"

pid_t PID_DRONE = 0;
pid_t PID_OBSTACLE = 0;
pid_t PID_BB = 0;
pid_t PID_INPUT = 0;
pid_t PID_TARGETS = 0;
pid_t PID_KONSOLE_BB = 0;
pid_t PID_KONSOLE_I = 0;


volatile sig_atomic_t request_reload = 0;
volatile sig_atomic_t num_process = 0;
volatile sig_atomic_t exiting = 0;
volatile sig_atomic_t watchdog_flag = 1;

volatile sig_atomic_t recv_drone = 1;
volatile sig_atomic_t recv_obstacles = 1;
volatile sig_atomic_t recv_bb = 1;
volatile sig_atomic_t recv_targets = 1;
volatile sig_atomic_t recv_input = 1;

volatile sig_atomic_t error_process = 0;

void Read_PID() 
{
    log_debug("Reading the PIDs");
    FILE *fp = fopen(FILENAME_PID, "r");
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
        else if (strncmp(line, "Konsole_BB:", 8) == 0) 
        {
            sscanf(line, "Konsole_BB: %d", &PID_KONSOLE_BB);
        }
        else if (strncmp(line, "Konsole_I:", 8) == 0) 
        {
            sscanf(line, "Konsole_I: %d", &PID_KONSOLE_I);
        }
        else
        {
            log_error("Unkonow process");
        }
    }
                
    fclose(fp);
}

static winDimension layout_and_draw(WINDOW *win) 
{
    int H, W;
    getmaxyx(stdscr, H, W);

    //Constant space outside the window
    int wh = (H > 6) ? H - 6 : H;
    int ww = (W > 10) ? W - 10 : W;
    if (wh < 3) wh = 3;
    if (ww < 3) ww = 3;

    winDimension size;
    size.height = wh;
    size.width = ww;

    //Resize the windows and put it in the center
    wresize(win, wh, ww);
    mvwin(win, (H - wh) / 2, (W - ww) / 2);

    //Erase and redraw the window
    werase(stdscr);
    werase(win);
    box(win, 0, 0);

    refresh();
    wrefresh(win);
    return size;
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
}

void action_obstacles()
{
    recv_obstacles = 1;
}

void action_bb()
{
    recv_bb = 1;
}

void action_targets()
{
    recv_targets = 1;
}

void action_input()
{
    recv_input = 1;
}

void action_watchdog()
{
    watchdog_flag = 1;
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
    else if (sender_pid == PID_INPUT) {
        action_input();
    }
    else {
        log_error("Process unknow: %d", sender_pid);;
    }

}

void watchdog_routine(WINDOW *my_win) 
{
    werase(my_win);
    int max_y, max_x;
    getmaxyx(my_win, max_y, max_x);

    //Time
    time_t rawtime;
    struct tm * timeinfo;
    char time_str[9];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);

    
    char msg_check[50];
    snprintf(msg_check, sizeof(msg_check), "--- WATCHDOG CHECK (%s) ---", time_str);
    int x_title = (max_x - strlen(msg_check)) / 2;
    log_debug(msg_check);
    wattron(my_win, A_BOLD);
    mvwprintw(my_win, 1, x_title, "%s", msg_check);
    wattroff(my_win, A_BOLD);

    int saved_errno = errno;
    int Y_coordinates = 2;

    if (PID_DRONE > 0) 
    {
        if (recv_drone == 0) 
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "ALERT: Drone dosen't respond!");
            log_error(msg);
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
            error_process = 1;
        }
        else
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "Drone is ok!");
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
        }
        recv_drone = 0; 
        kill(PID_DRONE, SIG_PING);
    }

    if (PID_OBSTACLE > 0) 
    {
        if (recv_obstacles == 0) 
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "ALERT: Obstacle dosen't respond!");
            log_error(msg);
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
            error_process = 1;
        }
        else
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "Obstacle is ok!");
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;

        }
        recv_obstacles = 0;
        kill(PID_OBSTACLE, SIG_PING);
    }

    if (PID_BB > 0) 
    {
        if (recv_bb == 0) 
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "ALERT: BB dosen't respond!");
            log_error(msg);
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
            error_process = 1;
        }
        else
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "BB is ok!");
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
        }
        recv_bb = 0;
        kill(PID_BB, SIG_PING);
    }

    if (PID_TARGETS > 0) 
    {
        if (recv_targets == 0) 
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "ALERT: Target dosen't respond!");
            log_error(msg);
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
            error_process = 1;
        }
        else
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "Target is ok!");
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
        }
        recv_targets = 0;
        kill(PID_TARGETS, SIG_PING);
    }

    if (PID_INPUT > 0) 
    {
        if (recv_input == 0) 
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "ALERT: Input dosen't respond!");
            log_error(msg);
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
            error_process = 1;
        }
        else
        {
            char msg[50];
            snprintf(msg, sizeof(msg), "Input is ok!");
            mvwprintw(my_win, Y_coordinates, 1, "%s", msg);
            Y_coordinates++;
        }
        recv_input = 0;
        kill(PID_INPUT, SIG_PING);
    }

    box(my_win, 0, 0);
    wrefresh(my_win);

    alarm(TIMER_WATCHDOG);
    
    errno = saved_errno;
}

void StopProcess()
{
    kill(PID_DRONE, SIGKILL);
    kill(PID_BB, SIGKILL);
    kill(PID_OBSTACLE, SIGKILL);
    kill(PID_TARGETS, SIGKILL);
    kill(PID_INPUT, SIGKILL);
    kill(PID_KONSOLE_BB, SIGTERM);
    kill(PID_KONSOLE_I, SIGTERM);
    log_debug("Ora dormo");
    sleep(5);
    log_debug("Dormito");
    exiting = true;
}


int main() 
{
    
    log_config(FILENAME_WATCHDOG, LOG_DEBUG);
    
    //Signal heartbeat from other process
    struct sigaction sa;
    sa.sa_sigaction = heartbeat_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    //Signal for timer
    struct sigaction sa_alarm;
    sa_alarm.sa_handler = action_watchdog;
    sa_alarm.sa_flags = SA_RESTART;
    sigemptyset(&sa_alarm.sa_mask);

    //Signal from konsole in case of error
    struct sigaction sa_hup;
    sa_hup.sa_handler = SIG_IGN;
    sa_hup.sa_flags = 0;
    sigemptyset(&sa_hup.sa_mask);


    if (sigaction(SIG_HEARTBEAT, &sa, NULL) == -1) 
    {
        perror("Error in sigaction heartbeat");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIG_WRITTEN, &sa, NULL) == -1) 
    {
        perror("Error in sigaction written PID");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIG_STOP, &sa, NULL) == -1) 
    {
        perror("Error in sigaction stop");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGALRM, &sa_alarm, NULL) == -1) 
    {
        perror("Error sigaction alarm");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGHUP, &sa_hup, NULL) == -1) {
        perror("Error sigaction SIGHUP");
    }

    //Window setting
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    WINDOW *win = newwin(10, 20, 5, 5);
    layout_and_draw(win);
    refresh();


    alarm(TIMER_WATCHDOG);

    while (1) 
    {
        pause();

        if (request_reload) 
        {
            Read_PID();
            request_reload = 0;
        }

        if(watchdog_flag)
        {
            watchdog_routine(win);
            watchdog_flag = 0;

        }

        if(error_process)
        {
            StopProcess();
        }

        if (exiting) break;
    }

    endwin();

    return 0;

}
