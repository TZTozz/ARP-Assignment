#include <ncurses.h>


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

int main() {

	int x, y, startx, starty, width, height;;
	int ch;
	
	WINDOW *my_win;
	
    initscr(); 
    cbreak();
    noecho();
    
    keypad(stdscr, TRUE);
    
    height = 15;
    width = 25;
    starty = (LINES - height) / 2;	
	startx = (COLS - width) / 2;
	
	x = 1;
	y = 1;

    curs_set(0);
	
	printw("Press F1 to exit");
	refresh();
	
	my_win = create_newwin(height, width, starty, startx);
    
    mvwprintw(my_win, y, x, "+");
    refresh();
    wrefresh(my_win);

    while((ch = getch()) != KEY_F(1))
    {
        switch(ch)
        {
            case 'e':       //Up
                mvwprintw(my_win, y, x, " ");
                y--;
                break;
            case 'r':       //Up-right
                mvwprintw(my_win, y, x, " ");
                x++;
                y--;
                break;
            case 'f':       //Right
                mvwprintw(my_win, y, x, " ");
                x++;
                break;
            case 'v':       //Down-right
                mvwprintw(my_win, y, x, " ");
                x++;
                y++;
                break;
            case 'c':       //Down
                mvwprintw(my_win, y, x, " ");
                y++;
                break;
            case 'x':       //Down-right
                mvwprintw(my_win, y, x, " ");
                x--;
                y++;
                break;		
            case 's':       //Left
                mvwprintw(my_win, y, x, " ");
                x--;
                break;
            case 'w':       //Left
                mvwprintw(my_win, y, x, " ");
                x--;
                y--;
                break;
        }
            
        mvwprintw(my_win, y, x, "+");
        refresh();
        wrefresh(my_win);		
    		 
    	}
    	
    	destroy_win(my_win);
      	endwin();           
    	return 0;
}