# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Executable names
GAME_EXE = game
PLAYER_EXE = player
GUI_EXE = tug_of_war

# Source files
GAME_SRCS = main.c functions.c
PLAYER_SRCS = player.c functions.c
GUI_SRCS = tug_of_war.c functions.c

# Object files (automatically generated)
GAME_OBJS = $(GAME_SRCS:.c=.o)
PLAYER_OBJS = $(PLAYER_SRCS:.c=.o)
GUI_OBJS = $(GUI_SRCS:.c=.o)

# Default target: build all executables
all: $(GAME_EXE) $(PLAYER_EXE) $(GUI_EXE)

# Build the main game executable
$(GAME_EXE): $(GAME_OBJS)
	$(CC) $(CFLAGS) -o $(GAME_EXE) $(GAME_OBJS)

# Build the player executable
$(PLAYER_EXE): $(PLAYER_OBJS)
	$(CC) $(CFLAGS) -o $(PLAYER_EXE) $(PLAYER_OBJS)

# Build the GUI executable (tug_of_war)
$(GUI_EXE): $(GUI_OBJS)
	$(CC) $(CFLAGS) -o $(GUI_EXE) $(GUI_OBJS) -lGL -lGLU -lglut -lm

# Compile main.c (depends on header.h)
main.o: main.c header.h
	$(CC) $(CFLAGS) -c main.c

# Compile player.c (depends on player.h and header.h)
player.o: player.c player.h
	$(CC) $(CFLAGS) -c player.c

# Compile functions.c (depends on header.h)
functions.o: functions.c header.h
	$(CC) $(CFLAGS) -c functions.c

# Compile tug_of_war.c (depends on header.h)
tug_of_war.o: tug_of_war.c header.h 
	$(CC) $(CFLAGS) -c tug_of_war.c

# Clean build artifacts
clean:
	rm -f *.o $(GAME_EXE) $(PLAYER_EXE) $(GUI_EXE)

.PHONY: all clean
