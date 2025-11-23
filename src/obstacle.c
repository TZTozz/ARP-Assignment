#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "logger.h"
#include "parameter_file.h"



void ClearArray(bool array[MaxHeight][MaxWidth])
{
    for(int i; i < MaxHeight; i++)
    {
        for(int j; j < MaxWidth; j++)
        {
            array[i][j] = false;
        }
    }
}

/// @brief Stampa in posizioni random gli ostacoli sulla base 
/// @param array 
/// @param height 
/// @param width 
void Positioning(bool array[][MaxWidth], int height, int width)
{
    int dim = (height - 2) * (width - 2);        //-3 cause the obstacles must be inside the window
    int numObstacle = (int)(density * dim);

    log_debug("Width: %d, Height: %d", width, height);
    bool vector[dim];
    //Inizializza il vettore a tutti false
    for(int i = 0; i < dim; i++)
    {
        vector[i] = false;
    }

    //Ciclo che riempie il vettore con un numero di ostacoli pari a numObstacle 
    int j;
    for(int i = 0; i < numObstacle; i++)
    {
        do
        {
            j = rand() % dim;

        }while(vector[j]);
        vector[j] = true;
    }

    //Trasforma il vettore in una matrice
    for (int i = 0; i < dim; i++) 
    {
        int r = i / (width - 2);  // indice riga
        int c = i % (width - 2);  // indice colonna
        array[r+1][c+1] = vector[i];
    }

}

int main(int argc, char *argv[])
{
    int fd_r_obstacle, fd_w_obstacle;
    sscanf(argv[1], "%d %d", &fd_r_obstacle, &fd_w_obstacle);

    log_config("simple.log", LOG_DEBUG);
    
    bool obstacle[MaxHeight][MaxWidth];     //Max dimension of the screen
    
    bool exiting = false;

    Msg_int msg_int_in;
    int h_Win, w_Win;

    //ClearArray(obstacle);
    //strcpy(message_out, "request");
    //write(fd_w_obstacle, message_out, strlen(message_out) + 1);
    
    while(1)
    {
        read(fd_r_obstacle, &msg_int_in, sizeof(msg_int_in));
        log_debug("The char is %c", msg_int_in.type);
        switch (msg_int_in.type)
        {
            case 'q':
                exiting = true;
                break;
            case 'o':       //La BB vuole la posizione degli ostacoli
                h_Win = msg_int_in.a;
                w_Win = msg_int_in.b;
                ClearArray(obstacle);
                Positioning(obstacle, h_Win, w_Win);
                write(fd_w_obstacle, obstacle, sizeof(obstacle));
                break;
            case 'f':       //La BB vuole le forze che gli ostacoli applicano sul drone
                break;
            default:
                log_error("ERRORE nel formato");
                perror("Format not correct");
                break;
        }

        if (exiting) break;

        usleep(100000);
        
    }

    close(fd_r_obstacle);
    close(fd_w_obstacle);
    return 0;

}