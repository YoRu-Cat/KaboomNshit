#include "Game.h"
#include "Config.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <ctime>

namespace {
    float RandRange(float lo, float hi) {
        float t = (float)GetRandomValue(0, 10000) / 10000.0f;
        return lo + (hi - lo) * t;
    }
}

Game::Game()
    : fireCooldown(0.0f)
    , grenadeCooldown(0.0f)
    , cameraShake(0.0f)
    , hitstopTimer(0.0f)
    , swayPhase(0.0f)
    , paused(false)
    , showHelp(true)
{}

bool Game::Initialize() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(cfg::SCREEN_W, cfg::SCREEN_H, "KaboomNshit");

    int monitor = GetCurrentMonitor();
    int mw = GetMonitorWidth(monitor);
    int mh = GetMonitorHeight(monitor);
    if (mw > 0 && mh > 0) SetWindowSize(mw, mh);
    ToggleFullscreen();

    SetTargetFPS(144);
    DisableCursor();

    textures.Load();
    world.SetTextures(&textures.floor, &textures.structure, textures.skies, 3);
    Enemy::SetTextures(textures.enemies);

    weapons.ConfigureAll();

    world.Generate((int)time(nullptr));
    SpawnEnemies(8);
    return IsWindowReady();
}

void Game::Shutdown() {
    textures.Unload();
    if (IsWindowReady()) CloseWindow();
}

bool Game::ShouldClose() const {
    return WindowShouldClose();
}

void Game::SpawnEnemies(int count) {
    enemies.clear();
    int placed = 0;
    int attempts = 0;
    while (placed < count && attempts < count * 30) {
        ++attempts;
        float x = (float)GetRandomValue(-(int)cfg::MAP_HALF + 4, (int)cfg::MAP_HALF - 4);
        float z = (float)GetRandomValue(-(int)cfg::MAP_HALF + 4, (int)cfg::MAP_HALF - 4);
        Vector3 spawn{ x, 0.0f, z };
        if (Vector3Distance(spawn, player.Position()) < 14.0f) continue;
        if (world.PointInsideAnyPillar(spawn, cfg::ENEMY_RADIUS + 0.2f)) continue;

        // Mix: ~60% Chaser, ~30% Shooter, ~10% Tank.
        int roll = GetRandomValue(0, 99);
        Enemy::Kind k;
        if (roll < 60)      k = Enemy::CHASER;
        else if (roll < 90) k = Enemy::SHOOTER;
        else                k = Enemy::TANK;

        enemies.emplace_back(spawn, k);
        ++placed;
    }
}

void Game::HandleShooting(float dt) {
    fireCooldown = std::max(0.0f, fireCooldown - dt);
    if (player.Hp() <= 0.0f) return;
    if (fireCooldown > 0.0f) return;

    Weapon& w = weapons.Current();
    if (w.IsReloading()) return;

    bool pressed = w.IsAutomatic() ? IsMouseButtonDown(MOUSE_BUTTON_LEFT)
                                   : IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (!pressed) return;

    // Consume a round; bails out cleanly (and may auto-reload) if mag is empty.
    if (!w.TryFire()) return;

    fireCooldown = w.FireCooldown();

    Camera3D cam = player.BuildCamera();
    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 muzzle  = ViewModel::MuzzleWorldPosition(cam, player.RecoilKick());

    // Build a perpendicular basis for spread.
    Vector3 worldUp = { 0, 1, 0 };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, worldUp));
    Vector3 up    = Vector3CrossProduct(right, forward);

    int pellets = std::max(1, w.PelletCount());
    float spread = w.SpreadRadians();
    bool explosive = w.IsExplosive();
    float speed   = w.BulletSpeed();
    float damage  = w.Damage();

    for (int i = 0; i < pellets; ++i) {
        Vector3 dir = forward;
        if (pellets > 1 || spread > 0.0f) {
            float a1 = RandRange(-spread, spread);
            float a2 = RandRange(-spread, spread);
            dir = Vector3Add(forward, Vector3Add(Vector3Scale(right, sinf(a1)),
                                                 Vector3Scale(up,    sinf(a2))));
            dir = Vector3Normalize(dir);
        }
        bullets.emplace_back(muzzle, dir, speed, damage, explosive);
    }

    player.AddRecoil(w.Recoil());
    // Heavier guns shake more. Scale with recoil but never below the baseline.
    cameraShake = std::max(cameraShake,
                           std::max(cfg::SHAKE_ON_FIRE, w.Recoil() * 0.5f));
}

void Game::HandleGrenade(float dt) {
    grenadeCooldown = std::max(0.0f, grenadeCooldown - dt);
    if (player.Hp() <= 0.0f) return;
    if (grenadeCooldown > 0.0f) return;

    bool throwPressed = IsKeyPressed(KEY_G) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if (!throwPressed) return;

    grenadeCooldown = cfg::GRENADE_THROW_CD;

    Camera3D cam = player.BuildCamera();
    Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 spawn = ViewModel::MuzzleWorldPosition(cam, player.RecoilKick());

    Vector3 v = Vector3Scale(dir, cfg::GRENADE_THROW_SPEED);
    v.y += cfg::GRENADE_UP_BIAS;
    grenades.emplace_back(spawn, v);
}

void Game::UpdateBullets(float dt) {
    for (Bullet& b : bullets) {
        if (!b.IsAlive()) continue;
        int hit = b.Step(dt, world, enemies.data(), (int)enemies.size());

        // Explosive rounds detonate where they stopped (enemy hit OR wall hit).
        if (!b.IsAlive() && b.IsExplosive()) {
            TriggerExplosion(b.Position());
            continue;
        }

        if (hit >= 0) {
            bool wasAlive = enemies[hit].IsAlive();
            enemies[hit].TakeDamage(b.Damage(), b.Position());
            hud.TriggerHitMarker();
            cameraShake = std::max(cameraShake, cfg::SHAKE_ON_HIT);
            SpawnHitParticles(b.Position(), { 0, 1, 0 }, cfg::PARTICLES_PER_HIT, RED);
            if (wasAlive && !enemies[hit].IsAlive()) {
                hitstopTimer = cfg::HITSTOP_ON_KILL;
            }
        } else if (!b.IsAlive()) {
            SpawnHitParticles(b.Position(), { 0, 1, 0 }, 3, BLACK);
        }
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](const Bullet& b) { return !b.IsAlive(); }),
                  bullets.end());
}

void Game::CollectEnemyShots() {
    for (Enemy& e : enemies) {
        Vector3 origin, dir;
        if (e.ConsumePendingShot(origin, dir)) {
            // Enemy bullets are slower + weaker. They live in their own list.
            enemyBullets.emplace_back(origin, dir, 38.0f, 12.0f, false, RED);
        }
    }
}

void Game::UpdateEnemyBullets(float dt) {
    const Vector3 p = player.Position();
    for (Bullet& b : enemyBullets) {
        if (!b.IsAlive()) continue;

        // Advance + world-hit test (no enemy targets — pass empty array).
        b.Step(dt, world, nullptr, 0);
        if (!b.IsAlive()) continue;

        // Cylinder hit against the player capsule.
        Vector3 q = b.Position();
        float feetY = p.y - cfg::EYE_HEIGHT;
        float headY = p.y;
        if (q.y < feetY - 0.1f || q.y > headY + 0.3f) continue;
        float dx = q.x - p.x;
        float dz = q.z - p.z;
        if (dx * dx + dz * dz < cfg::PLAYER_RADIUS * cfg::PLAYER_RADIUS) {
            player.TakeDamage(b.Damage(), q);
            cameraShake = std::max(cameraShake, cfg::SHAKE_ON_HIT);
            b.Kill();
        }
    }
    enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(),
                                      [](const Bullet& b) { return !b.IsAlive(); }),
                       enemyBullets.end());
}

void Game::UpdateGrenades(float dt) {
    for (Grenade& g : grenades) {
        if (!g.IsAlive()) continue;
        g.Step(dt, world);
        if (g.ShouldExplode()) {
            TriggerExplosion(g.Position());
            g.MarkExploded();
        }
    }
    grenades.erase(std::remove_if(grenades.begin(), grenades.end(),
                                  [](const Grenade& g) { return !g.IsAlive(); }),
                   grenades.end());
}

void Game::TriggerExplosion(Vector3 at) {
    explosions.emplace_back(at);
    cameraShake = std::max(cameraShake, cfg::SHAKE_ON_EXPLOSION);
    SpawnHitParticles(at, { 0, 1, 0 }, cfg::PARTICLES_PER_BOOM, RED);
}

void Game::UpdateExplosions(float dt) {
    for (Explosion& e : explosions) {
        e.Update(dt);
        if (!e.DamageApplied()) {
            for (Enemy& en : enemies) {
                if (!en.IsAlive()) continue;
                float d = Vector3Distance(en.Position(), e.Position());
                if (d < cfg::EXPLOSION_RADIUS) {
                    float falloff = 1.0f - (d / cfg::EXPLOSION_RADIUS);
                    en.TakeDamage(cfg::EXPLOSION_DAMAGE * falloff, e.Position());
                }
            }
            float pd = Vector3Distance(player.Position(), e.Position());
            if (pd < cfg::EXPLOSION_RADIUS) {
                float falloff = 1.0f - (pd / cfg::EXPLOSION_RADIUS);
                player.TakeDamage(cfg::EXPLOSION_DAMAGE * falloff * cfg::SELF_DAMAGE_MULT, e.Position());
            }
            e.MarkDamageApplied();
        }
    }
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
                                    [](const Explosion& e) { return !e.IsAlive(); }),
                     explosions.end());
}

void Game::SpawnHitParticles(Vector3 at, Vector3 /*normal*/, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Particle p{};
        p.position = at;
        p.velocity = {
            RandRange(-4.0f, 4.0f),
            RandRange( 1.0f, 6.0f),
            RandRange(-4.0f, 4.0f),
        };
        p.maxLife = cfg::PARTICLE_LIFETIME;
        p.life    = p.maxLife;
        p.color   = color;
        particles.push_back(p);
    }
}

void Game::UpdateParticles(float dt) {
    for (Particle& p : particles) {
        p.life -= dt;
        p.velocity.y -= 12.0f * dt;
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.life <= 0.0f; }),
                    particles.end());
}

void Game::Update(float dt) {
    if (IsKeyPressed(KEY_F1))   showHelp = !showHelp;
    if (IsKeyPressed(KEY_F11))  ToggleFullscreen();
    if (IsKeyPressed(KEY_R)) {
        if (player.Hp() <= 0.0f) {
            player = Player();
            bullets.clear();
            enemyBullets.clear();
            grenades.clear();
            explosions.clear();
            particles.clear();
            weapons.RefillAll();
            SpawnEnemies(8);
        } else {
            weapons.Current().StartReload();
        }
    }

    // Re-roll the map with a fresh random seed.
    if (IsKeyPressed(KEY_M)) {
        world.Generate((int)time(nullptr) ^ GetRandomValue(1, 1 << 30));
        player = Player();
        bullets.clear();
        enemyBullets.clear();
        grenades.clear();
        explosions.clear();
        particles.clear();
        weapons.RefillAll();
        SpawnEnemies(8);
    }

    // Cycle to the next theme deterministically (useful for previewing each).
    if (IsKeyPressed(KEY_N)) {
        int next = (world.MapSeed() + 1);
        world.Generate(next);
        player = Player();
        bullets.clear();
        enemyBullets.clear();
        grenades.clear();
        explosions.clear();
        particles.clear();
        weapons.RefillAll();
        SpawnEnemies(8);
    }

    // Apply hitstop: scale world dt, but keep input/cooldowns/hud on real time.
    float worldDt = dt;
    if (hitstopTimer > 0.0f) {
        hitstopTimer = std::max(0.0f, hitstopTimer - dt);
        worldDt *= cfg::HITSTOP_TIME_SCALE;
    }

    player.Update(worldDt, world);
    for (Enemy& e : enemies) e.Update(worldDt, world, player);

    CollectEnemyShots();

    weapons.HandleInput();
    HandleShooting(dt);
    HandleGrenade(dt);
    UpdateBullets(worldDt);
    UpdateEnemyBullets(worldDt);
    UpdateGrenades(worldDt);
    UpdateExplosions(worldDt);
    UpdateParticles(worldDt);

    cameraShake = std::max(0.0f, cameraShake - cfg::CAMERA_SHAKE_DECAY * dt * cameraShake);
    swayPhase += dt * (1.5f + player.HorizontalSpeed() * 0.18f);
    hud.Update(dt);

    weapons.Update(dt);

    int aliveEnemies = 0;
    for (const Enemy& e : enemies) if (e.IsAlive()) aliveEnemies++;
    if (aliveEnemies == 0) {
        // Wave wipe: reward the player with a full restock before next wave.
        weapons.RefillAll();
        SpawnEnemies((int)enemies.size() + 2);
    }
}

Camera3D Game::ApplyCameraShake(Camera3D cam) const {
    if (cameraShake <= 0.0f) return cam;
    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right   = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up      = Vector3CrossProduct(right, forward);

    float dx = RandRange(-1.0f, 1.0f) * cameraShake;
    float dy = RandRange(-1.0f, 1.0f) * cameraShake;
    Vector3 off = Vector3Add(Vector3Scale(right, dx), Vector3Scale(up, dy));
    cam.position = Vector3Add(cam.position, off);
    cam.target   = Vector3Add(cam.target,   off);
    return cam;
}

void Game::DrawBullets() {
    for (const Bullet& b : bullets)      b.Draw();
    for (const Bullet& b : enemyBullets) b.Draw();
}
void Game::DrawGrenades()   { for (const Grenade& g : grenades)     g.Draw(); }
void Game::DrawExplosions() { for (const Explosion& e : explosions) e.Draw(); }

void Game::DrawParticles() {
    for (const Particle& p : particles) {
        float a = p.life / p.maxLife;
        Color c = p.color;
        c.a = (unsigned char)(255.0f * a);
        float s = 0.06f + 0.10f * (1.0f - a);
        DrawCubeV(p.position, { s, s, s }, c);
        DrawCubeWiresV(p.position, { s, s, s }, BLACK);
    }
}

void Game::Draw3DWorld(const Camera3D& cam) {
    BeginMode3D(cam);
        world.Draw();
        for (const Enemy& e : enemies) e.Draw(cam);
        DrawBullets();
        DrawGrenades();
        DrawExplosions();
        DrawParticles();
    EndMode3D();

    // Viewmodel renders in its own 3D pass with a fixed origin camera.
    // This guarantees it always lands in the same spot on screen.
    const Weapon& w = weapons.Current();
    bool firing = fireCooldown > w.FireCooldown() * 0.5f;
    viewModel.Draw(cam, w, player.RecoilKick(), firing, swayPhase);
}

void Game::DrawHelpText() {
    const int x = GetScreenWidth() - 300;
    int y = 12;
    Color c = { 30, 30, 30, 220 };
    DrawText("WASD     move",       x, y, 18, c); y += 20;
    DrawText("MOUSE    look",       x, y, 18, c); y += 20;
    DrawText("SHIFT    sprint",     x, y, 18, c); y += 20;
    DrawText("SPACE    jump",       x, y, 18, c); y += 20;
    DrawText("LCTRL    dash",       x, y, 18, c); y += 20;
    DrawText("C        slide",      x, y, 18, c); y += 20;
    DrawText("LMB      shoot",      x, y, 18, c); y += 20;
    DrawText("RMB / G  grenade",    x, y, 18, c); y += 20;
    DrawText("R        reload",     x, y, 18, c); y += 20;
    DrawText("F1       toggle UI",  x, y, 18, c); y += 20;
    DrawText("F11      fullscreen", x, y, 18, c); y += 20;
    DrawText("M        new map",    x, y, 18, c); y += 20;
    DrawText("N        next theme", x, y, 18, c); y += 20;
    DrawText("WHEEL    swap weapon",x, y, 18, c); y += 20;
    DrawText("1-4      pick weapon",x, y, 18, c);
}

void Game::DrawDamageVignette() {
    float f = player.HitFlash();
    if (f <= 0.0f) return;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0,  0,        sw, 40,        Fade(RED, f * 0.35f));
    DrawRectangle(0,  sh - 40,  sw, 40,        Fade(RED, f * 0.35f));
    DrawRectangle(0,  0,        40, sh,        Fade(RED, f * 0.35f));
    DrawRectangle(sw - 40, 0,   40, sh,        Fade(RED, f * 0.35f));
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
    hud.DrawCrosshair();
    hud.DrawHitMarker();
    hud.Draw(Hud::MakeView(player), GetFPS());
    DrawDamageVignette();

    // Map name banner — small, centered up top.
    const char* name = world.MapName();
    int nw = MeasureText(name, 22);
    int sw = GetScreenWidth();
    DrawRectangle(sw / 2 - nw / 2 - 12, 8, nw + 24, 30, { 230, 230, 230, 220 });
    DrawText(name, sw / 2 - nw / 2, 12, 22, BLACK);

    // Current weapon — name + mag/reserve, bottom-right.
    const Weapon& cur = weapons.Current();
    const char* wname = cur.Name();
    int sh = GetScreenHeight();

    // Ammo line, e.g. "12 / 84" or "RELOADING..."
    const char* ammoBuf = cur.IsReloading()
        ? "RELOADING..."
        : TextFormat("%d / %d", cur.Mag(), cur.ReserveAmmo());

    int nameW = MeasureText(wname, 24);
    int ammoW = MeasureText(ammoBuf, 22);
    int boxW  = (nameW > ammoW ? nameW : ammoW) + 24;
    int boxX  = sw - boxW - 16;
    int boxY  = sh - 68;
    DrawRectangle(boxX, boxY, boxW, 60, { 230, 230, 230, 220 });
    DrawText(wname,   boxX + 12, boxY + 4,  24, RED);
    DrawText(ammoBuf, boxX + 12, boxY + 34, 22, cur.IsReloading() ? BLACK : RED);

    // Reload progress bar across the bottom of the ammo box.
    if (cur.IsReloading()) {
        float p = cur.ReloadProgress();
        int barW = (int)((boxW - 16) * p);
        DrawRectangle(boxX + 8, boxY + 56, boxW - 16, 3, { 200, 200, 200, 255 });
        DrawRectangle(boxX + 8, boxY + 56, barW,      3, RED);
    }

    if (showHelp) DrawHelpText();
    if (player.Hp() <= 0.0f) DrawDeathScreen();
}

void Game::Draw() {
    Camera3D cam = ApplyCameraShake(player.BuildCamera());
    BeginDrawing();
        ClearBackground({ 240, 238, 236, 255 });

        // Sky backdrop — drawn as a 2D fullscreen blit before the 3D world.
        // World geometry overdraws below the horizon line, leaving the sky
        // visible above.
        Texture2D* sky = world.CurrentSky();
        if (sky && sky->id != 0) {
            Rectangle src{ 0, 0, (float)sky->width, (float)sky->height };
            Rectangle dst{ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
            DrawTexturePro(*sky, src, dst, { 0, 0 }, 0.0f, WHITE);
        }

        Draw3DWorld(cam);
        Draw2DOverlay();
    EndDrawing();
}

void Game::Tick() {
    float dt = GetFrameTime();
    Update(dt);
    Draw();
}
