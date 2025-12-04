#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"
#include "parameter_file.h"

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
    keypad(stdscr, TRUE);

    printw("Controllo drone - Premi q per uscire\n");

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
                log_debug("New forze: %d %d", Fx, Fy);
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