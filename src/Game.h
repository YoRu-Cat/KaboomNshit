#pragma once
#include <raylib.h>
#include <vector>
#include "Player.h"
#include "World.h"
#include "Enemy.h"
#include "Hud.h"

// Top-level orchestrator. Owns the Player, World, Enemies, HUD, and the
// short-lived effects (tracers). Responsible for the main Update/Draw split.
// To extend: add new system pointers (e.g. PickupSystem, ParticleSystem),
// update them in Update(), draw them in Draw3D() / Draw2D().
class Game {
public:
    Game();

    bool Initialize();
    void Shutdown();
    bool ShouldClose() const;

    void Tick();

private:
    struct Tracer {
        Vector3 a;
        Vector3 b;
        float   life;
    };

    void Update(float dt);
    void Draw();
    void Draw3DWorld(const Camera3D& cam);
    void Draw2DOverlay();

    void HandleShooting(float dt);
    void SpawnEnemies(int count);
    void DrawTracers();
    void DrawDeathScreen();
    void DrawHelpText();

    Player              player;
    World               world;
    std::vector<Enemy>  enemies;
    Hud                 hud;
    std::vector<Tracer> tracers;

    float fireCooldown;
    bool  paused;
    bool  showHelp;
};
