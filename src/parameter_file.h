#ifndef parameter
#define parameter_file


#define skin '+'
#define MaxHeight 80
#define MaxWidth 270
#define densityObstacles 0.002
#define densityTargets 0.002


#define init_x 10        //Initial position x
#define init_y 10       //Initial position y

#define T 0.1           //Integration time value (s)
#define K 5             //Air coefficient
#define eta 10        //Coefficient Latombe formula
#define rho 5           //Distance of sensibility obstacles
#define MaxAttraction 10     //Max repulsive force by the obstacles

#define SleepTime 10000
#define FILENAME_PID "../files/PID_file"
#define SIG_HEARTBEAT SIGRTMIN
#define SIG_WRITTEN   (SIGRTMIN + 1)
#define SIG_STOP      (SIGRTMIN + 2)

typedef struct{
    char type;   // 'f', 's', 'q'
    int  a;
    int  b;
}Msg_int;

typedef struct{
    char type;   // 'f', 's', 'q'
    float  a;
    float  b;
}Msg_float;


#define Set_msg(msg, t, x, y) {(msg).type = (t); (msg).a = (x); (msg).b = y;}


typedef struct{
    int width;
    int height;
}winDimension;

typedef struct
{
    float x;
    float y;
    float Fx;
    float Fy;
    float x_1;
    float x_2;
    float y_1;
    float y_2;
}drone;


#endif