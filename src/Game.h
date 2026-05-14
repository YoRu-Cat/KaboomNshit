#pragma once
#include <raylib.h>
#include <vector>
#include "Player.h"
#include "World.h"
#include "Enemy.h"
#include "Hud.h"
#include "Bullet.h"
#include "Grenade.h"
#include "Explosion.h"
#include "ViewModel.h"

// Top-level orchestrator. Owns the Player, World, Enemies, Bullets, Grenades,
// Explosions, HUD, viewmodel, and short-lived effects. Responsible for the
// main Update/Draw split and for camera post-effects (shake, hitstop).
// Extend by adding new system pointers (pickups, particles_separate, AI sets)
// and calling them in Update()/Draw3DWorld()/Draw2DOverlay().
class Game {
public:
    Game();

    bool Initialize();
    void Shutdown();
    bool ShouldClose() const;

    void Tick();

private:
    struct Particle {
        Vector3 position;
        Vector3 velocity;
        float   life;
        float   maxLife;
        Color   color;
    };

    void Update(float dt);
    void Draw();

    void Draw3DWorld(const Camera3D& cam);
    void Draw2DOverlay();

    void HandleShooting(float dt);
    void HandleGrenade(float dt);
    void UpdateBullets(float dt);
    void UpdateGrenades(float dt);
    void UpdateExplosions(float dt);
    void UpdateParticles(float dt);

    void SpawnEnemies(int count);
    void SpawnHitParticles(Vector3 at, Vector3 normal, int count, Color color);
    void TriggerExplosion(Vector3 at);

    void DrawBullets();
    void DrawGrenades();
    void DrawExplosions();
    void DrawParticles();
    void DrawDeathScreen();
    void DrawHelpText();
    void DrawDamageVignette();

    Camera3D ApplyCameraShake(Camera3D cam);

    Player                 player;
    World                  world;
    std::vector<Enemy>     enemies;
    std::vector<Bullet>    bullets;
    std::vector<Grenade>   grenades;
    std::vector<Explosion> explosions;
    std::vector<Particle>  particles;
    Hud                    hud;
    ViewModel              viewModel;

    float fireCooldown;
    float grenadeCooldown;
    float cameraShake;     // current shake amplitude (m), decays each frame
    float hitstopTimer;    // when > 0, world updates run at HITSTOP_TIME_SCALE
    float swayPhase;       // accumulating phase for weapon sway
    bool  paused;
    bool  showHelp;
};
