//#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "logger.h"

#define init_x 1    //Initial position x
#define init_y 1    //Initial position y
#define T 0.5        //Integration time value (s)

//DA CAMBIARE!!!! Ma ora non è ancora implementato il resize
typedef struct{
    int width;
    int height;
}winDimension;

typedef struct
{
    float x;
    float y;
    int Fx;
    int Fy;
    float x_1;
    float x_2;
    float y_1;
    float y_2;
    const char *skin;
}drone;

/// @brief Calcola la nuova posizione del drone sulla base delle forze che riceve come input e calcola le forze
///         sulla base delle posizioni degli ostacoli e target
/// @param argc 
/// @param argv 
/// @return 
int main(int argc, char *argv[])
{
    int fd_r_drone, fd_w_drone;
    sscanf(argv[1], "%d %d", &fd_r_drone, &fd_w_drone);

    log_config("simple.log", LOG_DEBUG);
    
    
    char str[80], ch;
    //char request[80] = "request";

    winDimension size;
    size.height = 20;
    size.width = 100;

    drone drn;
    drn.skin = "+";
    
    bool exiting = false;

    //Initializzation drone proprieties
    drn.x=init_x;
    drn.y=init_y;
    drn.Fx=0;
    drn.Fy=0;
    drn.x_1=init_x;
    drn.y_1=init_y;
    drn.x_2=init_x;
    drn.y_2=init_y;
    
	
    //Legge dimensioni iniziali della finestra
	read(fd_r_drone, str, sizeof(str));
    sscanf(str, "s %d %d", &size.height, &size.width);

    //printw("Press q to quit");
	//refresh();

    log_debug("Inizio il ciclo");

    while(1)
    {
        sprintf(str, "%f %f", drn.x, drn.y);
        write(fd_w_drone, str, strlen(str) + 1);
        read(fd_r_drone, str, sizeof(str));
        ch = str[0];
        log_debug("Ch = %c", ch);
        switch(ch)
        {

            case 'q': 
                log_warn("Caso q");
                exiting = true;
                break;
            case 'f':
                sscanf(str, "f %d %d", &drn.Fx, &drn.Fy);
                break; 
            case 's':
                log_warn("caso s");
                sscanf(str, "s %d %d", &size.height, &size.width);
                read(fd_r_drone, str, sizeof(str));
                log_debug("Letto forza post resize %c", str[0]);
                sscanf(str, "f %d %d", &drn.Fx, &drn.Fy);
                break;
            default:
                log_error("ERRORE nel formato");
                perror("Format not correct");
                break;
        }
        
        if (exiting) break;

        
        drn.x_2=drn.x_1;
        drn.y_2=drn.y_1;
        drn.x_1 = drn.x;
        drn.y_1 = drn.y;
        drn.x=(T*T*drn.Fx-drn.x_2+(2+T)*drn.x_1)/(T+1);
        drn.y=(T*T*drn.Fy-drn.y_2+(2+T)*drn.y_1)/(T+1);

        //log_debug("Fx: %d, Fy: %d", drn.Fx, drn.Fy);
        if (drn.x < 1) drn.x = 1;
        if (drn.x > size.width - 2) drn.x = size.width - 2;
        if (drn.y < 1) drn.y = 1;
        if (drn.y > size.height - 2) drn.y = size.height - 2;
        log_debug("Position: X: %f Y: %f", drn.x, drn.y);
        usleep(100000);
    }

    log_debug("Sto uscendo");
    close(fd_r_drone);
    close(fd_w_drone);
    return 0;
}