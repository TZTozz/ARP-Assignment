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
    for(int i = 0; i < MaxHeight; i++)
    {
        for(int j = 0; j < MaxWidth; j++)
        {
            array[i][j] = false;
        }
    }
}


void ObstacleRepulsion(bool array[][MaxWidth], float x, float y, float *Fx, float *Fy)
{
    int min_c = (int)(x - rho);
    int max_c = (int)(x + rho);
    int min_r = (int)(y - rho);
    int max_r = (int)(y + rho);

    if (min_c < 0) min_c = 0;
    //if (max_c >= MaxWidth) max_c = MaxWidth - 1;
    if (min_r < 0) min_r = 0;
    
    for (int r = min_r; r <= max_r; r++)
    {
        for (int c = min_c; c <= max_c; c++)
        {
            if (array[r][c])
            {
                float dx = x - c; 
                float dy = y - r;

                float d = sqrtf(dx*dx + dy*dy);

                //To avoid division per zero
                if (d < 0.001f) d = 0.001f;

                if (d < rho)
                {
                    // Formula Latombe
                    // F = eta * (1/d - 1/rho) * (1/d^2)
                    float term1 = (1.0f / d) - (1.0f / rho);
                    float F = eta * term1 * (1.0f / (d * d));

                    // Proiezione vettoriale (molto più robusta della pendenza m):
                    // Fx = Magnitude * (dx / d) -> dove (dx/d) è il coseno direttore
                    // Fy = Magnitude * (dy / d) -> dove (dy/d) è il seno direttore
                    
                    float fx = F * (dx / d);
                    float fy = F * (dy / d);

                    if (abs(F) > MaxRepulsive)
                    {
                        float scale = MaxRepulsive / F;
                        fx = fx * scale;
                        fy = fy * scale;
                    }

                    log_debug("Obstacle at [%d,%d] Dist: %f -> F: %f %f", r, c, d, fx, fy);
                    
                    *Fx += fx;
                    *Fy += fy;
                }
            }
        }
    }
}

void WallRepulsion(float x, float y, float *Fx, float *Fy, int height, int width)
{
    //Left wall
    if (x - 1 < rho)
    {
        float d = x - 1;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("Thedistance is %f and the force is %f", d, F);
        //Sum to total force
        *Fx += F;
    }

    //Right wall
    if ((width - x - 2) < rho)
    {
        float d = width - x- 2;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("Thedistance is %f and the force is %f", d, F);

        //Sum to total force
        *Fx -= F;
    }

    //Upper wall
    if (y - 1 < rho)
    {
        float d = y - 1;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("Thedistance is %f and the force is %f", d, F);

        //Sum to total force
        *Fy += F;
    }

    //Bottom wall
    if ((height - y - 2) < rho)
    {
        float d = height - y - 2;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("Thedistance is %f and the force is %f", d, F);

        //Sum to total force
        *Fy -= F;
    }
}


/// @brief Stampa in posizioni random gli ostacoli sulla base 
/// @param array 
/// @param height 
/// @param width 
void Positioning(bool array[][MaxWidth], int height, int width)
{
    int dim = (height - 2) * (width - 2);        //-2 cause the obstacles must be inside the window
    int numObstacle = (int)(densityObstacles * dim);

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

    Msg_int msg_int_out;
    Msg_float msg_float_in, msg_float_out;
    int h_Win, w_Win;

    float Fx, Fy;

    //ClearArray(obstacle);
    //strcpy(message_out, "request");
    //write(fd_w_obstacle, message_out, strlen(message_out) + 1);
    
    while(1)
    {
        read(fd_r_obstacle, &msg_float_in, sizeof(msg_float_in));
        log_debug("The char is %c", msg_float_in.type);
        switch (msg_float_in.type)
        {
            case 'q':       //Quitting
                exiting = true;
                break;
            case 'o':       //La BB vuole la posizione degli ostacoli
                h_Win = (int)msg_float_in.a;
                w_Win = (int)msg_float_in.b;
                log_debug("Dimesioni finestra: %d, %d", h_Win, w_Win);
                ClearArray(obstacle);
                Positioning(obstacle, h_Win, w_Win);
                write(fd_w_obstacle, obstacle, sizeof(obstacle));
                break;
            case 'f':       //La BB vuole le forze che gli ostacoli applicano sul drone
                Fx = 0;
                Fy = 0;
                ObstacleRepulsion(obstacle, msg_float_in.a, msg_float_in.b, &Fx, &Fy);
                log_debug("Forze dopo ostacoli: %f %f", Fx, Fy);
                WallRepulsion(msg_float_in.a, msg_float_in.b, &Fx, &Fy, h_Win, w_Win);
                log_debug("Forze dopo wall: %f %f", Fx, Fy);
                Set_msg(msg_float_out, 'f', Fx, Fy);
                write(fd_w_obstacle, &msg_float_out, sizeof(msg_float_out));
                break;
            default:
                log_error("ERRORE nel formato");
                perror("Format not correct");
                break;
        }

        if (exiting) break;
        
    }

    close(fd_r_obstacle);
    close(fd_w_obstacle);
    return 0;

}