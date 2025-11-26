#include <raylib.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

// -------------------- Structures --------------------

struct Platform { Rectangle r; };
struct Ladder { Rectangle r; };

struct Player {
    Vector2 pos, vel;
    float w = 30, h = 40;
    int health = 10;
    bool grounded = false;
    bool hammer = false;
    float hammerTimer = 0;
    int smashedBarrels = 0;

    Player(float x = 100, float y = 640) { pos = { x,y }; vel = { 0,0 }; }

    Rectangle rect() const { return { pos.x,pos.y,w,h }; }

    void Update(float dt, const std::vector<Platform>& plats, const std::vector<Ladder>& ladders) {
        // Movement
        float dir = 0;
        if (IsKeyDown(KEY_A)) dir = -1;
        if (IsKeyDown(KEY_D)) dir = 1;
        vel.x = dir * 250;

        // Gravity
        vel.y += 1000 * dt;

        // Jump
        if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && grounded) {
            vel.y = -500; grounded = false;
        }

        // Ladder
        bool onLadder = false;
        for (auto& l : ladders) if (CheckCollisionRecs(rect(), l.r)) onLadder = true;
        if (onLadder) {
            vel.y = 0;
            if (IsKeyDown(KEY_W)) pos.y -= 160 * dt;
            if (IsKeyDown(KEY_S)) pos.y += 160 * dt;
        }

        // Move
        pos += vel * dt;

        // Platform collisions
        grounded = false;
        for (auto& p : plats) {
            Rectangle plat = p.r;
            if (CheckCollisionRecs(rect(), plat)) {
                if (vel.y >= 0 && pos.y + h - vel.y * dt <= plat.y) { pos.y = plat.y - h; vel.y = 0; grounded = true; }
                else if (vel.y < 0 && pos.y >= plat.y + plat.height) { pos.y = plat.y + plat.height; vel.y = 0; }
                else { if (vel.x > 0) pos.x = plat.x - w; else if (vel.x < 0) pos.x = plat.x + plat.width; vel.x = 0; }
            }
        }

        // Bounds
        if (pos.x < 0) pos.x = 0;
        if (pos.x + w > 1280) pos.x = 1280 - w;
        if (pos.y > 720) health = 0;

        // Hammer timer
        if (hammer) { hammerTimer -= dt; if (hammerTimer <= 0) hammer = false; }
    }

    void Draw() { DrawRectangleV(pos, { w,h }, BLUE); }
};

enum BarrelState { ROLLING, FALLING };

struct Barrel {
    Vector2 pos, vel;
    float radius = 16;
    bool alive = true;
    int platform = 0;
    BarrelState state = ROLLING;
    bool rollingRight = false;
    float speed = 140;

    Barrel(const std::vector<Platform>& plats, float s) {
        speed = s;
        platform = (int)plats.size() - 1;
        Rectangle p = plats[platform].r;
        pos = { p.x + p.width - radius, p.y - radius * 2 };
        vel = { -speed,0 };
        rollingRight = false;
        state = ROLLING;
    }

    void Update(float dt, const std::vector<Platform>& plats) {
        if (state == ROLLING) {
            vel.y = 0;
            pos.x += vel.x * dt;

            Rectangle plat = plats[platform].r;
            // Edge detection
            if ((!rollingRight && pos.x - radius <= plat.x) || (rollingRight && pos.x + radius >= plat.x + plat.width)) {
                if (platform > 0) {
                    platform--;
                    rollingRight = !rollingRight;
                    vel.x = rollingRight ? speed : -speed;
                    state = FALLING;
                }
                else alive = false;
            }
        }
        else if (state == FALLING) {
            vel.y += 900 * dt;
            pos.y += vel.y * dt;
            // Land on next platform
            Rectangle plat = plats[platform].r;
            if (CheckCollisionCircleRec(pos, radius, plat)) {
                pos.y = plat.y - radius;
                vel.y = 0;
                state = ROLLING;
            }
        }

        // Bounds
        if (pos.x < radius) { pos.x = radius; vel.x = fabs(vel.x); rollingRight = true; }
        if (pos.x > 1280 - radius) { pos.x = 1280 - radius; vel.x = -fabs(vel.x); rollingRight = false; }
        if (pos.y > 720) alive = false;
    }

    void Draw() { if (alive) DrawCircleV(pos, radius, RED); }
};

struct Boss {
    Rectangle r;
    int health = 20;

    Boss(float x = 720, float y = 160) { r = { x,y,96,64 }; }

    void Draw() { DrawTriangle({ r.x,r.y + r.height }, { r.x + r.width / 2,r.y }, { r.x + r.width,r.y + r.height }, BROWN); }
};

struct Hammer {
    Vector2 pos;
    bool active = true;
    float respawnTimer = 0;
    Rectangle rect() const { return { pos.x,pos.y,20,20 }; }
    void Draw() { if (active) DrawRectangleV(pos, { 20,20 }, YELLOW); }
};

struct ScoreManager {
    std::string playerName = "Player";
    int score = 0;
    struct Entry { std::string name; int score; };
    std::vector<Entry> highScores;

    void Load() {
        highScores.clear();
        std::ifstream file("scores.txt");
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string n; int s; ss >> n >> s; highScores.push_back({ n,s });
        }
    }

    void Save() {
        highScores.push_back({ playerName,score });
        std::sort(highScores.begin(), highScores.end(), [](Entry& a, Entry& b) { return a.score > b.score; });
        if (highScores.size() > 5) highScores.resize(5);
        std::ofstream file("scores.txt");
        for (auto& e : highScores) file << e.name << " " << e.score << "\n";
    }
};

// -------------------- Main --------------------
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(0, 0, "Donkey Kong Shapes");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(1280, 720);

    // DK-style platforms
    std::vector<Platform> plats = {
        {{0,680,1280,40}}, {{80,600,480,20}}, {{720,520,480,20}}, {{80,440,480,20}},
        {{720,360,480,20}}, {{80,280,480,20}}, {{720,200,480,20}}
    };
    std::vector<Ladder> ladders = {
        {{500,600,40,80}}, {{720,520,40,80}}, {{480,440,40,80}},
        {{720,360,40,80}}, {{480,280,40,80}}, {{720,200,40,80}}
    };

    Player player;
    Boss boss;
    std::vector<Barrel> barrels;
    Hammer hammer{ {600,400} };

    ScoreManager scoreMgr; scoreMgr.Load();

    float barrelTimer = 0, scoreTimer = 0;
    bool gameOver = false;
    int level = 1;
    float barrelSpeed = 140;
    float barrelInterval = 2.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (!gameOver) {
            player.Update(dt, plats, ladders);

            // Hammer respawn every 45 sec
            hammer.respawnTimer += dt;
            if (!hammer.active && hammer.respawnTimer >= 45.0f) { hammer.active = true; hammer.respawnTimer = 0; }

            // Spawn barrels
            barrelTimer += dt; scoreTimer += dt;
            if (barrelTimer >= barrelInterval) {
                barrels.emplace_back(plats, barrelSpeed);
                barrelTimer = 0;
            }

            for (auto& b : barrels) if (b.alive) b.Update(dt, plats);
            barrels.erase(std::remove_if(barrels.begin(), barrels.end(), [](Barrel& b) {return !b.alive;}), barrels.end());

            // Hammer pickup
            if (hammer.active && CheckCollisionRecs(player.rect(), hammer.rect())) {
                hammer.active = false; player.hammer = true; player.hammerTimer = 8.0f;
            }

            // Barrel collisions
            for (auto& b : barrels) {
                if (b.alive && CheckCollisionCircleRec(b.pos, b.radius, player.rect())) {
                    if (player.hammer) {
                        b.alive = false; player.smashedBarrels++; scoreMgr.score += 50;
                        if (player.smashedBarrels >= 15 && boss.health > 0) boss.health--;
                    }
                    else { player.health--; b.alive = false; if (player.health <= 0) gameOver = true; }
                }
            }

            // Score over time
            if (scoreTimer >= 1.0f) { scoreMgr.score += 10; scoreTimer = 0; }

            // Level progression
            if (boss.health <= 0 && level < 10) {
                level++;
                boss.health = 20;
                player.smashedBarrels = 0;
                barrels.clear();
                player.pos = { 100,640 };
                barrelSpeed += 20; barrelInterval *= 0.9f; hammer.active = true;
            }
        }

        // Draw
        BeginTextureMode(target);
        ClearBackground(BLACK);
        for (auto& p : plats) DrawRectangleRec(p.r, DARKGRAY);
        for (auto& l : ladders) DrawRectangleRec(l.r, BROWN);
        boss.Draw();
        for (auto& b : barrels) b.Draw();
        hammer.Draw();
        player.Draw();
        DrawText(TextFormat("Health: %d", player.health), 10, 10, 20, WHITE);
        DrawText(TextFormat("Score: %d", scoreMgr.score), 10, 40, 20, YELLOW);
        DrawText(TextFormat("Level: %d", level), 10, 70, 20, GREEN);
        if (gameOver) {
            DrawText("GAME OVER", 480, 300, 60, RED);
            DrawText("Press ENTER to restart", 480, 380, 24, WHITE);
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture,
            { 0,0,(float)target.texture.width,-(float)target.texture.height },
            { 0,0,(float)GetScreenWidth(),(float)GetScreenHeight() },
            { 0,0 }, 0, WHITE
        );
        EndDrawing();

        if (gameOver && IsKeyPressed(KEY_ENTER)) {
            gameOver = false;
            player = Player(); boss = Boss(); barrels.clear();
            hammer.active = true; hammer.respawnTimer = 0;
            player.health = 10; scoreMgr.score = 0; player.smashedBarrels = 0;
            level = 1; barrelSpeed = 140; barrelInterval = 2.0f;
        }
    }

    scoreMgr.Save();
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
