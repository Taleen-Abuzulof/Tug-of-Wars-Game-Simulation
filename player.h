#ifndef PLAYER_H
#define PLAYER_H


// ############################## our variable
extern int unsigned spawn_index;
extern int unsigned team_number;// 0 or 1
extern int  energy;
extern int initial_energy;

// Function prototypes

int get_random_energy();
int compare_messages(const void *a, const void *b);
void determine_position(int energy, int spawn_index, int team_number);
void send_energy_to_parent();
void start_signal_handler(int sig);
void start_sending_energy_periodically();
void stop_round_signal_handler(int sig);
#endif