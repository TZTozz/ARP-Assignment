#include <ncurses.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include "logger.h"
#include "parameter_file.h"
#include <signal.h>

volatile sig_atomic_t need_resize = 0;

int max(int a, int b) 
{
    return (a > b) ? a : b;
}

WINDOW *create_newwin(int height, int width, int starty, int startx){	

	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0);		
					
					
	wrefresh(local_win);		

	return local_win;
}


void destroy_win(WINDOW *local_win){	
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	wrefresh(local_win);
	delwin(local_win);
}

static winDimension layout_and_draw(WINDOW *win) {
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

void PrintObject(WINDOW *win, bool array[][MaxWidth], int height, int width, bool isTarget)
{
    for(int i = 0; i < height - 2; i++)
    {
        for(int j = 0; j < width - 2; j++)
        {
            if (array[i][j]) 
            {
                if(isTarget)
                {
                    mvwprintw(win, i, j, "#");
                }
                else mvwprintw(win, i, j, "0");
            }   
        }
    }

}

void handle_winch(int sig) {
    need_resize = 1;
}

int main(int argc, char *argv[])
{
    //Handles the pipes
    int fd_r_drone, fd_w_drone, fd_w_input, fd_r_obstacle, fd_w_obstacle, fd_r_target, fd_w_target;
    sscanf(argv[1], "%d %d %d %d %d %d %d", &fd_r_drone, &fd_w_drone,
                                            &fd_w_input,
                                            &fd_r_obstacle, &fd_w_obstacle,
                                            &fd_r_target, &fd_w_target);
    
    log_config("simple.log", LOG_DEBUG);
    
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
    
    bool isExiting = false;

    //Obstacle and target array
    bool obstacle[MaxHeight][MaxWidth]; 
    bool target[MaxHeight][MaxWidth];
    
    //Window setting
    initscr(); 
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    setlocale(LC_ALL, "C");
    
    //Signal for the risize 
    signal(SIGWINCH, handle_winch);
    
    size.height = 20;
    size.width = 100;
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
    read(fd_w_obstacle, obstacle, sizeof(obstacle));
    PrintObject(my_win, obstacle, size.height, size.width, false);
    

    //Communicate to targets the window's dimensions and prints the targets
    Set_msg(msg_float_out, 't', size.height, size.width);
    write(fd_r_target, &msg_float_out, sizeof(msg_float_out));
    write(fd_r_target, obstacle, sizeof(obstacle));
    read(fd_w_target, target, sizeof(target));
    PrintObject(my_win, target, size.height, size.width, true);

    //Print the drone in the initial position
    mvwprintw(my_win, yDrone, xDrone, "%s", skin);
    wrefresh(my_win);

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

            //Redraw obstacles
            Set_msg(msg_float_out, 'o', (float)size.height, (float)size.width);
            write(fd_r_obstacle, &msg_float_out, sizeof(msg_float_out));
            read(fd_w_obstacle, obstacle, sizeof(obstacle));
            PrintObject(my_win, obstacle, size.height, size.width, false);

            //Redraw targets
            Set_msg(msg_float_out, 't', size.height, size.width);
            write(fd_r_target, &msg_float_out, sizeof(msg_float_out));
            write(fd_r_target, obstacle, sizeof(obstacle));
            read(fd_w_target, target, sizeof(target));
            PrintObject(my_win, target, size.height, size.width, true);

            //Print the drone
            mvwprintw(my_win, yDrone, xDrone, skin);
            wrefresh(my_win);
            sizeChanged = true;
        }

        //Waiting
        retval = select(max_fd + 1, &r_fds, NULL, NULL, NULL);


        if (retval == -1) 
        {
            log_error("Error in the select");
            perror("select()");
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

                mvwprintw(my_win, yDrone, xDrone, skin);
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

                //Sum all the forces and send them to the drone
                Fx += msg_float_in.a;
                Fy += msg_float_in.b;
                log_debug("Total forces: %f %f", Fx, Fy);
                Set_msg(msg_float_out, 'f', Fx, Fy);
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