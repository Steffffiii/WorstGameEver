

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm> 


const int SCREEN_W = 800;
const int SCREEN_H = 720;
const int FPS = 60;

struct RectF { float x, y, w, h; };
struct Platform { RectF r; bool isSlope; float slopeDir; };
struct Ladder { RectF r; };
struct Barrel {
    Vector2 pos;
    Vector2 vel;
    float size;
    bool active;
};

enum PlayerState { PS_GROUND, PS_JUMP, PS_CLIMB, PS_FALL };


std::vector<Platform> platforms;
std::vector<Ladder> ladders;
std::vector<Barrel> barrels;

int score = 0;
int lives = 3;
int levelNumber = 1;

Vector2 playerPos;
Vector2 playerVel;

PlayerState pState = PS_GROUND;

const float PLAYER_SPEED = 160.0f;
const float PLAYER_JUMP = 420.0f;
const float GRAVITY = 1000.0f;
const float CLIMB_SPEED = 100.0f;

const float PLAYER_W = 28;
const float PLAYER_H = 40;

Vector2 apePos;

float barrelTimer = 0;
float barrelInterval = 2.0f;


bool hammerActive = false;       
float hammerTimeLeft = 0.0f;    
bool hammerExists = false;     
Rectangle hammerPickup = { 0,0,0,0 }; 


bool CheckRectCollision(const RectF& a, const RectF& b) {
    return !(a.x + a.w < b.x || b.x + b.w < a.x ||
        a.y + a.h < b.y || b.y + b.h < a.y);
}

bool PlayerOnPlatform(RectF& out) {
    RectF p{ playerPos.x, playerPos.y, PLAYER_W, PLAYER_H };

    for (auto& pl : platforms) {
        RectF r = pl.r;

        if (p.x + p.w > r.x + 2 &&
            p.x < r.x + r.w - 2) {

            float feet = p.y + p.h;

            if (feet >= r.y - 4 && feet <= r.y + 8) {
                out = r;
                return true;
            }
        }
    }
    return false;
}

int LadderUnderPlayerIndex() {
    RectF p{ playerPos.x + 4, playerPos.y, PLAYER_W - 8, PLAYER_H };

    for (int i = 0; i < (int)ladders.size(); i++) {
        if (CheckRectCollision(p, ladders[i].r)) return i;
    }
    return -1;
}
bool BarrelOnLadder(const Barrel& b, int& outIndex) {
    Rectangle br{ b.pos.x, b.pos.y, b.size, b.size };
    for (int i = 0; i < (int)ladders.size(); ++i) {
        Rectangle lr{ ladders[i].r.x, ladders[i].r.y, ladders[i].r.w, ladders[i].r.h };
        if (CheckCollisionRecs(br, lr)) {
            outIndex = i;
            return true;
        }
    }
    return false;
}


void BuildLevel(int level) {
    platforms.clear();
    ladders.clear();
    barrels.clear();

    float margin = 60;
    float ph = 16;

    int rows = 5;
    float gap = (SCREEN_H - 200) / rows;

    for (int i = 0; i < rows; i++) {
        float y = SCREEN_H - 100 - i * gap;
        float w = SCREEN_W - margin * 2;
        float x = margin;

        float leftCut = (i % 2 == 0) ? 0 : 120;
        float rightCut = (i % 2 == 0) ? 120 : 0;

        Platform p;
        p.r = { x + leftCut, y, w - leftCut - rightCut, ph };
        p.isSlope = false;
        p.slopeDir = 0;

        platforms.push_back(p);
    }

  
    platforms.push_back({ {0, SCREEN_H - 40, SCREEN_W, 40}, false, 0 });

    
    for (int i = 0; i < rows; i++) {
        float topY = SCREEN_H - 100 - i * gap;
        float bottomY = (i == 0)
            ? SCREEN_H - 40
            : (SCREEN_H - 100 - (i - 1) * gap + ph);

        float lx = (i % 2 == 0) ? 80.0f : SCREEN_W - 120.0f;

        ladders.push_back({ {lx, topY + ph, 36, bottomY - topY - ph} });
    }

   
    apePos.x = (level % 2 == 1) ? 100 : SCREEN_W - 140;
    apePos.y = SCREEN_H - 100 - (rows - 1) * gap - 60;

    
    playerPos = { 50, SCREEN_H - 40 - PLAYER_H };
    playerVel = { 0, 0 };
    pState = PS_GROUND;

    barrelTimer = 0;
    barrelInterval = 2.0f;

    
    if ((int)platforms.size() > 2) {
        Platform& plat = platforms[2]; 
        hammerPickup.width = 28;
        hammerPickup.height = 28;
        hammerPickup.x = plat.r.x + plat.r.w * 0.5f - hammerPickup.width * 0.5f;
        hammerPickup.y = plat.r.y - hammerPickup.height; 
        hammerExists = true;
        hammerActive = false;
        hammerTimeLeft = 0.0f;
    }
    else {
        hammerExists = false;
    }
}


void SpawnBarrel() {
    Barrel b;
    b.pos = { apePos.x + 40, apePos.y + 20 };

    if (apePos.x < SCREEN_W / 2)
        b.vel = { 140, 0 };
    else
        b.vel = { -140, 0 };

    b.active = true;
    b.size = 18;

    barrels.push_back(b);
}

void UpdateBarrels(float dt) {
    for (auto& b : barrels) {
        if (!b.active) continue;

        
        b.vel.y += GRAVITY * dt;

       
        
        if (fabs(b.vel.x) > 20.0f && fabs(b.vel.y) < 200.0f) {
            int ladIdx = -1;
            if (BarrelOnLadder(b, ladIdx)) {
                
                if ((rand() % 100) < 25) {
                    b.vel.x = 0;
                    b.vel.y = 250; 
                  
                    if (ladIdx >= 0 && ladIdx < (int)ladders.size()) {
                        float ladderCenterX = ladders[ladIdx].r.x + ladders[ladIdx].r.w * 0.5f - b.size * 0.5f;
                        b.pos.x = ladderCenterX;
                    }
                    
                }
            }
        }

        
        b.pos = Vector2Add(b.pos, Vector2Scale(b.vel, dt));

        
        for (auto& pl : platforms) {
            RectF r = pl.r;

            bool alignedX =
                b.pos.x + b.size > r.x + 2 &&
                b.pos.x < r.x + r.w - 2;

            if (alignedX) {
                float feet = b.pos.y + b.size;

                if (b.vel.y >= 0 &&
                    feet >= r.y &&
                    feet <= r.y + 10) {

                    
                    b.pos.y = r.y - b.size;
                    b.vel.y = 0;

                    
                    if (b.pos.x <= r.x + 4) b.vel.x = 140;
                    if (b.pos.x + b.size >= r.x + r.w - 4) b.vel.x = -140;

                    if (fabs(b.vel.x) < 1) {
                        b.vel.x = (levelNumber % 2 == 1) ? 140 : -140;
                    }
                }
            }
        }

        
        if (b.pos.x < -100 || b.pos.x > SCREEN_W + 100 ||
            b.pos.y > SCREEN_H + 200) {
            b.active = false;
        }
    }
}
void UpdatePlayer(float dt) {
    bool left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    bool right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    bool up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    bool down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    bool jumpKey = IsKeyPressed(KEY_SPACE);

    int ladderIdx = LadderUnderPlayerIndex();
    bool onLadder = ladderIdx != -1;

    
    if ((onLadder && (up || down)) || pState == PS_CLIMB) {
        pState = PS_CLIMB;

        if (up)   playerPos.y -= CLIMB_SPEED * dt;
        if (down) playerPos.y += CLIMB_SPEED * dt;

       
        if (jumpKey) {
            pState = PS_JUMP;
            playerVel.y = -PLAYER_JUMP;
            return;
        }

        
        RectF under;
        bool atPlat = PlayerOnPlatform(under);
        float chestY = playerPos.y + PLAYER_H * 0.3f;

        if (atPlat && chestY < under.y) {
            
        }
        else if (atPlat) {
            
            pState = PS_GROUND;
            playerPos.y = under.y - PLAYER_H;
            return;
        }

        
        if (!onLadder && !atPlat) {
            pState = PS_FALL;
        }

        return; 
    }

    
    float targetX = 0;
    if (left)  targetX = -PLAYER_SPEED;
    if (right) targetX = PLAYER_SPEED;

    playerVel.x = targetX;
    playerVel.y += GRAVITY * dt;

    playerPos = Vector2Add(playerPos, Vector2Scale(playerVel, dt));

    if (playerPos.x < 0) playerPos.x = 0;
    if (playerPos.x + PLAYER_W > SCREEN_W) playerPos.x = SCREEN_W - PLAYER_W;

   
    RectF under;
    if (PlayerOnPlatform(under)) {
        float feet = playerPos.y + PLAYER_H;

        if (feet > under.y) {
            playerPos.y = under.y - PLAYER_H;
            playerVel.y = 0;
            pState = PS_GROUND;
        }
    }
    else if (playerVel.y > 0) {
        pState = PS_FALL;
    }

    if ((pState == PS_GROUND || pState == PS_FALL) && jumpKey) {
        RectF tmp;
        if (PlayerOnPlatform(tmp)) {
            pState = PS_JUMP;
            playerVel.y = -PLAYER_JUMP;
        }
    }

    if (playerPos.y > SCREEN_H + 200) {
        lives--;
        playerPos = { 50, SCREEN_H - 120 - PLAYER_H };
        playerVel = { 0,0 };
        pState = PS_GROUND;
    }
}

void CheckPlayerBarrelCollisions() {
    Rectangle pr{ playerPos.x, playerPos.y, PLAYER_W, PLAYER_H };

    for (auto& b : barrels) {
        if (!b.active) continue;

        Rectangle br{ b.pos.x, b.pos.y, b.size, b.size };

        if (CheckCollisionRecs(pr, br)) {
            if (hammerActive) {
                
                b.active = false;
                score += 100;
                continue;
            }

            
            lives--;
            score = score > 50 ? score - 50 : 0;

            playerPos = { 50, SCREEN_H - 120 - PLAYER_H };
            playerVel = { 0,0 };
            pState = PS_GROUND;
        }
    }
}

bool CheckWinCondition() {
    if (playerPos.y + PLAYER_H < apePos.y + 20 &&
        fabs(playerPos.x - SCREEN_W / 2) < SCREEN_W / 2)
        return true;

    return false;
}


void DrawPlayer() {
    DrawRectangleV(playerPos, { PLAYER_W, PLAYER_H }, MAROON);

    Vector2 headPos = Vector2Add(playerPos, { PLAYER_W * 0.5f, 10 });
    DrawCircleV(headPos, 10, PINK);

    DrawRectangleLines((int)playerPos.x, (int)playerPos.y,
        (int)PLAYER_W, (int)PLAYER_H, BLACK);

    
    if (hammerActive) {
        Vector2 swingPos = Vector2Add(playerPos, { PLAYER_W + 8, PLAYER_H * 0.3f });
        DrawCircleV(swingPos, 14, GOLD);
    }
}

void DrawApe() {
    Rectangle r{ apePos.x, apePos.y, 80, 64 };
    DrawRectangleRec(r, BROWN);

    DrawCircle((int)apePos.x + 20, (int)apePos.y + 24, 8, BEIGE);
    DrawCircle((int)apePos.x + 60, (int)apePos.y + 24, 8, BEIGE);

    DrawText("APEX APE", (int)apePos.x + 6, (int)apePos.y - 18,
        12, YELLOW);
}

void DrawPlatformsAndLadders() {
    for (auto& p : platforms) {
        DrawRectangle((int)p.r.x, (int)p.r.y, (int)p.r.w, (int)p.r.h,
            LIGHTGRAY);
        DrawRectangleLines((int)p.r.x, (int)p.r.y,
            (int)p.r.w, (int)p.r.h, DARKGRAY);
    }

    for (auto& lad : ladders) {
        DrawRectangle((int)lad.r.x, (int)lad.r.y,
            (int)lad.r.w, (int)lad.r.h, ORANGE);

        int steps = (int)(lad.r.h / 20);

        for (int i = 0;i < steps;i++) {
            int y = (int)(lad.r.y + i * 20);
            DrawRectangle((int)lad.r.x, y, (int)lad.r.w, 4, BROWN);
        }
    }

    if (hammerExists && !hammerActive) {
        DrawRectangleRec(hammerPickup, GOLD);
        DrawText("H", (int)hammerPickup.x + 6, (int)hammerPickup.y + 4, 18, DARKBROWN);
    }
}

void DrawBarrels() {
    for (auto& b : barrels) {
        if (!b.active) continue;

        Vector2 center = Vector2Add(b.pos, { b.size * 0.5f, b.size * 0.5f });
        DrawCircleV(center, b.size * 0.5f, DARKBROWN);
    }
}


int main() {
    InitWindow(SCREEN_W, SCREEN_H, "ApeClimb");
    SetTargetFPS(FPS);
    srand((unsigned)time(nullptr));

    BuildLevel(levelNumber);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        barrelTimer += dt;
        if (barrelTimer >= barrelInterval) {
            barrelTimer = 0;
            SpawnBarrel();

            barrelInterval = 1.0f + ((float)rand() / RAND_MAX) * 1.0f;
        }

        UpdatePlayer(dt);
        UpdateBarrels(dt);

        
        Rectangle pr{ playerPos.x, playerPos.y, PLAYER_W, PLAYER_H };
        if (hammerExists && !hammerActive && CheckCollisionRecs(pr, hammerPickup)) {
            hammerActive = true;
            hammerTimeLeft = 10.0f; 
            hammerExists = false;
        }

      
        if (hammerActive) {
            hammerTimeLeft -= dt;
            if (hammerTimeLeft <= 0.0f) {
                hammerActive = false;
                hammerTimeLeft = 0.0f;
            }
        }

        CheckPlayerBarrelCollisions();

        if (CheckWinCondition()) {
            score += 500;
            levelNumber++;
            BuildLevel(levelNumber);
        }

        barrels.erase(
            std::remove_if(barrels.begin(), barrels.end(),
                [](Barrel& b) { return !b.active; }),
            barrels.end()
        );

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText(TextFormat("Score: %d", score), 12, 8, 20, DARKGRAY);
        DrawText(TextFormat("Lives: %d", lives),
            SCREEN_W - 140, 8, 20, DARKGRAY);
        DrawText(TextFormat("Level: %d", levelNumber),
            SCREEN_W / 2 - 40, 8, 20, DARKGRAY);

        DrawPlatformsAndLadders();
        DrawApe();
        DrawText("RESCUE", SCREEN_W / 2 - 40, 40, 18, RED);

        DrawBarrels();
        DrawPlayer();

        DrawText("Move: A/D or ←/→   Jump: SPACE   Climb: W/S or ↑/↓",
            10, SCREEN_H - 24, 14, GRAY);

        
        if (hammerActive) {
            DrawText(TextFormat("HAMMER: %.1fs", hammerTimeLeft), 12, 36, 18, ORANGE);
        }

        if (lives <= 0) {
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.6f));

            DrawText("GAME OVER",
                SCREEN_W / 2 - 160, SCREEN_H / 2 - 40,
                48, RAYWHITE);

            DrawText(TextFormat("Final Score: %d", score),
                SCREEN_W / 2 - 120, SCREEN_H / 2 + 16,
                28, RAYWHITE);

            DrawText("Press R to restart",
                SCREEN_W / 2 - 100, SCREEN_H / 2 + 64,
                20, RAYWHITE);

            if (IsKeyPressed(KEY_R)) {
                score = 0;
                lives = 3;
                levelNumber = 1;
                BuildLevel(levelNumber);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
