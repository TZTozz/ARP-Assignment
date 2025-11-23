gcc src/master.c -o bin/master &
gcc src/I_process.c src/logger.c -o bin/I_process -lncurses &
gcc src/drone.c src/logger.c -o bin/drone &
gcc src/obstacle.c src/logger.c -o bin/obstacle &
gcc src/BB.c src/logger.c -o bin/BB -lncurses 
