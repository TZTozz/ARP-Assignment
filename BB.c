#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int max(int a, int b) {
    return (a > b) ? a : b;
}

int main(int argc, char *argv[])
{
    int fd_r_drone = atoi(argv[1]);
    int fd_w_drone = atoi(argv[2]);

    int fd_r_input = atoi(argv[3]);
    int fd_w_input = atoi(argv[4]);

    int max_fd = max(fd_r_drone, fd_w_drone);

    char message_in[80], message_out[80];

    fd_set r_fds;
    int retval;
    char line[80];

    //Blackboard data
    int xDrone, yDrone;
    int Fx = 0, Fy = 0;

    while (1)
    {
        //Inizializzo i set
        FD_ZERO(&r_fds);
        //Aggiungo i file descriptor
        FD_SET(fd_r_drone, &r_fds);
        FD_SET(fd_r_input, &r_fds);

        //Rimane in attesa 
        retval = select(max_fd + 1, &r_fds, NULL, NULL, NULL);

        if (retval == -1) perror("select()");
        else if (retval) 
        {
            if(FD_ISSET(fd_r_drone, &r_fds))
            {
                read(fd_r_drone, message_in, sizeof(message_in));
                sprintf(message_out, "%d %d", xDrone, yDrone);
                write(fd_w_drone, message_out, strlen(message_out) + 1);

            }

            if(FD_ISSET(fd_r_input, &r_fds))
            {
                read(fd_r_input, message_in, sizeof(message_in));
                sscanf(message_in, "%d %d", &Fx, &Fy);

            }
        }
        else printf("No data within five seconds.\n");
        }
}