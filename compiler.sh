gcc master.c -o master &
gcc I_process.c logger.c -o I_process -lncurses &
gcc drone.c logger.c -o drone -lncurses &
gcc BB.c logger.c -o BB &
