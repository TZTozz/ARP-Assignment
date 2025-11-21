gcc master.c -o master &
gcc I_process.c logger.c -o I_process -lncurses &
gcc drone.c logger.c -o drone &
gcc obstacle.c logger.c -o obstacle &
gcc BB.c logger.c -o BB -lncurses 
