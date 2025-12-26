#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <errno.h>
#include "logger.h"
#include "parameter_file.h"
#include <signal.h>

volatile sig_atomic_t need_resize = 0;
volatile sig_atomic_t watchdogPid = 0;
volatile sig_atomic_t reposition = 0;

void WritePid() 
{
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
    snprintf(buffer, sizeof(buffer), "BB: %d\n", pid);
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

void ping_handler(int sig) 
{
    kill(watchdogPid, SIG_HEARTBEAT);
}

void redraw_routine(int sig)
{
    reposition = 1;
    alarm(15);
}

int max(int a, int b) 
{
    return (a > b) ? a : b;
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

WINDOW *create_newwin(int height, int width, int starty, int startx){	

	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0);		
					
					
	wrefresh(local_win);		

	return local_win;
}


void destroy_win(WINDOW *local_win)
{	
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	wrefresh(local_win);
	delwin(local_win);
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

void PrintObstacle(WINDOW *win, bool array[][MaxWidth], int height, int width)
{
    for(int i = 0; i < height - 2; i++)
    {
        for(int j = 0; j < width - 2; j++)
        {
            if (array[i][j]) 
            {
                mvwaddch(win, i, j, '0' | COLOR_PAIR(2));
            }   
        }
    }
}

void PrintTarget(WINDOW *win, int array[][MaxWidth], int height, int width, int target_reached)
{
    for(int i = 0; i < height - 2; i++)
    {
        for(int j = 0; j < width - 2; j++)
        {
            if (array[i][j] > target_reached) 
            {
                char ch = array[i][j] + '0';
                mvwaddch(win, i, j, ch | COLOR_PAIR(3));
            }   
        }
    }
}

void handle_winch(int sig) 
{
    need_resize = 1;
}

int main(int argc, char *argv[])
{
    //Handles the pipes
    int fd_r_drone, fd_w_drone, fd_w_input, fd_r_obstacle, fd_w_obstacle, fd_r_target, fd_w_target;
    sscanf(argv[1], "%d %d %d %d %d %d %d %d", &fd_r_drone, &fd_w_drone,
                                                &fd_w_input,
                                                &fd_r_obstacle, &fd_w_obstacle,
                                                &fd_r_target, &fd_w_target,
                                                &watchdogPid);
    
    
    log_config("../files/simple.log", LOG_DEBUG);

    WritePid();
    kill(watchdogPid, SIG_WRITTEN);
    
    int max_fd = max(fd_w_drone, fd_w_input);
    int retval;
    fd_set r_fds;
    
    //Variables relative to the window
    int startx, starty, width, height;
    bool sizeChanged = false;
    WINDOW *my_win;
    winDimension size;
    
    //Protocol messages
    Msg_int msg_int_in, msg_int_out;
    Msg_float msg_float_in, msg_float_out;
    
    
    //Blackboard data
    float xDrone = 1, yDrone = 1;
    float Fx = 0, Fy = 0;
    float F_obstacle_X = 0, F_obstacle_Y = 0;
    float F_target_X = 0, F_target_Y = 0;
    int score = 0;
    int targetReached = 0;
    
    bool isExiting = false;
    bool redraw_target = false, redraw_drone = false;
    bool firstLoss = true;


    //Obstacle and target array
    bool obstacle[MaxHeight][MaxWidth]; 
    int target[MaxHeight][MaxWidth];
    
    //Window setting
    initscr(); 
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    use_default_colors();
    setlocale(LC_ALL, "C");

    init_pair(1, COLOR_MAGENTA, -1);    //Color drone
    init_pair(2, COLOR_RED, -1);        //Color obstacles
    init_pair(3, COLOR_GREEN, -1);      //Color targets
    
    
    //Signal from watchdog
    struct sigaction sa_ping;
    sa_ping.sa_handler = ping_handler;
    sa_ping.sa_flags = SA_RESTART;
    sigemptyset(&sa_ping.sa_mask);
    
    if (sigaction(SIG_PING, &sa_ping, NULL) == -1) {
        perror("Error in ping_handler");
        exit(EXIT_FAILURE);
    }
    
    //Signal for the risize  
    struct sigaction sa_winch;
    sa_winch.sa_handler = handle_winch;
    sa_winch.sa_flags = 0;
    sigemptyset(&sa_winch.sa_mask);

    if (sigaction(SIGWINCH, &sa_winch, NULL) == -1) {
        perror("Error in sigaction SIGWINCH");
        exit(EXIT_FAILURE);
    }

    //Timer for the redrawing
    struct sigaction sa_alarm;
    sa_alarm.sa_handler = redraw_routine;
    sa_alarm.sa_flags = SA_RESTART;
    sigemptyset(&sa_alarm.sa_mask);

    if (sigaction(SIGALRM, &sa_alarm, NULL) == -1) {
        perror("Errore sigaction alarm");
        exit(EXIT_FAILURE);
    }
    
    
    //Create window
    size.height = 15;
    size.width = 20;
    starty = (LINES - size.height) / 2;	
	startx = (COLS - size.width) / 2;
    
    my_win = create_newwin(size.height, size.width, starty, startx);
    size = layout_and_draw(my_win);
    log_debug("height: %d Width: %d", size.height, size.width);


    //Communicate to the drone the window's dimensions
    Set_msg(msg_int_out, 's', size.height, size.width);
    write(fd_r_drone, &msg_int_out, sizeof(msg_int_out));


    //Communicate to obstacles the window's dimensions and prints the obstacles
    Set_msg(msg_float_out, 'o', size.height, size.width);
    write(fd_r_obstacle, &msg_float_out, sizeof(msg_float_out));
    read_exact(fd_w_obstacle, obstacle, sizeof(obstacle));
    PrintObstacle(my_win, obstacle, size.height, size.width);
    

    //Communicate to targets the window's dimensions and prints the targets
    Set_msg(msg_float_out, 't', size.height, size.width);
    write(fd_r_target, &msg_float_out, sizeof(msg_float_out));
    write(fd_r_target, obstacle, sizeof(obstacle));
    read_exact(fd_w_target, target, sizeof(target));
    PrintTarget(my_win, target, size.height, size.width, targetReached);

    //Print the drone in the initial position
    mvwaddch(my_win, yDrone, xDrone, skin | COLOR_PAIR(1));
    wrefresh(my_win);

    alarm(15);

    while (1)
    {
        //Inizialization for the select
        FD_ZERO(&r_fds);
        FD_SET(fd_w_drone, &r_fds);
        FD_SET(fd_w_input, &r_fds);

        //If the window has changed dimension
        if (need_resize) 
        {
            need_resize = 0;
            endwin();
            refresh();
            resize_term(0, 0);          // o resizeterm(0, 0)
            size = layout_and_draw(my_win);       // ricalcola layout e ridisegna

            reposition = true;
            sizeChanged = true;
        }

        if(reposition)
        {
            werase(my_win);
            box(my_win, 0, 0);
            
            //Redraw obstacles
            Set_msg(msg_float_out, 'o', (float)size.height, (float)size.width);
            write(fd_r_obstacle, &msg_float_out, sizeof(msg_float_out));
            read(fd_w_obstacle, obstacle, sizeof(obstacle));
            PrintObstacle(my_win, obstacle, size.height, size.width);
            
            //Redraw targets
            Set_msg(msg_float_out, 't', size.height, size.width);
            write(fd_r_target, &msg_float_out, sizeof(msg_float_out));
            write(fd_r_target, obstacle, sizeof(obstacle));
            read_exact(fd_w_target, target, sizeof(target));
            redraw_target = true;
            
            redraw_drone = true;

            reposition = 0;
        }

        if (redraw_target)
        {
            PrintTarget(my_win, target, size.height, size.width, targetReached);
            redraw_target = false;
        }

        if(redraw_drone)
        {
            //Print the drone
            mvwaddch(my_win, yDrone, xDrone, skin | COLOR_PAIR(1));
            wrefresh(my_win);
            refresh();
            redraw_drone = false;
        }

        //Print the values
        move(0, 0);
        clrtoeol();
        printw("Fx: %.3f\tFy: %.3f\tx: %.3f\ty: %.3f\tF_obstacle_X: %.3f\tF_obstacle_Y: %.3f\tScore: %d  ", Fx, Fy, xDrone, yDrone, F_obstacle_X, F_obstacle_Y, score);
        refresh();

        //Waiting
        retval = select(max_fd + 1, &r_fds, NULL, NULL, NULL);


        if (retval == -1)       //Error in select
        {
            if (errno == EINTR)         //Check if is due to a signal from watchdog
            {
                continue;
            }
            else
            {
                log_error("Error in the select");
                perror("select()");
            }
        }
        else if (retval) 
        {
            //Input want to write the new forces
            if(FD_ISSET(fd_w_input, &r_fds))
            {
                read(fd_w_input, &msg_int_in, sizeof(msg_int_in));
                switch(msg_int_in.type)
                {
                    case 'q':           //The user want to exit
                        Set_msg(msg_float_out, 'q', 0, 0);
                        write(fd_r_drone, &msg_float_out, sizeof(msg_float_out));
                        write(fd_r_obstacle, &msg_float_out, sizeof(msg_float_out));
                        write(fd_r_target, &msg_float_out, sizeof(msg_float_out));
                        kill(watchdogPid, SIG_STOP);
                        isExiting = true;
                        break;
                    case 'f':           //New forces
                        Fx += (float)msg_int_in.a;
                        Fy += (float)msg_int_in.b;
                        break;
                    case 'b':           //Brake
                        Fx = Fx * 0.5;
                        Fy = Fy * 0.5;
                        log_debug("Forces after breaking: %f %f", Fx, Fy);
                        if (abs(Fx) < 0.5) Fx = 0;
                        if (abs(Fy) < 0.5) Fy = 0;
                        break;
                    default:
                        log_error("Wrog format recived from the input");
                        perror("Error msg from input");
                        break;
                }

                if (isExiting) break;

            }

            //The drone wants to know the forces applied
            if(FD_ISSET(fd_w_drone, &r_fds))
            {
                //Update the position of the drone
                read(fd_w_drone, &msg_float_in, sizeof(msg_float_in));
                mvwprintw(my_win, yDrone, xDrone, " ");
                xDrone = msg_float_in.a;
                yDrone = msg_float_in.b;

                mvwaddch(my_win, yDrone, xDrone, skin | COLOR_PAIR(1));
                wrefresh(my_win);

                //If the size of the window changed send to drone the new dimensions
                if(sizeChanged)
                {
                    log_debug("size Changed");
                    Set_msg(msg_float_out, 's', (float)size.height, (float)size.width);
                    write(fd_r_drone, &msg_float_out, sizeof(msg_float_out));
                    sizeChanged = false;
                }

                //Ask the forces applied by the obstacles to the drone
                Set_msg(msg_float_out, 'f', xDrone, yDrone);
                write(fd_r_obstacle, &msg_float_out, sizeof(msg_float_out));
                read(fd_w_obstacle, &msg_float_in, sizeof(msg_float_in));
                F_obstacle_X = msg_float_in.a;
                F_obstacle_Y = msg_float_in.b;

                //Ask the forces applied by the targets to the drone
                Set_msg(msg_float_out, 'f', xDrone, yDrone);
                write(fd_r_target, &msg_float_out, sizeof(msg_float_out));
                read(fd_w_target, &msg_float_in, sizeof(msg_float_in));
                F_target_X = msg_float_in.a;
                F_target_Y = msg_float_in.b;
                
                if(msg_float_in.type == 'w')            //Win
                {
                    score++;
                    targetReached++;
                }
                if(msg_float_in.type == 'l' && firstLoss)       //Lose
                {
                    score--;
                    firstLoss = false;
                    log_error("Diminuito score");
                }
                if(msg_float_in.type == 'a')                    //After lose
                {
                    redraw_target = true;
                    redraw_drone = true;
                    firstLoss = true;
                }
                log_warn("Score in BB: %d", score);

                //Sum all the forces and send them to the drone
                //Fx += msg_float_in.a;
                //Fy += msg_float_in.b;
                //Set_msg(msg_float_out, 'f', Fx, Fy);
                
                float totFx = Fx + F_obstacle_X + F_target_X;
                float totFy = Fy + F_obstacle_Y + F_target_Y;
                log_debug("Total Forces: %f %f Target: %f %f", totFx, totFy, F_target_X, F_target_Y);
                Set_msg(msg_float_out, 'f', totFx, totFy);
                write(fd_r_drone, &msg_float_out, sizeof(msg_float_out));
            }

        }
        else printf("No data within five seconds.\n");
    }

    close(fd_r_drone);
    close(fd_w_drone);
    close(fd_w_input);
    close(fd_r_obstacle);
    close(fd_w_obstacle);
    close(fd_r_target);
    close(fd_w_target);
    endwin();
    return 0;

}