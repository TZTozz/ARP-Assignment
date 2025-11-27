gcc src/master.c -o bin/master &
gcc src/I_process.c src/logger.c -o bin/I_process -lncurses &
gcc src/drone.c src/logger.c -o bin/drone &
gcc src/obstacles.c src/logger.c -o bin/obstacles -lm &
gcc src/BB.c src/logger.c -o bin/BB -lncurses &
gcc src/targets.c src/logger.c -o bin/targets -lm &
cd ./bin &&
./master &&
cd ..