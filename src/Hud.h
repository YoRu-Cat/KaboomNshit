#pragma once
#include <raylib.h>

class Player;

// Read-only view of player state used by the HUD.
// Decouples HUD rendering from Player internals so we can swap renderers
// (e.g. retro CRT overlay, accessibility variant) without touching Player.
struct PlayerView {
    float hp;
    float hpMax;
    float stamina;
    float staminaMax;
    int   dashCharges;
    int   dashChargesMax;
    float dashRechargeFraction;
    float horizontalSpeed;
    float hitFlash;
    bool  isSliding;
    bool  isDashing;
};

class Hud {
public:
    Hud();

    void Draw(const PlayerView& pv, int fps);
    void DrawGunOverlay(float recoilKick);
    void DrawCrosshair();
    void DrawHitMarker();
    void TriggerHitMarker() { hitMarkerTimer = 0.18f; }
    void Update(float dt);

    static PlayerView MakeView(const Player& p);

private:
    float hitMarkerTimer;
};
