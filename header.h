#ifndef LIBRARIES
#define LIBRARIES

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <math.h>
#include <time.h>


#define TEAM_PLAYERS 4
#define FILE_NAME "arguments.txt"
#define TEAM_SIZE 4 //----------------------------------------
#define NUM_TEAMS 2
#define TEAM1FIFO "/tmp/TEAM0FIFO"
#define TEAM2FIFO "/tmp/TEAM1FIFO"
#define GUIFIFO "/tmp/PUBLICFIFO"

struct message {
    int unsigned index;
    int unsigned energy;
    int unsigned team;
    int unsigned pos;
};
  

typedef struct {
    int round;
    int team0Score;
    int team1Score;
    float ropePos;
    int playerEnergy[8];
    int playerPosition[8];
    int isFallen[8];
    int round_time;
    //int winner;  // 0 = game not over, 1 = Team 0 wins, 2 = Team 1 wins
} GameState;

// default values, if the file that opend is empty or somthing get error
extern int energy_min;
extern int energy_max;
extern float ROUND_TIME;
extern int decay_min;
extern int decay_max;
extern int NUMBER_OF_ROUNDS;
extern int THRESHOLD;
extern int fall_time_min;
extern int fall_time_max;
extern int energy_increase_min;
extern int energy_increase_max;

void writeMessageFifo(const char *fifo_name, const struct message *msg);
void createFifo(char *fifo_name);
void readMessageFifo(const char *team_fifo_name, struct message *message);
void read_config(const char *filename);
void spawn_players();
void startGame();
void startRound();
void calculateRoundScores();
void killAllPlayers();
void create_fifos_players();
int compare_energy(const void *a, const void *b);
void send_start_signal_to_all_players();
void stop_all_players();
void calculate_max_difference();

#endif