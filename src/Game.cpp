#include "Game.h"
#include "Config.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>

Game::Game()
    : fireCooldown(0.0f)
    , muzzleFlash(0.0f)
    , paused(false)
    , showHelp(true)
{}

bool Game::Initialize() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(cfg::SCREEN_W, cfg::SCREEN_H, "KaboomNshit");

    int monitor = GetCurrentMonitor();
    int mw = GetMonitorWidth(monitor);
    int mh = GetMonitorHeight(monitor);
    if (mw > 0 && mh > 0) {
        SetWindowSize(mw, mh);
    }
    ToggleFullscreen();

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
    muzzleFlash  = std::max(0.0f, muzzleFlash  - dt);

    if (player.Hp() <= 0.0f) return;
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;
    if (fireCooldown > 0.0f) return;

    fireCooldown = cfg::FIRE_COOLDOWN;
    muzzleFlash  = cfg::MUZZLE_FLASH_TIME;

    Camera3D cam = player.BuildCamera();
    Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 muzzle = Vector3Add(cam.position, Vector3Scale(dir, 0.45f));

    bullets.emplace_back(muzzle, dir, cfg::BULLET_SPEED, cfg::BULLET_DAMAGE);
    player.AddRecoil(cfg::RECOIL_KICK);
}

void Game::UpdateBullets(float dt) {
    for (Bullet& b : bullets) {
        if (!b.IsAlive()) continue;
        int hit = b.Step(dt, world, enemies.data(), (int)enemies.size());
        if (hit >= 0) {
            enemies[hit].TakeDamage(b.Damage(), b.Position());
            hud.TriggerHitMarker();
        }
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](const Bullet& b) { return !b.IsAlive(); }),
                  bullets.end());
}

void Game::Update(float dt) {
    if (IsKeyPressed(KEY_F1))           showHelp = !showHelp;
    if (IsKeyPressed(KEY_F11))          ToggleFullscreen();
    if (IsKeyPressed(KEY_R) && player.Hp() <= 0.0f) {
        player = Player();
        bullets.clear();
        SpawnEnemies(8);
    }

    player.Update(dt, world);
    for (Enemy& e : enemies) e.Update(dt, world, player);

    HandleShooting(dt);
    UpdateBullets(dt);
    hud.Update(dt);

    int aliveEnemies = 0;
    for (const Enemy& e : enemies) if (e.IsAlive()) aliveEnemies++;
    if (aliveEnemies == 0) SpawnEnemies((int)enemies.size() + 2);
}

void Game::DrawBullets() {
    for (const Bullet& b : bullets) b.Draw();
}

void Game::DrawMuzzleFlash(const Camera3D& cam) {
    if (muzzleFlash <= 0.0f) return;
    Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 mp  = Vector3Add(cam.position, Vector3Scale(dir, 0.5f));
    float r = 0.18f * (muzzleFlash / cfg::MUZZLE_FLASH_TIME);
    DrawSphere(mp, r, RED);
}

void Game::Draw3DWorld(const Camera3D& cam) {
    BeginMode3D(cam);
        world.Draw();
        for (const Enemy& e : enemies) e.Draw();
        DrawBullets();
        DrawMuzzleFlash(cam);
    EndMode3D();
}

void Game::DrawHelpText() {
    const int x = GetScreenWidth() - 290;
    int y = 12;
    Color c = { 30, 30, 30, 220 };
    DrawText("WASD     move",      x, y, 18, c); y += 20;
    DrawText("MOUSE    look",      x, y, 18, c); y += 20;
    DrawText("SHIFT    sprint",    x, y, 18, c); y += 20;
    DrawText("SPACE    jump",      x, y, 18, c); y += 20;
    DrawText("LCTRL    dash",      x, y, 18, c); y += 20;
    DrawText("C        slide",     x, y, 18, c); y += 20;
    DrawText("LMB      shoot",     x, y, 18, c); y += 20;
    DrawText("F1       toggle UI", x, y, 18, c); y += 20;
    DrawText("F11      fullscreen",x, y, 18, c);
}

void Game::DrawDeathScreen() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, Fade(WHITE, 0.6f));
    const char* msg = "YOU DIED";
    int w = MeasureText(msg, 64);
    DrawText(msg, sw / 2 - w / 2, sh / 2 - 50, 64, RED);
    const char* sub = "press R to restart";
    int subW = MeasureText(sub, 24);
    DrawText(sub, sw / 2 - subW / 2, sh / 2 + 30, 24, BLACK);
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
