//#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "logger.h"
#include "parameter_file.h"

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
    
    Msg_int msg_int_in;
    Msg_float msg_float_in, msg_float_out;
    //char request[80] = "request";

    winDimension size;
    size.height = 20;
    size.width = 100;

    drone drn;
    
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
    
    log_debug("Parto");
	
    //Legge dimensioni iniziali della finestra
	read(fd_r_drone, &msg_int_in, sizeof(msg_int_in));
    size.height = msg_int_in.a;
    size.width = msg_int_in.b;
    log_debug("Ho letto");

    //printw("Press q to quit");
	//refresh();

    log_debug("Inizio il ciclo");

    while(1)
    {
        Set_msg(msg_float_out, 'f', drn.x, drn.y);
        write(fd_w_drone, &msg_float_out, sizeof(msg_float_out));
        read(fd_r_drone, &msg_float_in, sizeof(msg_float_in));
        log_debug("Ch = %c", msg_float_in.type);
        switch(msg_float_in.type)
        {

            case 'q': 
                log_warn("Caso q");
                exiting = true;
                break;
            case 'f':
                drn.Fx = msg_float_in.a;
                drn.Fy = msg_float_in.b;
                break; 
            case 's':
                log_debug("caso s");
                size.height = msg_float_in.a;
                size.width = msg_float_in.b;
                read(fd_r_drone, &msg_float_in, sizeof(msg_float_in));
                //log_debug("Letto forza post resize %c", msg_int_in.type);
                drn.Fx = msg_float_in.a;
                drn.Fy = msg_float_in.b;
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