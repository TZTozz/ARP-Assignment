#include "Network.h"
#include <unistd.h>



Point_INT transform_coordinate_FLOAT(Point_FLOAT input, Point_INT origin, RotationAngle alpha) {
    Point_INT result;
    
    switch (alpha) {
        case ANGLE_0:
        result.x = origin.x + (int)input.x;
        result.y = origin.y + (int)input.y;
        break;
        
        case ANGLE_90:
        result.x = origin.x - (int)input.y;
        result.y = origin.y + (int)input.x;
        break;
        
        case ANGLE_NEG_90:
        result.x = origin.x + (int)input.y;
        result.y = origin.y - (int)input.x;
        break;
        
        case ANGLE_180:
        result.x = origin.x - (int)input.x;
        result.y = origin.y - (int)input.y;
        break;
        
        default:
        result.x = origin.x + (int)input.x;
        result.y = origin.y + (int)input.y;
        break;
    }
    
    return result;
}

Point_INT inverse_transform_coordinate_INT(Point_INT p1, Point_INT origin, RotationAngle alpha) {
    Point_INT result;

    int dx = p1.x - origin.x;
    int dy = p1.y - origin.y;

    switch (alpha) {
        case ANGLE_0:

            result.x = dx;
            result.y = dy;
            break;

        case ANGLE_90:
            result.x = dy;
            result.y = -dx;
            break;

        case ANGLE_NEG_90:
            result.x = -dy;
            result.y = dx;
            break;

        case ANGLE_180:
            result.x = -dx;
            result.y = -dy;
            break;

        default:
            result.x = dx;
            result.y = dy;
            break;
    }

    return result;
}

Point_INT Origin = {0, 0};
RotationAngle angleVirtual = ANGLE_0;


ssize_t read_line(int fd, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;

    while (i < maxlen - 1) {
        ssize_t n = read(fd, &c, 1);
        if (n == 1) {
            buf[i++] = c;
            if (c == '\0') break;
        } else if (n == 0) {
            return 0;   // connection closed
        } else {
            return -1;  // error
        }
    }
    buf[i] = '\0';
    return i;
}