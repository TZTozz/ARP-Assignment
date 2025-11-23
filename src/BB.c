#include <ncurses.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
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

    // Finestra con un margine fisso intorno
    int wh = (H > 6) ? H - 6 : H;
    int ww = (W > 10) ? W - 10 : W;
    if (wh < 3) wh = 3;
    if (ww < 3) ww = 3;

    winDimension size;
    size.height = wh;
    size.width = ww;

    // Ridimensiona e ricentra la finestra
    wresize(win, wh, ww);
    mvwin(win, (H - wh) / 2, (W - ww) / 2);

    // Pulisci e ridisegna
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
            if (array[i][j]) mvwprintw(win, i, j, "0");
        }
    }

}

void handle_winch(int sig) {
    need_resize = 1;
}

int main(int argc, char *argv[])
{
    int fd_r_drone, fd_w_drone, fd_w_input, fd_r_obstacle, fd_w_obstacle;
    sscanf(argv[1], "%d %d %d %d %d", &fd_r_drone, &fd_w_drone, &fd_w_input, &fd_r_obstacle, &fd_w_obstacle);
    
    log_config("simple.log", LOG_DEBUG);
    
    int max_fd = max(fd_w_drone, fd_w_input);
    int startx, starty, width, height;
    bool sizeChanged = false;
    winDimension size;
    
    Msg_int msg_int_in, msg_int_out;
    Msg_float msg_float_in, msg_float_out;
    
    fd_set r_fds;
    int retval;
    //char line[80],
    char ch;
    
    //Blackboard data
    float xDrone = 1, yDrone = 1;
    int Fx = 0, Fy = 0;

    //Obstacle array
    bool obstacle[MaxHeight][MaxWidth]; 
    
    initscr(); 
    cbreak();
    noecho();
    
    signal(SIGWINCH, handle_winch);
    
    keypad(stdscr, TRUE);
    curs_set(0);
    
    setlocale(LC_ALL, "C");

    WINDOW *my_win;
    
    size.height = 20;
    size.width = 100;
    starty = (LINES - size.height) / 2;	
	startx = (COLS - size.width) / 2;
    
    my_win = create_newwin(size.height, size.width, starty, startx);
    size = layout_and_draw(my_win);

    //Comunica al drone la dimensione della finestra
    Set_msg(msg_int_out, 's', size.height, size.width);
    //(msg_int_out, "s %d %d", size.height, size.width);
    write(fd_r_drone, &msg_int_out, sizeof(msg_int_out));

    log_debug("height: %d Width: %d", size.height, size.width);

    //Comunica a obstacle le dimensioni della finestra
    Set_msg(msg_int_out, 'o', size.height, size.width);
    //sprintf(msg_int_out, "o %d %d", size.height, size.width);
    write(fd_r_obstacle, &msg_int_out, sizeof(msg_int_out));
    read(fd_w_obstacle, obstacle, sizeof(obstacle));
    PrintObstacle(my_win, obstacle, size.height, size.width);

    //Stampa il drone nella posizione iniziale
    mvwprintw(my_win, yDrone, xDrone, "%s", skin);
    wrefresh(my_win);

    while (1)
    {
        //Inizializzo i set
        FD_ZERO(&r_fds);
        //Aggiungo i file descriptor
        FD_SET(fd_w_drone, &r_fds);
        FD_SET(fd_w_input, &r_fds);

        if (need_resize) {
            need_resize = 0;
            endwin();
            refresh();
            resize_term(0, 0);          // o resizeterm(0, 0)
            size = layout_and_draw(my_win);       // ricalcola layout e ridisegna

            Set_msg(msg_int_out, 'o', size.height, size.width);
            write(fd_r_obstacle, &msg_int_out, sizeof(msg_int_out));
            read(fd_w_obstacle, obstacle, sizeof(obstacle));
            PrintObstacle(my_win, obstacle, size.height, size.width);
            mvwprintw(my_win, yDrone, xDrone, skin);
            wrefresh(my_win);
            sizeChanged = true;

        }

        //Rimane in attesa 
        retval = select(max_fd + 1, &r_fds, NULL, NULL, NULL);


        if (retval == -1) perror("select()");
        else if (retval) 
        {
            if(FD_ISSET(fd_w_input, &r_fds))        //Input vuole scrivere le nuove forze
            {
                read(fd_w_input, &msg_int_in, sizeof(msg_int_in));
                if(msg_int_in.type == 'q') 
                {
                    Set_msg(msg_int_out, 'q', 0, 0);
                    write(fd_r_drone, &msg_int_out, sizeof(msg_int_out));
                    write(fd_r_obstacle, &msg_int_out, sizeof(msg_int_out));
                    break;
                }
                Fx = msg_int_in.a;
                Fy = msg_int_in.b;
                log_debug("Leggo input: Fx: %d Fy: %d", Fx, Fy);

            }

            if(FD_ISSET(fd_w_drone, &r_fds))        //Il drone vuole leggere
            {
                read(fd_w_drone, &msg_float_in, sizeof(msg_float_in));
                mvwprintw(my_win, yDrone, xDrone, " ");
                xDrone = msg_float_in.a;
                yDrone = msg_float_in.b;

                mvwprintw(my_win, yDrone, xDrone, skin);
                wrefresh(my_win);

                if(sizeChanged)
                {
                    log_debug("size Changed");
                    Set_msg(msg_int_out, 's', size.height, size.width);
                    write(fd_r_drone, &msg_int_out, sizeof(msg_int_out));
                    sizeChanged = false;
                }
                Set_msg(msg_int_out, 'f', Fx, Fy);
                write(fd_r_drone, &msg_int_out, sizeof(msg_int_out));
                log_debug("Ho scritto anche le forze: %c", msg_int_out.type);
            }

        }
        else printf("No data within five seconds.\n");
    }

    close(fd_r_drone);
    close(fd_w_drone);
    close(fd_w_input);
    endwin();
    return 0;

}