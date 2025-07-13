#include "header.h"
#include "player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float ropePosition = 0.0f;
int team0Score = 0, team1Score = 0;
float playerEnergy[8] = {0};
int playerPosition[8] = {0};
int isFallen[8] = {0};
int roundNumber = 0;
float gameTime = 0.0f;
static volatile bool gameOver = false;
static int  winner    = 0;    // 1 == team0, 2 == team1
int fd;


void exitAfterDelay(int value) {
    (void)value;  // suppress unused‐param warning
    exit(0);
}

void onGameEnd(int sig) {
    // pick winner however you like
    winner   = (team0Score > team1Score) ? 0 : 1;
    gameOver = true;

    // schedule exit in 5000 ms using a real function pointer
    glutTimerFunc(5000, exitAfterDelay, 0);
}

void drawRectangle(float x, float y, float width, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
    glEnd();
}

void drawCircle(float x, float y, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);
        for (int i = 0; i <= 360; i++) {
            float angle = i * M_PI / 180.0f;
            glVertex2f(x + cos(angle)*radius, y + sin(angle)*radius);
        }
    glEnd();
}

void drawPlayer(float x, float y, float r, float g, float b) {
    drawCircle(x, y + 0.08f, 0.03f, r, g, b);
    drawRectangle(x - 0.02f, y - 0.05f, 0.04f, 0.08f, r, g, b);
    drawRectangle(x - 0.03f, y - 0.10f, 0.02f, 0.05f, r, g, b);
    drawRectangle(x + 0.01f, y - 0.10f, 0.02f, 0.05f, r, g, b);
    drawRectangle(x - 0.04f, y, 0.02f, 0.04f, r, g, b);
    drawRectangle(x + 0.02f, y, 0.02f, 0.04f, r, g, b);
}




void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (!gameOver) {
        // Draw background
        drawRectangle(-1.0f, 0.0f, 2.0f, 1.0f, 0.53f, 0.81f, 0.92f);  // Sky
        drawRectangle(-1.0f, -1.0f, 2.0f, 1.0f, 0.0f, 0.6f, 0.0f);   // Grass
        drawRectangle(ropePosition - 0.8f, -0.1f, 1.6f, 0.02f, 0.9f, 0.6f, 0.2f); // Rope

        for (int team = 0; team < 2; team++) {
            int base = team * TEAM_PLAYERS;
            int idx[TEAM_PLAYERS] = {0, 1, 2, 3};

            // Sort indices by energy
            for (int i = 0; i < TEAM_PLAYERS - 1; i++) {
                for (int j = i + 1; j < TEAM_PLAYERS; j++) {
                    if (playerEnergy[base + idx[i]] < playerEnergy[base + idx[j]]) {
                        int temp = idx[i];
                        idx[i] = idx[j];
                        idx[j] = temp;
                    }
                }
            }

            for (int i = 0; i < TEAM_PLAYERS; i++) {
                int p = base + idx[i];
                float posX = (team == 0 ? -0.8f + i * 0.2f : 0.8f - i * 0.2f) + ropePosition * 0.5f;
                float posY = -0.3f;

                if (isFallen[p]) {
                    glColor3f(1.0f, 0.0f, 0.0f);
                    glRasterPos2f(posX - 0.02f, posY);
                    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, 'X');
                    continue;
                }

                // Normalize and determine color level
                float norm = (playerEnergy[p] - energy_min) / (float)(energy_max - energy_min);
                int level = (int)(norm * 4); if (level > 3) level = 3;
                float r, g, b;

                if (team == 0) { // Blue team
                    switch (level) {
                        case 0: r = 0.6f; g = 0.6f; b = 1.0f; break;
                        case 1: r = 0.3f; g = 0.3f; b = 0.9f; break;
                        case 2: r = 0.0f; g = 0.0f; b = 0.7f; break;
                        case 3: r = 0.0f; g = 0.0f; b = 0.5f; break;
                    }
                } else { // Red team
                    switch (level) {
                        case 0: r = 1.0f; g = 0.6f; b = 0.6f; break;
                        case 1: r = 0.9f; g = 0.3f; b = 0.3f; break;
                        case 2: r = 0.7f; g = 0.0f; b = 0.0f; break;
                        case 3: r = 0.5f; g = 0.0f; b = 0.0f; break;
                    }
                }

                drawPlayer(posX, posY, r, g, b);

                // Display Energy
                char label[20];
                sprintf(label, "E:%d", (int)playerEnergy[p]);
                glColor3f(1.0f, 1.0f, 0.0f);
                glRasterPos2f(posX - 0.03f, posY - 0.15f);
                for (char *c = label; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

                // Display Position
                sprintf(label, "P:%d", playerPosition[p]);
                glRasterPos2f(posX - 0.03f, posY - 0.25f);
                for (char *c = label; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
            }
        }

        // HUD Info
        glColor3f(1.0f, 1.0f, 1.0f);
        char info[64];
        glRasterPos2f(-0.9f, 0.85f);
        sprintf(info, "Team 0: %d", team0Score);
        for (char *c = info; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(0.6f, 0.85f);
        sprintf(info, "Team 1: %d", team1Score);
        for (char *c = info; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(-0.05f, 0.95f);
        sprintf(info, "Round: %d", roundNumber);
        for (char *c = info; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(-0.05f, 0.90f);
        sprintf(info, "Time: %.1f", gameTime);
        for (char *c = info; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    else {
        // draw a big “Team X Wins!” screen
        char buf[64];
        snprintf(buf, sizeof(buf), "Team %d Wins!", winner);
        // center‑and‑draw buf in large font:
        glColor3f(1,1,0);
        glRasterPos2f(-0.3f, 0.0f);
        for (char* c = buf; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    glutSwapBuffers();
}


void updateGameStateFromFIFO() {
    GameState gs;
    ssize_t n = read(fd, &gs, sizeof(gs));
    if (n == sizeof(gs)) {
        // pull in the new remaining time
        gameTime    = gs.round_time;
        roundNumber = gs.round;
        team0Score  = gs.team0Score;
        team1Score  = gs.team1Score;
        ropePosition = gs.ropePos;
        for (int i = 0; i < 8; i++) {
            playerEnergy[i]   = gs.playerEnergy[i];
            playerPosition[i] = gs.playerPosition[i];
            isFallen[i]       = gs.isFallen[i];
        }
    }

}

void updateGame() {
    updateGameStateFromFIFO();
}

void timerCallback(int value) {
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(15, timerCallback, 0);
}

void init() {
    fd = open(GUIFIFO, O_RDWR | O_NONBLOCK);
    if (fd == -1) return;
    glClearColor(0.2f, 0.5f, 0.2f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.5, 1.5, -1.5, 1.5);
}

int main(int argc, char** argv) {
    signal(SIGTERM, onGameEnd);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("Tug-of-War Game - Final");
    init();
    glutDisplayFunc(drawScene);
    glutTimerFunc(15, timerCallback, 0);
    glutMainLoop();
    return 0;
}