#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

    int fd_r_input, fd_w_input;
    sprintf(argv[1], "%d %d", fd_r_input, fd_w_input);

    char str[80];
	int ch, ax, ay;

	
	ax = 0;
	ay = 0;

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
                ay--;
                break;
            case 'r':       //Up-right
                ax++;
                ay--;
                break;
            case 'f':       //Right
                ax++;
                break;
            case 'v':       //Down-right
                ax++;
                ay++;
                break;
            case 'c':       //Down
                ay++;
                break;
            case 'x':       //Down-right
                ax--;
                ay++;
                break;		
            case 's':       //Left
                ax--;
                break;
            case 'w':       //Up-left
                ax--;
                ay--;
                break;
            case 'd':       //Brake
                ax = 0;
                ay = 0;
                break;
            default:
                wrongKey = true;
                break;
        }
        
        if(!wrongKey)
        {
            sprintf(str, "%d %d", ax, ay);
            write(fd_w_input, str, strlen(str) + 1);
        }
        wrongKey = false;
        
    }
    
    strcpy(str, "quit");
    write(fd_w_input, str, strlen(str) + 1);

    endwin();
    close(fd_r_input);
    close(fd_w_input);           
    return 0;
}