#include <ncurses.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include "logger.h"
#include <signal.h>

#define skin "+"

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

static void layout_and_draw(WINDOW *win) {
    int H, W;
    getmaxyx(stdscr, H, W);

    // Finestra con un margine fisso intorno
    int wh = (H > 6) ? H - 6 : H;
    int ww = (W > 10) ? W - 10 : W;
    if (wh < 3) wh = 3;
    if (ww < 3) ww = 3;

    // Ridimensiona e ricentra la finestra
    wresize(win, wh, ww);
    mvwin(win, (H - wh) / 2, (W - ww) / 2);

    // Pulisci e ridisegna
    werase(stdscr);
    werase(win);
    box(win, 0, 0);
    //mvprintw(0, 0, "Ridimensiona la shell; premi 'q' per uscire.");
    //mvwprintw(win, 1, 2, "stdscr: %dx%d  win: %dx%d",
    //          H, W, wh, ww);

    refresh();
    wrefresh(win);
}

void handle_winch(int sig) {
    need_resize = 1;
}

int main(int argc, char *argv[])
{
    int fd_r_drone, fd_w_drone, fd_w_input;
    sscanf(argv[1], "%d %d %d", &fd_r_drone, &fd_w_drone, &fd_w_input);
    
    log_config("simple.log", LOG_DEBUG);
    
    int max_fd = max(fd_w_drone, fd_w_input);
    int startx, starty, width, height;
    
    char message_in[80], message_out[80];
    
    fd_set r_fds;
    int retval;
    char line[80], ch;
    
    //Blackboard data
    float xDrone = 1, yDrone = 1;
    int Fx = 0, Fy = 0;
    
    initscr(); 
    cbreak();
    noecho();
    
    signal(SIGWINCH, handle_winch);
    
    keypad(stdscr, TRUE);
    curs_set(0);
    
    setlocale(LC_ALL, "");

    WINDOW *my_win;
    
    height = 20;
    width = 100;
    starty = (LINES - height) / 2;	
	startx = (COLS - width) / 2;
    
    my_win = create_newwin(height, width, starty, startx);
    layout_and_draw(my_win);
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
            layout_and_draw(my_win);       // ricalcola layout e ridisegna
        }

        //Rimane in attesa 
        retval = select(max_fd + 1, &r_fds, NULL, NULL, NULL);


        if (retval == -1) perror("select()");
        else if (retval) 
        {
            if(FD_ISSET(fd_w_input, &r_fds))        //Input vuole scrivere le nuove forze
            {
                read(fd_w_input, message_in, sizeof(message_in));
                if(message_in[0]=='q') 
                {
                    write(fd_r_drone, "quite", strlen("quite") + 1);
                    break;
                }
                sscanf(message_in, "%d %d", &Fx, &Fy);
                log_debug("Leggo input: Fx: %d Fy: %d", Fx, Fy);

            }

            if(FD_ISSET(fd_w_drone, &r_fds))        //Il drone vuole conoscere le forze
            {
                read(fd_w_drone, message_in, sizeof(message_in));
                mvwprintw(my_win, yDrone, xDrone, " ");
                sscanf(message_in, "%f %f", &xDrone, &yDrone);
                log_debug("Position X: %f Y: %f", xDrone, yDrone);

                mvwprintw(my_win, yDrone, xDrone, skin);
                wrefresh(my_win);
                sprintf(message_out, "f %d %d", Fx, Fy);
                write(fd_r_drone, message_out, strlen(message_out) + 1);

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