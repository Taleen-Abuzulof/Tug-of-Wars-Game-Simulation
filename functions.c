#include "header.h"


 int NUMBER_OF_ROUNDS = 5;
 int energy_min = 20, energy_max = 90;
 int decay_min = 1, decay_max = 5;
 float ROUND_TIME = 10.0;
 int THRESHOLD=400;
 int fall_time_min = 1;
 int fall_time_max = 3;
  int energy_increase_min=1;
  int energy_increase_max=10;
// create a fifo
void createFifo(char *fifo_name)
{
    // remove fifo if it's exist
    remove(fifo_name);

    if ((mkfifo(fifo_name,  0777)) == -1)
    {
        perror("Error Creating Fifo");
        exit(-1);
    }
}


void writeMessageFifo(const char *fifo_name, const struct message *msg)
{
    int fifo_fd;

    // Keep trying to open until reader is ready
    while ((fifo_fd = open(fifo_name, O_WRONLY | O_NONBLOCK)) == -1) {
        if (errno == ENXIO) {
            usleep(10000); // 10ms delay
            continue;
        } else {
            perror("writeMessageFifo: open failed");
            exit(EXIT_FAILURE);
        }
    }

    if (write(fifo_fd, msg, sizeof(*msg)) != sizeof(*msg)) {
        perror("writeMessageFifo: incomplete write");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }

    //printf("[DEBUG] WROTE to FIFO: %s from PID: %d\n", fifo_name, getpid());
    fflush(stdout);
    close(fifo_fd);
}
void readMessageFifo(const char *fifo_name, struct message *msg)
{
    int fifo_fd;

    // Wait until writer is ready
    while ((fifo_fd = open(fifo_name, O_RDONLY | O_NONBLOCK)) == -1) {
        if (errno == ENXIO) {
            usleep(10000); // 10ms delay
            continue;
        } else {
            perror("readMessageFifo: open failed");
            exit(EXIT_FAILURE);
        }
    }

    ssize_t read_bytes;
    while ((read_bytes = read(fifo_fd, msg, sizeof(*msg))) <= 0) {
        if (read_bytes == 0) {
            // No data yet
            usleep(10000);
            continue;
        } else if (errno == EAGAIN || errno == EINTR) {
            continue;
        } else {
            perror("readMessageFifo: read error");
            close(fifo_fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("[DEBUG] READ from FIFO: %s in PID: %d\n", fifo_name, getpid());
    fflush(stdout);
    close(fifo_fd);
}



void read_config(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("read_config: could not open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    char key[64];
    int  val;

    while (fgets(line, sizeof(line), f)) {
        // skip empty or comment lines
        if (line[0] == '\n' || line[0] == '#') 
            continue;

        // try to parse "KEY=INT\n"
        if (sscanf(line, "%63[^=]=%d", key, &val) != 2)
            continue;

        // dispatch
        if      (strcmp(key, "NUMBER_OF_ROUNDS")   == 0) NUMBER_OF_ROUNDS    = val;
        else if (strcmp(key, "THRESHOLD")          == 0) THRESHOLD           = val;
        else if (strcmp(key, "INITIAL_ENERGY_MIN") == 0) energy_min          = val;
        else if (strcmp(key, "INITIAL_ENERGY_MAX") == 0) energy_max          = val;
        else if (strcmp(key, "ENERGY_DECAY_MIN")   == 0) decay_min           = val;
        else if (strcmp(key, "ENERGY_DECAY_MAX")   == 0) decay_max           = val;
        else if (strcmp(key, "FALL_TIME_MIN")      == 0) fall_time_min       = val;
        else if (strcmp(key, "FALL_TIME_MAX")      == 0) fall_time_max       = val;
        else if (strcmp(key, "ENERGY_INCREASE_MIN")== 0) energy_increase_min = val;
        else if (strcmp(key, "ENERGY_INCREASE_MAX")== 0) energy_increase_max = val;
        else if (strcmp(key, "ROUND_TIME")         == 0) ROUND_TIME          = (float)val;
        // else: unknown key → ignore
    }

    fclose(f);
}