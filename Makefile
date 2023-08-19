all:
	cc -I src\include\SDL2 -std=gnu11 -Wall -Wextra -Werror -O2 main.c gb_gpu.c gb_cpu.c -o main -lmingw32 -lSDL2main -lSDL2 -Lsrc\lib
