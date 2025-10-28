#include <raylib.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>


namespace MathUtils {
    inline float clamp(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }
    inline float length(const Vector2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
}


const int BASE_WIDTH = 1280;
const int BASE_HEIGHT = 720;


struct Platform {
    Rectangle rect;
    Color color;
    Platform(float x, float y, float w, float h, Color c)
        : rect{ x, y, w, h }, color(c) {
    }
    void Draw() const { DrawRectangleRec(rect, color); }
};


struct Enemy {
    Vector2 pos;
    Vector2 vel;
    float radius = 20;
    bool alive = true;
    float gravity = 900.0f;
    int currentPlatform = 0;
    bool rollingRight = false;

    Enemy(const std::vector<Platform>& plats) {
        currentPlatform = (int)plats.size() - 1; // top platform
        const auto& top = plats[currentPlatform];
        pos = { top.rect.x + top.rect.width - radius, top.rect.y - radius * 2 };
        vel = { -100, 0 };
        rollingRight = false;
    }

    void Update(float dt, const std::vector<Platform>& plats) {
        if (!alive) return;

        pos.x += vel.x * dt;
        vel.y += gravity * dt;
        pos.y += vel.y * dt;

        const auto& plat = plats[currentPlatform];

        if (CheckCollisionCircleRec(pos, radius, plat.rect)) {
            pos.y = plat.rect.y - radius;
            vel.y = 0;

            if ((!rollingRight && pos.x - radius <= plat.rect.x) ||
                (rollingRight && pos.x + radius >= plat.rect.x + plat.rect.width)) {
                if (currentPlatform > 0) {
                    currentPlatform--;
                    rollingRight = !rollingRight;
                    vel.x = (rollingRight ? 100 : -100);
                }
                else {
                    alive = false;
                }
            }
        }

        pos.x = MathUtils::clamp(pos.x, radius, (float)BASE_WIDTH - radius);
    }

    void Draw() const { if (alive) DrawCircleV(pos, radius, RED); }
};


struct Player {
    Vector2 pos;
    Vector2 vel;
    float width = 30;
    float height = 40;
    float gravity = 1000.0f;
    float moveSpeed = 250.0f;
    float jumpVel = 500.0f;
    bool grounded = false;
    int health = 100;

    Player(float x, float y) { pos = { x, y }; vel = { 0, 0 }; }

    Rectangle getRect() const { return { pos.x, pos.y, width, height }; }

    void Update(float dt, const std::vector<Platform>& platforms) {
        float dir = 0.0f;
        if (IsKeyDown(KEY_A)) dir -= 1.0f;
        if (IsKeyDown(KEY_D)) dir += 1.0f;
        vel.x = dir * moveSpeed;

        vel.y += gravity * dt;

        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W)) && grounded) {
            vel.y = -jumpVel;
            grounded = false;
        }

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        grounded = false;
        Rectangle r = getRect();

        for (const auto& plat : platforms) {
            if (CheckCollisionRecs(r, plat.rect)) {
                pos.y = plat.rect.y - height;
                vel.y = 0;
                grounded = true;
                r = getRect();
            }
        }

        pos.x = MathUtils::clamp(pos.x, 0.0f, (float)BASE_WIDTH - width);
        if (pos.y > BASE_HEIGHT) health = 0;
    }

    void Draw() const { DrawRectangleV(pos, { width, height }, BLUE); }
};


struct ScoreManager {
    std::string playerName;
    int score = 0;

    struct Entry {
        std::string name;
        int score;
    };
    std::vector<Entry> highScores;

    void LoadHighScores() {
        highScores.clear();
        std::ifstream file("scores.txt");
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string name;
            int s;
            ss >> name >> s;
            highScores.push_back({ name, s });
        }
    }

    void SaveHighScore() {
        highScores.push_back({ playerName, score });
        std::sort(highScores.begin(), highScores.end(), [](const Entry& a, const Entry& b) {
            return a.score > b.score;
            });
        if (highScores.size() > 5) highScores.resize(5);

        std::ofstream file("scores.txt");
        for (auto& e : highScores) {
            file << e.name << " " << e.score << "\n";
        }
    }

    int GetTopScore() const {
        return highScores.empty() ? 0 : highScores[0].score;
    }
};

enum GameState { MENU, GAME, GAME_OVER };

int main() {
    InitWindow(BASE_WIDTH, BASE_HEIGHT, "Kong Mario");
    SetTargetFPS(60);
    RenderTexture2D target = LoadRenderTexture(BASE_WIDTH, BASE_HEIGHT);

    std::vector<Platform> platforms = {
        {0, 680, 1280, 40, DARKGRAY},
        {150, 550, 300, 20, GRAY},
        {500, 450, 350, 20, GRAY},
        {900, 350, 300, 20, GRAY},
        {300, 250, 300, 20, GRAY}
    };

    ScoreManager scoreManager;
    scoreManager.LoadHighScores();

    GameState state = MENU;
    char nameBuffer[32] = "";
    int letterCount = 0;

    Player player(100, 600);
    std::vector<Enemy> enemies;
    float enemyTimer = 0;
    float scoreTimer = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (state == MENU) {
            // Name input
            int key = GetCharPressed();
            while (key > 0) {
                if (letterCount < 31 && key >= 32 && key <= 125) {
                    nameBuffer[letterCount] = (char)key;
                    letterCount++;
                    nameBuffer[letterCount] = '\0';
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
                letterCount--;
                nameBuffer[letterCount] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                scoreManager.playerName = nameBuffer;
                state = GAME;
                player = Player(100, 600);
                enemies.clear();
                enemyTimer = 0;
                scoreTimer = 0;
                scoreManager.score = 0;
            }
        }

        else if (state == GAME) {
            player.Update(dt, platforms);
            enemyTimer += dt;
            scoreTimer += dt;

            if (enemyTimer > 3.0f) {
                enemyTimer = 0;
                enemies.emplace_back(platforms);
            }

            for (auto& e : enemies)
                e.Update(dt, platforms);

            enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                [](const Enemy& e) {return !e.alive;}), enemies.end());

            for (auto& e : enemies)
                if (CheckCollisionCircleRec(e.pos, e.radius, player.getRect()))
                    player.health -= 1;

            if (scoreTimer > 1.0f) {
                scoreManager.score += 10;
                scoreTimer = 0;
            }

            if (player.health <= 0) {
                scoreManager.SaveHighScore();
                state = GAME_OVER;
            }
        }

        else if (state == GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER)) {
                state = MENU;
                letterCount = 0;
                nameBuffer[0] = '\0';
                scoreManager.LoadHighScores();
            }
        }


        BeginTextureMode(target);
        ClearBackground(BLACK);

        if (state == MENU) {
            DrawText("KONG MARIO", 480, 150, 60, YELLOW);
            DrawText("Enter your name:", 460, 300, 30, WHITE);
            DrawText(nameBuffer, 460, 350, 40, LIGHTGRAY);
            DrawText("Press [ENTER] to start", 460, 450, 25, GRAY);
            DrawText("High Scores:", 460, 500, 25, GREEN);
            for (size_t i = 0; i < scoreManager.highScores.size(); ++i) {
                DrawText(TextFormat("%s %d", scoreManager.highScores[i].name.c_str(),
                    scoreManager.highScores[i].score),
                    460, 530 + i * 30, 25, WHITE);
            }
        }

        else if (state == GAME) {
            for (auto& p : platforms) p.Draw();
            for (auto& e : enemies) e.Draw();
            player.Draw();
            DrawText(TextFormat("Health: %i", player.health), 20, 20, 30, WHITE);
            DrawText(TextFormat("Score: %i", scoreManager.score), 20, 60, 30, YELLOW);
            DrawText(TextFormat("Top Score: %i", scoreManager.GetTopScore()), 20, 100, 25, GREEN);
        }

        else if (state == GAME_OVER) {
            DrawText("GAME OVER!", 520, 250, 60, RED);
            DrawText(TextFormat("Your Score: %d", scoreManager.score), 520, 350, 30, WHITE);
            DrawText("High Scores:", 520, 390, 30, GREEN);
            for (size_t i = 0; i < scoreManager.highScores.size(); ++i) {
                DrawText(TextFormat("%s %d", scoreManager.highScores[i].name.c_str(),
                    scoreManager.highScores[i].score),
                    520, 430 + i * 30, 25, WHITE);
            }
            DrawText("Press [ENTER] to return to menu", 460, 550, 25, GRAY);
        }

        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture,
            { 0,0,(float)target.texture.width,-(float)target.texture.height },
            { 0,0,(float)GetScreenWidth(),(float)GetScreenHeight() },
            { 0,0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
