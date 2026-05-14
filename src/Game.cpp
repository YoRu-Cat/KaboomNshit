#include "Game.h"
#include "Config.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>

Game::Game()
    : fireCooldown(0.0f)
    , paused(false)
    , showHelp(true)
{}

bool Game::Initialize() {
    InitWindow(cfg::SCREEN_W, cfg::SCREEN_H, "KaboomNshit");
    SetTargetFPS(144);
    DisableCursor();

    world.Generate(1337, 22);
    SpawnEnemies(8);
    return IsWindowReady();
}

void Game::Shutdown() {
    if (IsWindowReady()) CloseWindow();
}

bool Game::ShouldClose() const {
    return WindowShouldClose();
}

void Game::SpawnEnemies(int count) {
    enemies.clear();
    SetRandomSeed(424242);
    int placed = 0;
    int attempts = 0;
    while (placed < count && attempts < count * 30) {
        ++attempts;
        float x = (float)GetRandomValue(-(int)cfg::MAP_HALF + 4, (int)cfg::MAP_HALF - 4);
        float z = (float)GetRandomValue(-(int)cfg::MAP_HALF + 4, (int)cfg::MAP_HALF - 4);
        Vector3 spawn{ x, 0.0f, z };
        if (Vector3Distance(spawn, player.Position()) < 12.0f) continue;
        if (world.PointInsideAnyPillar(spawn, cfg::ENEMY_RADIUS + 0.2f)) continue;
        enemies.emplace_back(spawn);
        ++placed;
    }
}

void Game::HandleShooting(float dt) {
    fireCooldown = std::max(0.0f, fireCooldown - dt);

    if (player.Hp() <= 0.0f) return;
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;
    if (fireCooldown > 0.0f) return;

    fireCooldown = cfg::FIRE_COOLDOWN;

    Camera3D cam = player.BuildCamera();
    Vector3 origin = cam.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));

    float bestT = cfg::BULLET_RANGE;
    int bestEnemy = -1;
    Vector3 bestPoint = Vector3Add(origin, Vector3Scale(dir, cfg::BULLET_RANGE));

    HitInfo worldHit = world.Raycast(origin, dir, cfg::BULLET_RANGE);
    if (worldHit.hit && worldHit.distance < bestT) {
        bestT = worldHit.distance;
        bestPoint = worldHit.point;
    }

    for (int i = 0; i < (int)enemies.size(); ++i) {
        float t;
        Vector3 p;
        if (enemies[i].RaycastHit(origin, dir, bestT, t, p)) {
            if (t < bestT) {
                bestT = t;
                bestEnemy = i;
                bestPoint = p;
            }
        }
    }

    if (bestEnemy >= 0) {
        enemies[bestEnemy].TakeDamage(cfg::BULLET_DAMAGE, origin);
        hud.TriggerHitMarker();
    }

    Vector3 muzzle = Vector3Add(origin, Vector3Scale(dir, 0.6f));
    tracers.push_back({ muzzle, bestPoint, cfg::TRACER_LIFETIME });

    player.AddRecoil(cfg::RECOIL_KICK);
}

void Game::Update(float dt) {
    if (IsKeyPressed(KEY_F1)) showHelp = !showHelp;
    if (IsKeyPressed(KEY_R) && player.Hp() <= 0.0f) {
        player = Player();
        SpawnEnemies(8);
    }

    player.Update(dt, world);

    for (Enemy& e : enemies) e.Update(dt, world, player);

    HandleShooting(dt);

    for (Tracer& t : tracers) t.life -= dt;
    tracers.erase(std::remove_if(tracers.begin(), tracers.end(),
                                 [](const Tracer& t) { return t.life <= 0.0f; }),
                  tracers.end());

    hud.Update(dt);

    int aliveEnemies = 0;
    for (const Enemy& e : enemies) if (e.IsAlive()) aliveEnemies++;
    if (aliveEnemies == 0) SpawnEnemies((int)enemies.size() + 2);
}

void Game::DrawTracers() {
    for (const Tracer& t : tracers) {
        float a = t.life / cfg::TRACER_LIFETIME;
        DrawLine3D(t.a, t.b, Fade(BLACK, a));
    }
}

void Game::Draw3DWorld(const Camera3D& cam) {
    BeginMode3D(cam);
        world.Draw();
        for (const Enemy& e : enemies) e.Draw();
        DrawTracers();
    EndMode3D();
}

void Game::DrawHelpText() {
    const int x = cfg::SCREEN_W - 290;
    int y = 12;
    Color c = { 30, 30, 30, 220 };
    DrawText("WASD     move",      x, y, 18, c); y += 20;
    DrawText("MOUSE    look",      x, y, 18, c); y += 20;
    DrawText("SHIFT    sprint",    x, y, 18, c); y += 20;
    DrawText("SPACE    jump",      x, y, 18, c); y += 20;
    DrawText("LCTRL    dash",      x, y, 18, c); y += 20;
    DrawText("C        slide",     x, y, 18, c); y += 20;
    DrawText("LMB      shoot",     x, y, 18, c); y += 20;
    DrawText("F1       toggle",    x, y, 18, c);
}

void Game::DrawDeathScreen() {
    DrawRectangle(0, 0, cfg::SCREEN_W, cfg::SCREEN_H, Fade(WHITE, 0.6f));
    const char* msg = "YOU DIED";
    int w = MeasureText(msg, 64);
    DrawText(msg, cfg::SCREEN_W / 2 - w / 2, cfg::SCREEN_H / 2 - 50, 64, RED);
    const char* sub = "press R to restart";
    int sw = MeasureText(sub, 24);
    DrawText(sub, cfg::SCREEN_W / 2 - sw / 2, cfg::SCREEN_H / 2 + 30, 24, BLACK);
}

void Game::Draw2DOverlay() {
    hud.DrawGunOverlay(player.RecoilKick());
    hud.DrawCrosshair();
    hud.DrawHitMarker();
    hud.Draw(Hud::MakeView(player), GetFPS());
    if (showHelp) DrawHelpText();
    if (player.Hp() <= 0.0f) DrawDeathScreen();
}

void Game::Draw() {
    Camera3D cam = player.BuildCamera();
    BeginDrawing();
        ClearBackground(WHITE);
        Draw3DWorld(cam);
        Draw2DOverlay();
    EndDrawing();
}

void Game::Tick() {
    float dt = GetFrameTime();
    Update(dt);
    Draw();
}
