#ifndef NETWORK
#define NETWORK

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef struct {
    int x;
    int y;
} Point_INT;

typedef struct {
    float x;
    float y;
} Point_FLOAT;

typedef enum {
    ANGLE_0,
    ANGLE_90,
    ANGLE_NEG_90,
    ANGLE_180
} RotationAngle;

extern Point_INT Origin;
extern RotationAngle angleVirtual; 

typedef enum {
    SINGLE_PLAYER = 0,
    SERVER = 1,
    CLIENT = 2
}GameMode;

Point_INT transform_coordinate_FLOAT(Point_FLOAT input, Point_INT origin, RotationAngle alpha);
Point_INT inverse_transform_coordinate_INT(Point_INT p1, Point_INT origin, RotationAngle alpha);
ssize_t read_line(int fd, char *buf, size_t maxlen);

#endif