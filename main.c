#include "header.h"

// Global variables
unsigned int i;
int current_round_number = 1;
int team0_score = 0, team1_score = 0; // Team scores (Team 0 and Team 1)
int team0_round_final_energy = 0, team1_round_final_energy = 0;
int gui_pid;
volatile sig_atomic_t round_active = 1;
unsigned int max_energy_difference = 0;

float rope_position_offset = 0.0f;      // Rope displacement

// Global config values
pid_t team[NUM_TEAMS][TEAM_PLAYERS];







int main(int argc, char **argv)
{
    printf("*\nProgram started, I am the Parent and My process ID is %d\n\n", getpid());
    // Read configuration and calculate max energy difference
    read_config(FILE_NAME);
    //printf("%d\n\n",THRESHOLD);

    calculate_max_difference();
    printf("⚙ MAX ENERGY DIFFERENCE = %u\n", max_energy_difference);

    // Create FIFOs for GUI and teams
    createFifo(GUIFIFO);
    createFifo(TEAM1FIFO);
    createFifo(TEAM2FIFO);
    create_fifos_players();
    printf("\n\nFIFOs for teams and GUI created successfully\n\n");
    fflush(stdout);

    // Fork and execute the GUI process
    gui_pid = fork();
    if (gui_pid == 0) {
        execlp("./tug_of_war", "tug_of_war", NULL);
        perror("Error: Execute tug_of_war failed.\n");
        exit(1);
    }
    
    // Create the player processes (teams)
    spawn_players();

    sleep(5);
    send_start_signal_to_all_players();

    // Start the game rounds
    startGame();
    sleep(4);
    killAllPlayers();

    printf("\n\n*The Game Is Finished\nGood Bye*\n\n");
    return 0;
}

void spawn_players()
{
    for (int t = 0; t < NUM_TEAMS; t++) {
        for (int i = 0; i < TEAM_PLAYERS; i++) {
            pid_t pid = fork();
            switch (pid) {
                case -1:
                    perror("Error: Fork failed.\n");
                    exit(1);
                case 0: {
                    char index_str[10];
                    char team_str[10];
                    snprintf(index_str, sizeof(index_str), "%d", i);
                    snprintf(team_str, sizeof(team_str), "%d", t);
                    printf("I am a child, PID %d, index=%s, team_NO=%s\n\n", getpid(), index_str, team_str);
                    execlp("./player", "player", index_str, team_str, NULL);
                    perror("execlp failed");
                    exit(1);
                }
                default:
                    team[t][i] = pid;
                    break;
            }
        }
    }
}

void send_start_signal_to_all_players() {
    for (int t = 0; t < NUM_TEAMS; t++) {
        for (int i = 0; i < TEAM_PLAYERS; i++) {
            kill(team[t][i], SIGUSR1);
        }
    }
}

void startGame()
{
    while (team0_score <= (NUMBER_OF_ROUNDS/2) && team1_score <= (NUMBER_OF_ROUNDS/2) && current_round_number <= NUMBER_OF_ROUNDS)
    {
        // Start one round and calculate scores
        startRound();
        calculateRoundScores();
        printf("\n\nTeam results after Round #%d finished:\n\tTeam0: %d    -    %d Team1\n\n", current_round_number, team0_score, team1_score);
        fflush(stdout);

        // Reset round energies and increment round counter
        team0_round_final_energy = 0;
        team1_round_final_energy = 0;
        current_round_number++;
    }
    fflush(stdout);
}
void startRound()
{
    printf("\n\n> Round #%d started after 6 seconds.\n\n", current_round_number);
    fflush(stdout);
    sleep(6);
    //////////////////// CENTERING FOR RESET ///////////////
    // === RESET STATE: players go to center ===
    GameState resa;
    resa.round      = current_round_number;
    resa.team0Score = team0_score;
    resa.team1Score = team1_score;
    resa.ropePos    = 0.0f;
    resa.round_time=ROUND_TIME;

    for (int i = 0; i < 8; i++) {
        resa.playerEnergy[i]   = 100;  // dummy energy just to look filled
        resa.playerPosition[i] = 2;    // everyone centered
        resa.isFallen[i]       = 0;    // assume no one is fallen yet
    }
    //sleep(1);
    int fd_gui = open(GUIFIFO, O_WRONLY | O_NONBLOCK);
    if (fd_gui != -1) {
        write(fd_gui, &resa, sizeof(resa));
        //close(fd_gui);
        printf("[RESET TO GUI] Players centered before round start.\n");
        fflush(stdout);
        sleep(1); 
    }
    //////////////////// END OF CENTERING FOR RESET ///////////////

    int team0_fd = open(TEAM1FIFO, O_RDONLY | O_NONBLOCK);
    int team1_fd = open(TEAM2FIFO, O_RDONLY | O_NONBLOCK);
    if (team0_fd == -1 || team1_fd == -1) {
        perror("Failed to open team FIFOs");
        exit(EXIT_FAILURE);
    }

    int total_team0_energy = 0, total_team1_energy = 0;
    struct message msg;
    ssize_t    bytes_read;

    printf("📱 Listening for energy reports from teams...\n");
    fflush(stdout);
    send_start_signal_to_all_players();


    int team0_energy_sum = 0, team1_energy_sum = 0;
    for (int second = 1; second <= ROUND_TIME && round_active; second++) {
        team0_energy_sum = 0, team1_energy_sum = 0;

        // per-second buffers
        int playerEnergy[8] = {0};
        int playerPos[8]    = {0};
        int playerFall[8]   = {0};

        time_t start = time(NULL), now;
        do {
            // --- read team 0 messages ---
            while ((bytes_read = read(team0_fd, &msg, sizeof(msg))) > 0) {
                team0_energy_sum      += msg.energy;
                playerEnergy[msg.index] = msg.energy;
                playerPos[msg.index]    = msg.pos;      // use the position sent by the player
            }
            // --- read team 1 messages (indices offset by +4) ---
            while ((bytes_read = read(team1_fd, &msg, sizeof(msg))) > 0) {
                int idx = msg.index + TEAM_PLAYERS;
                team1_energy_sum       += msg.energy;
                playerEnergy[idx]       = msg.energy;
                playerPos[idx]          = msg.pos;      // use the position sent by the player
            }

            usleep(5000);
            now = time(NULL);
        } while (now == start && round_active);
        
        total_team0_energy += team0_energy_sum;
        total_team1_energy += team1_energy_sum;

        
        // update rope position as before
        float diff = team1_energy_sum - team0_energy_sum;
        rope_position_offset += diff / (float)max_energy_difference;
        if (rope_position_offset >  1.0f) rope_position_offset =  1.0f;
        if (rope_position_offset < -1.0f) rope_position_offset = -1.0f;

        // mark fallen players
        for (int i = 0; i < TEAM_PLAYERS*2; i++) {
            playerFall[i] = (playerEnergy[i] > 0) ? 0 : 1;
        }

        // build and send the binary GameState
        GameState gs;
        gs.round      = current_round_number;
        gs.team1Score = team1_score;   // note: team0 → team1Score
        gs.team0Score = team0_score;
        gs.ropePos    = rope_position_offset;
        for (int i = 0; i < 8; i++) {
            gs.playerEnergy[i]   = playerEnergy[i];
            gs.playerPosition[i] = playerPos[i];
            gs.isFallen[i]       = playerFall[i];
            gs.round_time=ROUND_TIME-second+1;
        }
        
        int fd_gui = open(GUIFIFO, O_WRONLY | O_NONBLOCK);
        if (fd_gui != -1) {
            ssize_t w = write(fd_gui, &gs, sizeof(gs));
            if (w != sizeof(gs)) {
                fprintf(stderr,
                    "[ERROR] Partial write to GUI FIFO: wrote %zd of %zu bytes\n",
                    w, sizeof(gs));
            }
            close(fd_gui);
            printf("[SEND TO GUI] struct (%zu bytes)\n", sizeof(gs));
        } else {
            perror("[ERROR] Failed to open GUI FIFO for writing");
        }
        close(fd_gui);
    
        printf("\n⏱  [S%d] Energy Snapshot: Team0=%d  Team1=%d\n\n",
               second, team0_energy_sum, team1_energy_sum);
        fflush(stdout);

        if (abs(team0_energy_sum - team1_energy_sum) >= THRESHOLD) {
            printf("Threshold reached! Ending round early.\n");
            round_active = 0;
            break;
        }
        sleep(1);
    }



    stop_all_players();
    team0_round_final_energy = team0_energy_sum;
    team1_round_final_energy = team1_energy_sum;
    round_active = 1;

    close(team0_fd);
    close(team1_fd);
    fflush(stdout);
}
void calculateRoundScores()
{
    if (team0_round_final_energy > team1_round_final_energy) {
        team0_score++;
        printf("Team 0 won the round\n");
        fflush(stdout);
    } else if (team0_round_final_energy < team1_round_final_energy) {
        team1_score++;
        printf("Team 1 won the round\n");
        fflush(stdout);
    } else {
        printf("The round is draw, so the scores remain unchanged\n");
        fflush(stdout);
    }
}


void create_fifos_players()
{
    for (int k = 0; k < TEAM_PLAYERS; k++){
        for (int i = 0; i < TEAM_PLAYERS; i++) {
            for (int j = 0; j < TEAM_PLAYERS; j++) {
                if (i != j) {
                    char fifo[64];
                    sprintf(fifo, "/tmp/TEAM%dFIFO%d_from%d", k, i, j);
                    createFifo(fifo); // Create FIFO for inter-player communication
                }
            }
        }
    }
}


void killAllPlayers()
{
    printf("\nStart kill all players\n");
    for (int t = 0; t < 2; t++){
        for (i = 0; i < TEAM_PLAYERS; i++) {
            kill(team[t][i], SIGQUIT);
            kill(team[t][i], SIGQUIT);
        }
    }
    kill(gui_pid, SIGTERM);
    stop_all_players();
}

void stop_all_players()
{
    printf("\nStop all players\n");
    for (int t = 0; t < 2; t++){
        for (i = 0; i < TEAM_PLAYERS; i++) {
            kill(team[t][i], SIGUSR2);
            kill(team[t][i], SIGUSR2);
        }
    }
}


void calculate_max_difference(){
    int team0_max_possible_energy = 0, team1_min_possible_energy = 0;
    for (int i = 0; i < TEAM_PLAYERS; i++) {
        team0_max_possible_energy += (energy_max * (i + 1));
        team1_min_possible_energy += (energy_min * (i + 1));
    }
    max_energy_difference = team0_max_possible_energy - team1_min_possible_energy;
}