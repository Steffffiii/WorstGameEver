#include <raylib.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>


struct Platform { Rectangle r; };
struct Ladder { Rectangle r; };


struct Barrel {
	Vector2 pos, vel; float radius; bool alive; int curPlat; bool rollingRight;
	Barrel() { alive = false; }
	Barrel(const std::vector<Platform>& plats) { alive = true; radius = 16; curPlat = (int)plats.size() - 1; const auto& p = plats[curPlat].r; pos = { p.x + p.width - 30,p.y - radius - 1 }; rollingRight = false; vel = { -140,0 }; }
	void Update(float dt, const std::vector<Platform>& plats) {
		vel.y += 900 * dt; pos.x += vel.x * dt; pos.y += vel.y * dt;
		if (curPlat >= 0 && curPlat < (int)plats.size()) {
			auto& plat = plats[curPlat].r;
			if (CheckCollisionCircleRec(pos, radius, plat)) {
				pos.y = plat.y - radius; vel.y = 0;
				if ((!rollingRight && pos.x - radius <= plat.x) || (rollingRight && pos.x + radius >= plat.x + plat.width)) {
					if (curPlat > 0) { curPlat--; rollingRight = !rollingRight; vel.x = rollingRight ? 140 : -140; }
					else alive = false;
				}
			}
		}
		if (pos.x < radius) { pos.x = radius; vel.x = fabs(vel.x); rollingRight = true; }
		if (pos.x > 1920 - radius) { pos.x = 1920 - radius; vel.x = -fabs(vel.x); rollingRight = false; }
		if (pos.y > 1600) alive = false;
	}
	void Draw(Texture2D& tex) { if (alive) DrawTexturePro(tex, { 0,0,(float)tex.width,(float)tex.height }, { pos.x - radius,pos.y - radius,radius * 2,radius * 2 }, { 0,0 }, 0, ORANGE); }
};


struct Fire {
	Vector2 pos; Vector2 vel; float timer; bool alive;
	Fire(Vector2 p) { pos = p; vel = { 40,0 }; timer = 0; alive = true; }
	void Update(float dt) { pos.x += vel.x * dt; pos.y += vel.y * dt; timer += dt; if (timer > 0.5f) { vel.y = 200; } if (pos.y > 1600) alive = false; }
	void Draw(Texture2D& t) { if (alive) DrawTexturePro(t, { 0,0,(float)t.width,(float)t.height }, { pos.x - 12,pos.y - 12,24,24 }, { 0,0 }, 0, RED); }
};


struct HammerItem { Rectangle r; bool taken; HammerItem(float x, float y) { r = { x,y,28,28 }; taken = false; } void Draw(Texture2D& t) { if (!taken) DrawTexturePro(t, { 0,0,(float)t.width,(float)t.height }, r, { 0,0 }, 0, GOLD); } };


struct Boss { Rectangle r; float timer; float interval; Boss(float x, float y) { r = { x,y,96,64 }; timer = 0; interval = 2.0f; } bool ShouldThrow(float dt) { timer += dt; if (timer >= interval) { timer = 0; interval = 1.5f + GetRandomValue(0, 200) / 100.0f; return true; } return false; } void Draw(Texture2D& t) { DrawTexturePro(t, { 0,0,(float)t.width,(float)t.height }, r, { 0,0 }, 0, BROWN); } };


struct Player {
	Vector2 pos; Vector2 vel; float w, h; bool grounded; bool climbing; int lives; bool hasHammer; float hammerT;
	int animFrame; float animTimer; Player() { pos = { 100,1400 }; vel = { 0,0 }; w = 28; h = 44; grounded = false; climbing = false; lives = 3; hasHammer = false; hammerT = 0; animFrame = 0; animTimer = 0; }
	Rectangle rect() const { return { pos.x,pos.y,w,h }; }
	void Update(float dt, const std::vector<Platform>& plats, const std::vector<Ladder>& ladders) {
		float dir = 0; if (IsKeyDown(KEY_A)) dir = -1; if (IsKeyDown(KEY_D)) dir = 1; vel.x = dir * 220; climbing = false; for (auto& l : ladders) if (CheckCollisionRecs(rect(), l.r)) climbing = true; if (climbing) { vel.y = 0; if (IsKeyDown(KEY_W)) pos.y -= 160 * dt; if (IsKeyDown(KEY_S)) pos.y += 160 * dt; }
		else { vel.y += 1100 * dt; if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W)) && grounded) { vel.y = -520; grounded = false; } }
		pos.x += vel.x * dt; pos.y += vel.y * dt; grounded = false; Rectangle r = rect(); for (auto& p : plats) { if (CheckCollisionRecs(r, p.r)) { if (vel.y > 0 && r.y + r.height - vel.y * dt <= p.r.y + 1) { pos.y = p.r.y - h; vel.y = 0; grounded = true; r = rect(); } else if (vel.y < 0 && r.y >= p.r.y + p.r.height - 1) { pos.y = p.r.y + p.r.height; vel.y = 0; r = rect(); } else { if (vel.x > 0) pos.x = p.r.x - r.width; else if (vel.x < 0) pos.x = p.r.x + p.r.width; vel.x = 0; r = rect(); } } }
		if (pos.x < 0) pos.x = 0; if (pos.x + w > 1920) pos.x = 1920 - w; if (pos.y > 1700) { lives--; pos = { 100,1400 }; vel = { 0,0 }; }
		if (hasHammer) { hammerT -= dt; if (hammerT <= 0) { hasHammer = false; hammerT = 0; } }
		animTimer += dt; if (animTimer > 0.12f) { animTimer = 0; animFrame = (animFrame + 1) % 4; }
	}
	void Draw(Texture2D& tex) { Rectangle dest = { pos.x,pos.y,w,h }; Rectangle src = { (float)(animFrame % 4) * tex.width / 4.0f,0,(float)tex.width / 4.0f,(float)tex.height }; DrawTexturePro(tex, src, dest, { 0,0 }, 0, hasHammer ? YELLOW : WHITE); }
};


struct ScoreManager { std::string name; int score; struct E { std::string n; int s; }; std::vector<E> hs; void Load() { hs.clear(); std::ifstream f("scores.txt"); std::string line; while (std::getline(f, line)) { std::stringstream ss(line); E e; ss >> e.n >> e.s; if (e.n.size()) hs.push_back(e); } } void Save() { hs.push_back({ name,score }); std::sort(hs.begin(), hs.end(), [](const E& a, const E& b) {return a.s > b.s;}); if (hs.size() > 5) hs.resize(5); std::ofstream o("scores.txt"); for (auto& e : hs) o << e.n << " " << e.s << "\n"; } };


int main()
{
	InitWindow(1280, 720, "Donkey Kong - Full"); SetTargetFPS(60);
	Texture2D texPlayer = LoadTexture("player_spritesheet.png");
	Texture2D texBarrel = LoadTexture("barrel.png");
	Texture2D texBoss = LoadTexture("boss.png");
	Texture2D texHammer = LoadTexture("hammer.png");
	Texture2D texFire = LoadTexture("fire.png");
	Sound sJump = LoadSound("jump.wav");
	Sound sHit = LoadSound("hit.wav");
	Sound sSmash = LoadSound("smash.wav");


	std::vector<Platform> plats;
	std::vector<Ladder> ladders;
	std::vector<Barrel> barrels;
	std::vector<Fire> fires;


	plats.push_back({ {0,680,1280,40} }); plats.push_back({ {80,560,420,20} }); plats.push_back({ {520,440,420,20} }); plats.push_back({ {80,320,420,20} }); plats.push_back({ {520,200,420,20} });
	ladders.push_back({ {230,560 - 120,40,120} }); ladders.push_back({ {670,440 - 120,40,120} }); ladders.push_back({ {230,320 - 120,40,120} });


	HammerItem hammer(600, 380);
	Boss boss(640, 120);
	Player player;
	ScoreManager score; score.score = 0;
	score.name = "PLAYER";
	score.Load();


	float barrelSpawn = 0; float scoreTimer = 0; bool gameOver = false; int level = 1;


	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
		if (!gameOver) {
			player.Update(dt, plats, ladders);
			if (!hammer.taken && CheckCollisionRecs(player.rect(), hammer.r)) { hammer.taken = true; player.hasHammer = true; player.hammerT = 8; PlaySound(sSmash); }
			if (boss.ShouldThrow(dt)) barrels.emplace_back(plats);
			for (auto& b : barrels) if (b.alive) b.Update(dt, plats);
			barrels.erase(std::remove_if(barrels.begin(), barrels.end(), [](Barrel& b) {return !b.alive;}), barrels.end());
			for (auto& b : barrels) { if (b.alive && CheckCollisionCircleRec(b.pos, b.radius, player.rect())) { if (player.hasHammer) { b.alive = false; score.score += 100; PlaySound(sSmash); } else { player.lives--; PlaySound(sHit); player.pos = { 100,1400 }; player.vel = { 0,0 }; if (player.lives <= 0) gameOver = true; } } }
			scoreTimer += dt; if (scoreTimer > 1) { score.score += 10; scoreTimer = 0; }
		}


		return 0;
	}
}