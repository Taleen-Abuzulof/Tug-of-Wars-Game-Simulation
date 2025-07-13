#include "player.h"
#include "header.h"


int unsigned spawn_index;
int unsigned team_number;// 0 or 1
int energy;
int initial_energy;
int decrease_rate;
int recovery_time;
int unsigned position;  // 1 (lowest) → TEAM_PLAYERS (highest)
struct message *all_msgs;

volatile sig_atomic_t ready_to_start = 0;
volatile sig_atomic_t stop_round = 0;
// Build the full team energy array
struct message full_team[TEAM_SIZE];

int compare_energy(const void *a, const void *b) {
    struct message *m1 = (struct message *)a;
    struct message *m2 = (struct message *)b;
    return m2->energy - m1->energy; // descending order
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        perror("Usage: ./player <index> <team>");
        exit(EXIT_FAILURE);
    }

    spawn_index = atoi(argv[1]);
    team_number = atoi(argv[2]);

    srand((unsigned)getpid()); // Seed RNG with PID

    printf("👋 [INIT] Player %d from Team %d initialized. PID: %d\n", spawn_index, team_number, getpid());
    fflush(stdout);

    // Register signal handlers
    signal(SIGUSR1, start_signal_handler); 
    signal(SIGUSR2, stop_round_signal_handler); 

    // Generate random energy
    energy = get_random_energy();
    initial_energy = energy;
    decrease_rate = decay_min + rand() % (decay_max - decay_min + 1);
    recovery_time = fall_time_min + rand() % (fall_time_max - fall_time_min + 1);

    printf("⚡ [ENERGY] Player %d (Team %d) initial energy: %d\n\n", spawn_index, team_number, energy);
    fflush(stdout);

    all_msgs = malloc(sizeof(struct message) * TEAM_PLAYERS);
    if (!all_msgs) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Wait for round start
        ready_to_start = 0;
        while (!ready_to_start) pause();
        printf("✅ [ROUND START] Player %d (Team %d) exchanging energy...\n", spawn_index, team_number);
        fflush(stdout);

        // Refill message list
        all_msgs[0] = (struct message){ .energy = energy, .index = spawn_index, .team = team_number, .pos= position};
        int count = 1;

        for (int i = 0; i < TEAM_PLAYERS; i++) {
            if (i == spawn_index) {
                for (int j = 0; j < TEAM_PLAYERS; j++) {
                    if (j == spawn_index) continue;

                    struct message msg = { .energy = energy, .index = spawn_index, .team = team_number, .pos= position };
                    char fifo[64];
                    sprintf(fifo, "/tmp/TEAM%dFIFO%d_from%d", team_number, j, spawn_index);
                    writeMessageFifo(fifo, &msg);
                }
            } else {
                struct message received_msg;
                char fifo[64];
                sprintf(fifo, "/tmp/TEAM%dFIFO%d_from%d", team_number, spawn_index, i);
                readMessageFifo(fifo, &received_msg);
                all_msgs[count++] = received_msg;
            }
            usleep(3000);
        }

        // Build full team array and sort
        for (int i = 0; i < TEAM_SIZE; i++) {
            full_team[i] = all_msgs[i];
        }

        qsort(full_team, TEAM_SIZE, sizeof(struct message), compare_energy);

        // Print sorted team energy
        printf("📊 [TEAM %d] Sorted player energies this round:\n", team_number);
        for (int i = 0; i < TEAM_SIZE; i++) {
            printf("   ➤ Player %d: Energy = %d \n", full_team[i].index, full_team[i].energy);
        }
        fflush(stdout);

        // Determine player’s ranking
        determine_position(energy, spawn_index, team_number);

        // Start sending energy every second
        start_sending_energy_periodically();

        // Wait for round stop
        stop_round = 0;

        while (!stop_round) pause();
        int energy_increase= energy_increase_min + rand() % (energy_increase_max- energy_increase_min + 1);
        energy+=energy_increase;
    }

    return 0;
}

int compare_messages(const void *a, const void *b) {
    struct message *msgA = (struct message *)a;
    struct message *msgB = (struct message *)b;

    if (msgA->energy != msgB->energy) {
        return msgA->energy - msgB->energy; // Ascending energy
    } else {
        return msgA->index - msgB->index;   // Ascending index (so highest index = last)
    }
}

void determine_position(int energy, int spawn_index, int team_number) {  ////////////////////////////////// Q
    
    // Sort all messages
    qsort(all_msgs, TEAM_PLAYERS, sizeof(struct message), compare_messages);

    // Determine own position (1 = lowest energy, TEAM_PLAYERS = highest)
    for (int i = 0; i < TEAM_PLAYERS; i++) {
        if (all_msgs[i].index == spawn_index) {
            position = i + 1;
            break;
        }
    }

    printf("🏅 Player %d (Team %d) ranked position %d out of %d\n\n",
           spawn_index, team_number, position, TEAM_PLAYERS);
    fflush(stdout);

    free(all_msgs);
}

void send_energy_to_parent() {
    energy -= decrease_rate;

    if (energy < 0)
        energy = 0; // Prevent negative energy

    if (energy == 0) {
        printf("💀 [PLAYER FALL] Player %d (Team %d) fell! Resting for %d seconds...\n", 
               spawn_index, team_number, recovery_time);
        fflush(stdout);

        sleep(recovery_time); // Simulate recovery
        energy = initial_energy;

        printf("✅ [PLAYER RECOVERED] Player %d (Team %d) is back!\n", 
               spawn_index, team_number);
        fflush(stdout);
    }

    // Multiply energy by position (priority weight)
    struct message public_msg = { 
        .energy = energy * position, 
        .index = spawn_index, 
        .team = team_number, 
        .pos = position
    };

    if (team_number == 0) {
        printf("📤 [PUBLIC SEND] Player %d (Team %d) sending energy %d to team 0 public FIFO (%s)\n",
               spawn_index, team_number, public_msg.energy, TEAM1FIFO);
        fflush(stdout);
        writeMessageFifo(TEAM1FIFO, &public_msg);
    } else if (team_number == 1) {
        printf("📤 [PUBLIC SEND] Player %d (Team %d) sending energy %d to team 1 public FIFO (%s)\n",
               spawn_index, team_number, public_msg.energy, TEAM2FIFO);
        fflush(stdout);
        writeMessageFifo(TEAM2FIFO, &public_msg);
    }

    // Set alarm to repeat this every 1 sec
    alarm(1);
}


void start_sending_energy_periodically() {
    signal(SIGALRM, send_energy_to_parent);  // Register the signal handler
    alarm(1);  // Trigger the alarm to send energy every second
}


int get_random_energy(){
    srand((unsigned)getpid());
    int energy = energy_min + rand() % (energy_max - energy_min + 1);
    return energy;
}

void start_signal_handler(int sig) {
    if (sig == SIGUSR1) {
        ready_to_start = 1;
    }
}


void stop_round_signal_handler(int sig) {
    if (sig == SIGUSR2) {
        stop_round = 1;
    }
}