#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"
#include "parameter_file.h"

int main(int argc, char *argv[]) {

    int fd_w_input;
    sscanf(argv[1], "%d", &fd_w_input);

    log_config("simple.log", LOG_DEBUG);

    Msg_int msg_int_out;
	int ch, Fx, Fy;

	
	Fx = 0;
	Fy = 0;

    bool wrongKey = false;

	initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    printw("Controllo drone - Premi q per uscire\n");

    while((ch = getch()) != 'q')
    {

        switch(ch)
        {
            case 'e':       //Up
                Fy--;
                break;
            case 'r':       //Up-right
                Fx++;
                Fy--;
                break;
            case 'f':       //Right
                Fx++;
                break;
            case 'v':       //Down-right
                Fx++;
                Fy++;
                break;
            case 'c':       //Down
                Fy++;
                break;
            case 'x':       //Down-left
                Fx--;
                Fy++;
                break;		
            case 's':       //Left
                Fx--;
                break;
            case 'w':       //Up-left
                Fx--;
                Fy--;
                break;
            case 'd':       //Brake
                Fx = Fx * 0.5;
                Fy = Fy * 0.5;
                
                break;
            default:
                wrongKey = true;
                break;
        }
        
        if(!wrongKey)
        {
            Set_msg(msg_int_out, 'f', Fx, Fy);
            log_debug("New forze: %d %d", Fx, Fy);
            write(fd_w_input, &msg_int_out, sizeof(msg_int_out));
        }
        wrongKey = false;
        
    }
    
    Set_msg(msg_int_out, 'q', 0, 0);
    write(fd_w_input, &msg_int_out, sizeof(msg_int_out));

    endwin();
    close(fd_w_input);           
    return 0;
}