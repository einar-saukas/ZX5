CC = owcc
CFLAGS  = -ox -ob -ol+ -onatx -oh -zp8 -fp6 -g0 -Ot -oe -ot -Wall -xc -s -finline-functions -finline-intrinsics -finline-math -floop-optimize -frerun-optimizer -fno-stack-check -march=i386 -mtune=i686
RM = del
EXTENSION = .exe

all: zx5 dzx5

zx5: zx5.c optimize.c compress.c memory.c zx5.h
	$(CC) $(CFLAGS) -o zx5$(EXTENSION) zx5.c optimize.c compress.c memory.c

dzx5: dzx5.c
	$(CC) $(CFLAGS) -o dzx5$(EXTENSION) dzx5.c

clean:
	$(RM) *.obj
