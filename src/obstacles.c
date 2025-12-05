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
    if (min_r < 0) min_r = 0;
    
    for (int r = min_r; r <= max_r; r++)
    {
        for (int c = min_c; c <= max_c; c++)
        {
            if (array[r][c])
            {
                float dx = x - (c + 0.5);   //The obstacles is in the center of char space
                float dy = y - (r + 0.5);   //The obstacles is in the center of char space

                float d = sqrtf(dx*dx + dy*dy);

                //To avoid division per zero
                if (d < 0.001f) d = 0.001f;

                if (d < rho)
                {
                    //Latombe's formula
                    //F = eta * (1/d - 1/rho) * (1/d^2)
                    float term1 = (1.0f / d) - (1.0f / rho);
                    float F = eta * term1 * (1.0f / (d * d));

                    //(dx/d) is the cosine
                    //(dy/d) is the sine
                    
                    float fx = F * (dx / d);
                    float fy = F * (dy / d);

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
    if (x - 0.9 < rho)
    {
        float d = x - 0.9;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        //if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("The d from wall LEFT is %f and the force is %f", d, F);

        //Sum to total force
        *Fx += F;
    }

    //Right wall
    if ((width - x - 1.9) < rho)
    {
        float d = width - x - 1.9;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        //if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("The d from wall RIGHT is %f and the force is %f", d, F);

        //Sum to total force
        *Fx -= F;
    }

    //Upper wall
    if (y - 0.9 < rho)
    {
        float d = y - 0.9;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        //if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("The d from wall UP is %f and the force is %f", d, F);

        //Sum to total force
        *Fy += F;
    }

    //Bottom wall
    if ((height - y - 1.9) < rho)
    {
        float d = height - y - 1.9;
        if (d < 0.001f) d = 0.001f;

        //Calculate relative distance 
        float term1 = (1.0f / d) - (1.0f / rho);
        float F = eta * term1 * (1.0f / (d * d));

        //if (F > MaxRepulsive) F = MaxRepulsive;
        log_debug("The dfrom wall BOTTOM is %f and the force is %f", d, F);

        //Sum to total force
        *Fy -= F;
    }
}


/// @brief Prints obstacles in random positions 
/// @param array 
/// @param height 
/// @param width 
void Positioning(bool array[][MaxWidth], int height, int width)
{
    int dim = (height - 2) * (width - 2);        //-2 cause the obstacles must be inside the window
    int numObstacle = (int)(densityObstacles * dim);

    log_debug("Width: %d, Height: %d", width, height);
    bool vector[dim];

    //Inizialize the vector with all false
    for(int i = 0; i < dim; i++)
    {
        vector[i] = false;
    }

    //Fill the vector with a number of obstacles as numObstacle 
    int j;
    for(int i = 0; i < numObstacle; i++)
    {
        do
        {
            j = rand() % dim;

        }while(vector[j]);      //Check for another obtacle in that position
        vector[j] = true;
    }

    //From vector to matrix
    for (int i = 0; i < dim; i++) 
    {
        int r = i / (width - 2);  //Row
        int c = i % (width - 2);  //Column
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
    
    while(1)
    {
        read(fd_r_obstacle, &msg_float_in, sizeof(msg_float_in));
        log_debug("The char is %c", msg_float_in.type);
        switch (msg_float_in.type)
        {
            case 'q':       //Quitting
                exiting = true;
                break;
            case 'o':       //The BB wants to know the positions of the obstacles
                h_Win = (int)msg_float_in.a;
                w_Win = (int)msg_float_in.b;
                log_debug("Window dimension: %d, %d", h_Win, w_Win);
                ClearArray(obstacle);
                Positioning(obstacle, h_Win, w_Win);
                write(fd_w_obstacle, obstacle, sizeof(obstacle));
                break;
            case 'f':       //The BB wants to know the forces that obstacles apply to the drone
                Fx = 0;
                Fy = 0;
                ObstacleRepulsion(obstacle, msg_float_in.a, msg_float_in.b, &Fx, &Fy);
                log_debug("Forces after obstacles: %f %f", Fx, Fy);
                WallRepulsion(msg_float_in.a, msg_float_in.b, &Fx, &Fy, h_Win, w_Win);
                log_debug("Forces after walls: %f %f", Fx, Fy);
                Set_msg(msg_float_out, 'f', Fx, Fy);
                write(fd_w_obstacle, &msg_float_out, sizeof(msg_float_out));
                break;
            default:
                log_error("Error: wrong format of the message recived");
                perror("Format not correct");
                break;
        }

        if (exiting) break;
        
    }

    close(fd_r_obstacle);
    close(fd_w_obstacle);
    return 0;

}