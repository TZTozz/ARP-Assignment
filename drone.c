#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>

#define init_x 1    //Initial position x
#define init_y 1    //Initial position y
#define T 10        //Integration time value

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

typedef struct
{
    int x;
    int y;
    int Fx;
    int Fy;
    int x_1;
    int x_2;
    int y_1;
    int y_2;
    const char *skin;
}drone;

/// @brief Calcola la nuova posizione del drone sulla base delle forze che riceve come input e calcola le forze
///         sulla base delle posizioni degli ostacoli e target
/// @param argc 
/// @param argv 
/// @return 
int main(int argc, char *argv[])
{
    int startx, starty, width, height;
    char str[80], ch;
    
    int fd = atoi(argv[1]);

    WINDOW *my_win;

    drone drn;
    drn.skin = "+";

    //Initializzation drone proprieties
    drn.x=init_x;
    drn.y=init_y;
    drn.Fx=0;
    drn.Fy=0;
    drn.x_1=init_x;
    drn.y_1=init_y;
    drn.x_2=init_x;
    drn.y_2=init_y;
    
	
    initscr(); 
    cbreak();
    noecho();
    	
    keypad(stdscr, TRUE);
    curs_set(0);
    	
    height = 20;
    width = 100;
    starty = (LINES - height) / 2;	
	startx = (COLS - width) / 2;
	
    printw("Press q to quit");
	refresh();

    my_win = create_newwin(height, width, starty, startx);
    mvwprintw(my_win, drn.y, drn.x, "%s", drn.skin);
    wrefresh(my_win);

    while(1)
    {
        read(fd, str, sizeof(str));
        if (str[0]=='q') break;
        sscanf(str, "%d %d", &drn.Fx, &drn.Fy);
        mvwprintw(my_win, drn.y, drn.x, " ");
        drn.x_2=drn.x_1;
        drn.y_2=drn.y_1;
        drn.x_1 = drn.x;
        drn.y_1 = drn.y;
        drn.x=(T*T*drn.Fx-drn.x_2+(2+T)*drn.x_1)/(T+1);
        drn.y=(T*T*drn.Fy-drn.y_2+(2+T)*drn.y_1)/(T+1);
        //drn.x += drn.Fx;
        //drn.y += drn.Fy;
        if (drn.x < 1) drn.x = 1;
        if (drn.x > width - 2) drn.x = width - 2;
        if (drn.y < 1) drn.y = 1;
        if (drn.y > height - 2) drn.y = height - 2;
        mvwprintw(my_win, drn.y, drn.x, "%s", drn.skin);
        wrefresh(my_win);
        usleep(100);
    }
    endwin();
    close(fd);
    return 0;
}