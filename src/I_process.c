#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"
#include "parameter_file.h"


void PrintKeyboard(WINDOW *win, int start_y, int start_x) {
    char keys[3][3] = {
        {'W', 'E', 'R'},
        {'S', 'D', 'F'},
        {'X', 'C', 'V'}
    };

    int box_h = 2; 
    int box_w = 4; 

    //Draw grid
    for (int i = 0; i <= 3; i++) {
        mvwhline(win, start_y + (i * box_h), start_x, ACS_HLINE, (3 * box_w) + 1);
    }
    for (int j = 0; j <= 3; j++) {
        mvwvline(win, start_y, start_x + (j * box_w), ACS_VLINE, (3 * box_h) + 1);
    }

    //Handles the intersections
    for (int i = 0; i <= 3; i++) {
        for (int j = 0; j <= 3; j++) {
            int y = start_y + (i * box_h);
            int x = start_x + (j * box_w);
            chtype ch;

            if (i == 0 && j == 0) ch = ACS_ULCORNER;
            else if (i == 0 && j == 3) ch = ACS_URCORNER;
            else if (i == 3 && j == 0) ch = ACS_LLCORNER;
            else if (i == 3 && j == 3) ch = ACS_LRCORNER;
            else if (i == 0) ch = ACS_TTEE;
            else if (i == 3) ch = ACS_BTEE;
            else if (j == 0) ch = ACS_LTEE;
            else if (j == 3) ch = ACS_RTEE;
            else ch = ACS_PLUS;

            mvwaddch(win, y, x, ch);
        }
    }

    //Prints the letters
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int y_pos = start_y + (i * box_h) + 1;
            int x_pos = start_x + (j * box_w) + 2;
            
            wattron(win, COLOR_PAIR(1) | A_BOLD); 
            mvwaddch(win, y_pos, x_pos, keys[i][j]);
            wattroff(win, COLOR_PAIR(1) | A_BOLD);
        }
    }
}

int main(int argc, char *argv[]) {

    //Handles the pipes
    int fd_w_input;
    sscanf(argv[1], "%d", &fd_w_input);

    log_config("simple.log", LOG_DEBUG);

    //Protocol variable 
    Msg_int msg_int_out;

	int ch, Fx, Fy;

    //Inizialize the forces
	Fx = 0;
	Fy = 0;

    //Flags
    bool wrongKey = false;
    bool isBreaking = false;

	//Window setting
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    setlocale(LC_ALL, "C");
    init_pair(1, COLOR_YELLOW, -1);
    WINDOW *my_win = newwin(10, 20, 5, 5);
    refresh();
    
    //Print the instruction
    PrintKeyboard(my_win, 1, 1);
    printw("Press q to quit\n");
    wrefresh(my_win);

    while((ch = getch()) != 'q')
    {
        switch(ch)
        {
            case 'e':       //Up
                Fx = 0;
                Fy = -1;
                break;
            case 'r':       //Up-right
                Fx = 1;
                Fy = -1;
                break;
            case 'f':       //Right
                Fx = 1;
                Fy = 0;
                break;
            case 'v':       //Down-right
                Fx = 1;
                Fy = +1;
                break;
            case 'c':       //Down
                Fx = 0;
                Fy = +1;
                break;
            case 'x':       //Down-left
                Fx = -1;
                Fy = +1;
                break;		
            case 's':       //Left
                Fx = -1;
                Fy = 0;
                break;
            case 'w':       //Up-left
                Fx = -1;
                Fy = -1;
                break;
            case 'd':       //Brake
                isBreaking = true;
                break;
            case KEY_RESIZE:
                refresh();
                PrintKeyboard(my_win, 1, 1);
                wrefresh(my_win);
                break;
            default:
                wrongKey = true;
                break;
        }
        
        if(!wrongKey)
        {
            if(isBreaking)
            {
                Set_msg(msg_int_out, 'b', 0, 0);
                log_debug("Breaking!");
            } 
            else 
            {
                Set_msg(msg_int_out, 'f', Fx, Fy);
                log_debug("New forces: %d %d", Fx, Fy);
            }
            
            write(fd_w_input, &msg_int_out, sizeof(msg_int_out));
        }
        wrongKey = false;
        isBreaking = false;
        
    }
    
    //Communicates to the BB the user wants to quit
    Set_msg(msg_int_out, 'q', 0, 0);
    write(fd_w_input, &msg_int_out, sizeof(msg_int_out));

    endwin();
    close(fd_w_input);           
    return 0;
}