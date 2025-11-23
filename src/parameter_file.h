#ifndef parameter
#define parameter_file


#define skin "+"
#define MaxHeight 80
#define MaxWidth 270
#define density 0.01

#define init_x 1    //Initial position x
#define init_y 1    //Initial position y
#define T 0.5        //Integration time value (s)


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
    int Fx;
    int Fy;
    float x_1;
    float x_2;
    float y_1;
    float y_2;
}drone;



#endif