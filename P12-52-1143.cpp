#define GL_SILENCE_DEPRECATION
#include <random>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <glut.h>

const int screenWidth = 1000;
const int screenHeight = 600;
const int maxLives = 5;
const float amplitude = 0.3;
const float animationSpeed = 0.05;
float animationPhase = 0.0;
int score = 0;
int lives = maxLives;
int timer = 120;
double M_PI = 3.141592653589793238462643383279502884197;

struct Obstacle {
    float x;
    float y;
    float size;
};

struct Collectable {
    float x;
    float y;
    float size;
    bool isCollected;
};

struct Target {
    float x;
    float y;
    float size;
    bool isHit;
};

struct Player {
    float x;
    float y;
    float size;
    float angle;
    float speed;
};

struct PowerUp {
    float x;
    float y;
    float size;
    float type;
    bool isObtained;
    int effectDuration;
    bool isActive;
};

struct Moon {
    float x;
    float y;
    float size;
    float rotationAngle;
};

Obstacle obstacles[5];
std::vector<Collectable> collectables;
std::vector<Collectable> extraCollectables;
Target target;
Player player;
std::vector<PowerUp> powerUps; // 0 --> green power-up (speed), 1 --> red power-up (collectables)
Moon moon;

// Play background music
void playBackgroundMusic() {
    PlaySound(TEXT("horror-ambience"), NULL, SND_ASYNC | SND_LOOP);
}

// Play obstacle sound
void playGhostSound(int duration) {
    PlaySound(TEXT("cartoon-ghosts"), NULL, SND_ASYNC);
    Sleep(duration * 1000);  
    playBackgroundMusic(); 
}

// Play collectable sound
void playCollectableSound(int duration) {
    PlaySound(TEXT("collectable"), NULL, SND_ASYNC);
    Sleep(duration * 1000);
    playBackgroundMusic();
}

// Play target sound
void playTargetSound(int duration) {
    PlaySound(TEXT("target"), NULL, SND_ASYNC);
    Sleep(duration * 1000);
    playBackgroundMusic();
}

// Play game over sound
void playGameOverSound(int duration) {
    PlaySound(TEXT("game-over"), NULL, SND_ASYNC);
    Sleep(duration * 1000);
    playBackgroundMusic();
}

// Play you win sound
void playYouWinSound(int duration) {
    PlaySound(TEXT("you-win"), NULL, SND_ASYNC);
    Sleep(duration * 1000);
    playBackgroundMusic();
}

// Draw a circle
void drawCircle(int x, int y, float r) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    GLUquadric* quadObj = gluNewQuadric();
    gluDisk(quadObj, 0, r, 50, 50);
    glPopMatrix();
}

// Draw batman outline
void drawBatman(float x, float y, float size, float r, float g, float b) {
    glColor3f(r, g, b);

    glPushMatrix();

    glTranslated(x, y, 0);
    glScaled(size, size, 1);

    glPushMatrix();

    glBegin(GL_POINTS);

    for (double xcord = 7; xcord >= 4; xcord -= 0.01)
    {
        double ycordPos = 2 * sqrt((double)(-1 * (abs(abs(xcord) - 1)) * abs(3 - abs(xcord)) / ((abs(xcord) - 1) * (3 - abs(xcord))))) *
            (1 + abs(abs(xcord) - 3) / (abs(xcord) - 3)) * sqrt((double)(1 - pow((xcord / 7), 2))) +
            (5 + 0.97 * (abs(xcord - 0.5) + abs(xcord + 0.5)) - 3 * (abs(xcord - 0.75) + abs(xcord + 0.75))) *
            (1 + abs(1 - abs(xcord)) / (1 - abs(xcord)));
        glVertex3d(xcord, ycordPos, 0);

        double ycordNeg = -3 * sqrt((double)(1 - pow((xcord / 7), 2))) * sqrt((double)(abs(abs(xcord) - 4) / (abs(xcord) - 4)));
        glVertex3d(xcord, ycordNeg, 0);

        glVertex3d(-xcord, ycordPos, 0);
        glVertex3d(-xcord, ycordNeg, 0);
    }

    for (double xcord = 3.99; xcord >= 0; xcord -= 0.01)
    {
        double ycordPos;
        if (xcord >= 3 || xcord <= 1)
        {
            ycordPos = 2 * sqrt((double)(-1 * (abs(abs(xcord) - 1)) * abs(3 - abs(xcord)) / ((abs(xcord) - 1) * (3 - abs(xcord))))) *
                (1 + abs(abs(xcord) - 3) / (abs(xcord) - 3)) * sqrt((double)(1 - pow((xcord / 7), 2))) +
                (5 + 0.97 * (abs(xcord - 0.5) + abs(xcord + 0.5)) - 3 * (abs(xcord - 0.75) + abs(xcord + 0.75))) *
                (1 + abs(1 - abs(xcord)) / (1 - abs(xcord)));
        }
        else
        {
            ycordPos = 2.71052 + (1.5 - 0.5 * abs(xcord)) - 1.35526 * sqrt((double)(4 - pow(abs(xcord) - 1, 2))) *
                sqrt((double)(abs(abs(xcord) - 1) / (abs(xcord) - 1))) + 0.9;
        }
        glVertex3d(xcord, ycordPos, 0);

        double ycordNeg = abs(xcord / 2) - 0.0913722 * pow(xcord, 2) - 3 + sqrt((double)(1 - pow((abs(abs(xcord) - 2) - 1), 2)));
        glVertex3d(xcord, ycordNeg, 0);

        glVertex3d(-xcord, ycordPos, 0);
        glVertex3d(-xcord, ycordNeg, 0);
    }

    glEnd();

    glPopMatrix();
    glPopMatrix();
}

// Check collisions
bool checkCollision(float x1, float y1, float size1, float x2, float y2, float size2) {
    float minDistance = size1 + size2;
    float distance = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));

    if (distance <= minDistance)
        return true;
    return false;
}

// Generate obstacles
void createObstacles() {
    Obstacle obstacle1 = { 200.0, 200.0, 20.0 };
    Obstacle obstacle2 = { 400.0, 400.0, 20.0 };
    Obstacle obstacle3 = { 600.0, 300.0, 20.0 };
    Obstacle obstacle4 = { 800.0, 100.0, 20.0 };
    Obstacle obstacle5 = { 300.0, 500.0, 20.0 };

    obstacles[0] = obstacle1;
    obstacles[1] = obstacle2;
    obstacles[2] = obstacle3;
    obstacles[3] = obstacle4;
    obstacles[4] = obstacle5;
}

// Draw obstacles
void drawObstacles() {
    for (Obstacle& obstacle : obstacles) {
        // Draw ghost
        glColor3f(0.8, 0.8, 0.8); // Off-white color for ghost 👻
        drawCircle(obstacle.x, obstacle.y, obstacle.size);

        // Draw eyes
        float eye1X = obstacle.x - obstacle.size * 0.25;
        float eye1Y = obstacle.y + obstacle.size * 0.25;
        float eye2X = obstacle.x + obstacle.size * 0.25;
        float eye2Y = obstacle.y + obstacle.size * 0.25;

        glColor3f(0.0, 0.0, 0.0); // Black color
        drawCircle(eye1X, eye1Y, obstacle.size * 0.1);
        drawCircle(eye2X, eye2Y, obstacle.size * 0.1);

        // Draw mouth
        glLineWidth(2.0);
        glBegin(GL_LINES);
        glVertex2f(obstacle.x - obstacle.size * 0.2, obstacle.y - obstacle.size * 0.25);
        glVertex2f(obstacle.x + obstacle.size * 0.2, obstacle.y - obstacle.size * 0.25);
        glEnd();
    }
}

// Generate collectables
void createCollectables() {
    for (int i = 0; i < 5; i++) {
        float x = rand() % 950 + 10;
        float y = rand() % 550 + 10;
        float size = 17.0;

        Collectable collectable = { x, y, size, false };
        collectables.push_back(collectable);
    }
}

// Draw collectables
void drawCollectables() {
    for (Collectable& collectable : collectables) {
        if (!collectable.isCollected) {
            glColor3f(1.0, 1.0, 0.0); // Yellow color
            drawCircle(collectable.x, collectable.y, collectable.size);

            glColor3f(0.0, 0.0, 0.0); // Black color
            drawCircle(collectable.x, collectable.y, collectable.size * 0.8);

            glColor3f(0.5, 0.0, 0.5); // Purple color
            drawCircle(collectable.x, collectable.y, collectable.size * 0.6);
            drawBatman(collectable.x, collectable.y, collectable.size * 0.086, 0, 0, 0);
        }
    }

    for (Collectable& collectable : extraCollectables) {
        if (!collectable.isCollected) {
            glColor3f(1.0, 1.0, 0.0); // Yellow color
            drawCircle(collectable.x, collectable.y, collectable.size);

            glColor3f(0.0, 0.0, 0.0); // Black color
            drawCircle(collectable.x, collectable.y, collectable.size * 0.8);

            glColor3f(0.5, 0.0, 0.5); // Purple color
            drawCircle(collectable.x, collectable.y, collectable.size * 0.6);
            drawBatman(collectable.x, collectable.y, collectable.size * 0.086, 0, 0, 0);
        }
    }
}

// Generate target
void createTarget() {
    target.x = 50.0;
    target.y = 50.0;
    target.size = 20.0;
    target.isHit = false;
}

// Draw target
void drawTarget() {
    if (!target.isHit) {
        glColor3f(1.0, 0.0, 0.0); // Red color
        drawCircle(target.x, target.y, target.size);

        glColor3f(1.0, 1.0, 1.0); // White color
        drawCircle(target.x, target.y, target.size * 0.8);

        glColor3f(1.0, 0.0, 0.0); // Red color
        drawCircle(target.x, target.y, target.size * 0.6);

        glColor3f(1.0, 1.0, 1.0); // White color
        drawCircle(target.x, target.y, target.size * 0.4);

        glColor3f(1.0, 0.0, 0.0); // Red color
        drawCircle(target.x, target.y, target.size * 0.2);

        glPopMatrix();
    }
}

// Generate player
void createPlayer() {
    player.x = screenWidth / 2;
    player.y = screenHeight / 2;
    player.size = 30.0;
    player.angle = 0;
    player.speed = 10.0;
}

// Draw player
void drawPlayer() {
    glPushMatrix();
    glTranslatef(player.x, player.y, 0);

    glRotatef(player.angle, 0, 0, 1);

    // Draw bat
    glColor3f(1.0, 0.5, 0.0); // Orange color
    drawBatman(0, 0, 7.0, 1, 0.5, 0);

    // Draw eyes
    glColor3f(1.0, 1.0, 1.0); // Black color
    drawCircle(-2, 15, 1.0);
    drawCircle(2, 15, 1.0);

    // Draw mouth
    glColor3f(1.0, 1.0, 1.0); // White color
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glVertex2f(-3, 10);
    glVertex2f(3, 10);
    glEnd();

    // Draw pattern on wings
    glColor3f(1.0, 0.5, 0.0); // Orange color
    glBegin(GL_TRIANGLES);
    glVertex2f(40, 0);
    glVertex2f(35, 4);
    glVertex2f(35, -4);
    glEnd();

    glColor3f(1.0, 0.5, 0.0); // Orange color
    glBegin(GL_TRIANGLES);
    glVertex2f(-40, 0);
    glVertex2f(-35, 4);
    glVertex2f(-35, -4);
    glEnd();

    glPopMatrix();
}

// Generate power-ups
void createPowerUps() {
    float greenX = rand() % 950 + 10;
    float greenY = rand() % 550 + 10;
    PowerUp powerSpeed = { greenX, greenY, 15, 0, false, 5, false };

    float redX = rand() % 950 + 10;
    float redY = rand() % 550 + 10;
    PowerUp powerCollect = { redX, redY, 15, 1, false, 5, false };

    powerUps.push_back(powerSpeed);
    powerUps.push_back(powerCollect);
}

// Draw power-ups
void drawPowerUps() {
    for (const PowerUp& powerUp : powerUps) {
        if (!powerUp.isObtained) {
            glPushMatrix();
            glTranslatef(powerUp.x, powerUp.y, 0);

            if (powerUp.type == 0) {
                glColor3f(0.0, 1.0, 0.0); // Green color
            }
            else if (powerUp.type == 1) {
                glColor3f(1.0, 0.0, 0.0); // Red color
            }

            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 5; ++i) {
                float angle = i * (2 * M_PI / 5);
                float x = powerUp.size * cos(angle);
                float y = powerUp.size * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();

            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 5; ++i) {
                float angle = i * (2 * M_PI / 5);
                float x = (powerUp.size * 0.8) * cos(angle);
                float y = (powerUp.size * 0.8) * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();

            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 5; ++i) {
                float angle = i * (2 * M_PI / 5);
                float x = (powerUp.size * 0.6) * cos(angle);
                float y = (powerUp.size * 0.6) * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();

            glBegin(GL_POLYGON);
            for (int i = 0; i < 5; ++i) {
                float angle = i * (2 * M_PI / 5);
                float x = (powerUp.size * 0.4) * cos(angle);
                float y = (powerUp.size * 0.4) * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();

            glPopMatrix();
        }
    }
}

// Generate moon
void createMoon() {
    moon.x = 920;
    moon.y = screenHeight - 90;
    moon.size = 27;
    moon.rotationAngle = 90;
}

// Draw moon
void drawMoon() {
    glPushMatrix();
    glTranslatef(moon.x, moon.y, 0);
    glRotatef(moon.rotationAngle, 0, 0, 1);

    glColor3f(0.9, 0.9, 0.9); // Off-white color
    drawCircle(0, 0, moon.size);

    glColor3f(0.8, 0.8, 0.8); // Light gray color
    drawCircle(-7, -15, moon.size * 0.3);

    glColor3f(0.7, 0.7, 0.7); // Medium grey color
    drawCircle(10, 10, moon.size * 0.2);

    glColor3f(0.6, 0.6, 0.6); // Dark grey color
    drawCircle(-9, 6, moon.size * 0.15);

    glColor3f(0.7, 0.7, 0.7); // Medium grey color
    drawCircle(0, 0, moon.size * 0.09);

    glColor3f(0.7, 0.7, 0.7); // Medium grey color
    drawCircle(10, -12, moon.size * 0.09);

    glPopMatrix();
}


// Draw lives bar
void drawLivesBar() {
    float r, g, b;
    switch (lives) {
    case 5: // Dark green color
        r = 0.0;
        g = 1.0;
        b = 0.0;
        break;
    case 4: // Light green color
        r = 0.4;
        g = 1.0;
        b = 0.4;
        break;
    case 3: // Yellow color
        r = 1.0;
        g = 1.0;
        b = 0.0;
        break;
    case 2: // Orange color
        r = 1.0;
        g = 0.8;
        b = 0.0;
        break;
    case 1: // Red-ish orange color
        r = 1.0;
        g = 0.5;
        b = 0.0;
        break;
    default: // Red color
        r = 1.0;
        g = 0.0;
        b = 0.0;
        break;
    }

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(12, screenHeight - 70);
    glVertex2f(130, screenHeight - 70);
    glVertex2f(130, screenHeight - 60);
    glVertex2f(12, screenHeight - 60);
    glEnd();
}

// Draw outer border
void drawOuterBorder() {
    glColor3f(1.0, 0.5, 0.0); // Orange color
    glLineWidth(5.0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    glLineWidth(5.0);
}

// Draw inner border
void drawInnerBorder() {
    glColor3f(1.0, 1.0, 1.0); // White color
    glBegin(GL_QUADS);

    // Top border
    glVertex2f(0, screenHeight);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(screenWidth, screenHeight - 5);
    glVertex2f(0, screenHeight - 5);

    // Right border
    glVertex2f(screenWidth - 5, screenHeight);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth - 5, 0);

    // Bottom border
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, 5);
    glVertex2f(0, 5);

    // Left border
    glVertex2f(0, screenHeight);
    glVertex2f(5, screenHeight);
    glVertex2f(5, 0);
    glVertex2f(0, 0);

    glEnd();
}

// Handle player movement using special keys and collisions with obstacles/borders
// The collisions with both the obstacles and borders were handled here
// as they would change the location of the player.
void specialKeys(int key, int x, int y) {
    if (lives > 0 && timer > 0 && !target.isHit) {
        float playerX = player.x;
        float playerY = player.y;
        float playerSize = player.size;
        float playerAngle = player.angle;
        float playerSpeed = player.speed;

        switch (key) {
        case GLUT_KEY_LEFT:
            playerX -= playerSpeed;
            playerAngle = 90.0;
            break;
        case GLUT_KEY_RIGHT:
            playerX += playerSpeed;
            playerAngle = 270.0;
            break;
        case GLUT_KEY_UP:
            playerY += playerSpeed;
            playerAngle = 0.0;
            break;
        case GLUT_KEY_DOWN:
            playerY -= playerSpeed;
            playerAngle = 180.0;
            break;

        default:
            break;
        }

        // Check collision with obstacles
        for (Obstacle obstacle : obstacles) {
            bool collide = checkCollision(playerX, playerY, playerSize,
                obstacle.x, obstacle.y, obstacle.size);
            if (collide) {
                playGhostSound(2);

                lives--;
                createObstacles();
                switch (static_cast<int>(playerAngle)) {
                case 0:
                    playerX = player.x + 5;
                    playerY = player.y;
                    break;
                case 180:
                    playerX = player.x - 5;
                    playerY = player.y;
                    break;
                case 90:
                    playerX = player.x;
                    playerY = player.y + 10;
                    break;
                case 270:
                    playerX = player.x;
                    playerY = player.y - 10;
                    break;
                default:
                    break;
                }
            }
        }

        // Check collision with borders
        if (playerX - playerSize < 0 || playerX + playerSize > screenWidth - 10 ||
            playerY - playerSize < 0 || playerY + playerSize > screenHeight - 10) {
            lives--;
            createObstacles();
            playerX = 500.0;
            playerY = 300.0;
        }

        player.x = playerX;
        player.y = playerY;
        player.angle = playerAngle;
        glutPostRedisplay();
    }
}

// Check collision with collectables
void collectablesCollision() {
    for (auto it = collectables.begin(); it != collectables.end(); ) {
        if (!it->isCollected) {
            float collectableX = it->x;
            float collectableY = it->y;
            float collectableSize = it->size;

            bool collide = checkCollision(player.x, player.y, player.size,
                collectableX, collectableY, collectableSize);
            if (collide) {
                playCollectableSound(2);
                it = collectables.erase(it);
                score++;
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }

    for (auto it = extraCollectables.begin(); it != extraCollectables.end(); ) {
        if (!it->isCollected) {
            float collectableX = it->x;
            float collectableY = it->y;
            float collectableSize = it->size;

            bool collide = checkCollision(player.x, player.y, player.size,
                collectableX, collectableY, collectableSize);
            if (collide) {
                it = extraCollectables.erase(it);
                score++;
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }
}

// Check collision with target
void targetCollision() {
    if (!target.isHit) {
        bool collide = checkCollision(player.x, player.y, player.size,
            target.x, target.y, target.size);

        if (collide) {
            target.isHit = true;
            playTargetSound(3);
        }
    }
}


// Green power-up effect (increase speed by 30 for 5 seconds)
void greenEffect() {
    powerUps[0].isActive = true;
    player.speed += 40;
}

// Red power-up effect (add 5 extra collectables for 5 seconds)
void redEffect() {
    powerUps[1].isActive = true;

    for (int i = 0; i < 5; i++) {
        float x = rand() % 950 + 10;
        float y = rand() % 550 + 10;
        float size = 17.0;

        Collectable collectable = { x, y, size, false };
        extraCollectables.push_back(collectable);
    }
}

// Check coollision with power-ups
void powerUpCollision() {
    for (PowerUp& powerUp : powerUps) {
        if (!powerUp.isObtained) {
            float powerUpX = powerUp.x;
            float powerUpY = powerUp.y;
            float powerUpSize = powerUp.size;

            bool collide = checkCollision(player.x, player.y, player.size,
                powerUpX, powerUpY, powerUpSize);
            if (collide) {
                powerUp.isObtained = true;
                if (powerUp.type == 0) {
                    greenEffect();
                }
                else if (powerUp.type == 1) {
                    redEffect();
                }
            }
        }
    }
    glutPostRedisplay();
}

// Handle the power-up effects
void handlePowerUpEffects(int value) {
    for (PowerUp& powerUp : powerUps) {
        if (powerUp.isObtained) {
            powerUp.effectDuration -= 1.0;
            if (powerUp.effectDuration <= 0) {
                if (powerUp.type == 0) {
                    powerUps[0].isActive = false;
                    player.speed = 10;

                }
                else {
                    powerUps[1].isActive = false;
                    extraCollectables.clear();
                }

                for (auto it = powerUps.begin(); it != powerUps.end(); ) {
                    if (it->isObtained && !it->isActive) {
                        it = powerUps.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(1000, handlePowerUpEffects, 0);
}


// Animate collectables and target
void anim(int value) {
    for (Collectable& collectable : collectables) {
        if (!collectable.isCollected) {
            float yOffset = amplitude * sin(animationPhase);
            collectable.y += yOffset;
        }
    }

    for (Collectable& collectable : extraCollectables) {
        if (!collectable.isCollected) {
            float yOffset = amplitude * sin(animationPhase);
            collectable.y += yOffset;
        }
    }

    if (!target.isHit) {
        float yOffset = amplitude * sin(animationPhase);
        target.y += yOffset;
    }

    for (PowerUp& powerUp : powerUps) {
        if (!powerUp.isObtained) {
            float xOffset = amplitude * sin(animationPhase);
            powerUp.x += xOffset;
        }
    }

    moon.rotationAngle += 1.0;

    animationPhase += animationSpeed;

    glutPostRedisplay();

    glutTimerFunc(1000 / 60, anim, 0);
}

// Print text
void print(int x, int y, char* string) {
    int len, i;
    glRasterPos2f(x, y);
    len = (int)strlen(string);
    for (i = 0; i < len; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
    }
}

// Display score
void viewScore() {
    glColor3f(1.0, 1.0, 1.0);
    char scoreText[20];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
    print(12, screenHeight - 30, scoreText);
}

// Display time
void viewTime() {
    glColor3f(1.0, 1.0, 1.0);
    char timeText[20];
    snprintf(timeText, sizeof(timeText), "Time: %d", timer);
    print(12, screenHeight - 53, timeText);
}

// Display "Game Over!" message
void gameOver() {
    glColor3f(1.0, 0.0, 0.0);
    char overText[20];
    snprintf(overText, sizeof(overText), "Game Over!");
    print(screenWidth / 2 - 60, screenHeight / 2, overText);
    playGameOverSound(2);
}

// Display "You Win!" message
void youWin() {
    glColor3f(0.0, 1.0, 0.0);
    char winText[20];
    snprintf(winText, sizeof(winText), "You Win!");
    print(screenWidth / 2 - 60, screenHeight / 2, winText);
    playYouWinSound(2);
}

// Update game timer
void update(int value) {
    if (lives == 0 || timer == 0 || target.isHit) {
        // Game over, do nothing
    }
    else {
        timer--;
        glutTimerFunc(1000, update, 0);
    }
    glutPostRedisplay();
}

void Display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0.0, 0.0, 0.0, 1.0f);

    viewScore();
    viewTime();

    drawObstacles();
    drawCollectables();
    drawTarget();
    drawPlayer();
    drawLivesBar();
    drawPowerUps();

    collectablesCollision();
    targetCollision();
    powerUpCollision();

    drawInnerBorder();
    drawOuterBorder();

    drawMoon();

    if (lives == 0 || timer == 0) {
        gameOver();
    }
    else if (target.isHit) {
        youWin();
    }


    glFlush();
}

void main(int argc, char** argr) {
    glutInit(&argc, argr);

    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(150, 150);

    glutCreateWindow("This is Halloween!");
    glutDisplayFunc(Display);
    glutSpecialFunc(specialKeys);

    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gluOrtho2D(0.0, screenWidth, 0.0, screenHeight);

    // Generate a random number in each run
    srand(static_cast<unsigned>(time(nullptr)));

    createObstacles();
    createCollectables();
    createTarget();
    createPlayer();
    createPowerUps();
    createMoon();

    glutTimerFunc(0, update, 0);
    glutTimerFunc(0, anim, 0);
    glutTimerFunc(1000, handlePowerUpEffects, 0);

    playBackgroundMusic();

    glutMainLoop();

}
