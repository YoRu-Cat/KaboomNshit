#include "Hud.h"
#include "Config.h"
#include "Player.h"
#include <algorithm>

namespace {
    const Color BOX_BG = { 230, 230, 230, 220 };
}

Hud::Hud() : hitMarkerTimer(0.0f) {}

void Hud::Update(float dt) {
    hitMarkerTimer = std::max(0.0f, hitMarkerTimer - dt);
}

PlayerView Hud::MakeView(const Player& p) {
    PlayerView v{};
    v.hp                   = p.Hp();
    v.hpMax                = cfg::MAX_HP;
    v.stamina              = p.Stamina();
    v.staminaMax           = cfg::MAX_STAMINA;
    v.dashCharges          = p.DashCharges();
    v.dashChargesMax       = cfg::DASH_MAX_CHARGES;
    v.dashRechargeFraction = p.DashRechargeFraction();
    v.horizontalSpeed      = p.HorizontalSpeed();
    v.hitFlash             = p.HitFlash();
    v.isSliding            = p.IsSliding();
    v.isDashing            = p.IsDashing();
    return v;
}

void Hud::Draw(const PlayerView& pv, int fps) {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    DrawText(TextFormat("%d FPS", fps), 16, 12, 22, GREEN);

    const int boxX = 16;
    const int boxW = 200;
    const int boxH = 28;
    int y = sh - 12 - boxH;

    DrawRectangle(boxX, y, boxW, boxH, BOX_BG);
    DrawText(TextFormat("STAMINA: %d", (int)pv.stamina), boxX + 8, y + 4, 22, RED);
    y -= boxH + 4;

    DrawRectangle(boxX, y, boxW, boxH, BOX_BG);
    DrawText(TextFormat("HP: %d", (int)pv.hp), boxX + 8, y + 4, 22, RED);
    y -= boxH + 4;

    int dashY = sh - 12 - boxH;
    int dashX = boxX + boxW + 12;
    int slotW = 26;
    int slotH = 18;
    for (int i = 0; i < pv.dashChargesMax; ++i) {
        Rectangle r{ (float)(dashX + i * (slotW + 4)), (float)(dashY + (boxH - slotH) / 2), (float)slotW, (float)slotH };
        if (i < pv.dashCharges) {
            DrawRectangleRec(r, RED);
        } else {
            float frac = (i == pv.dashCharges) ? pv.dashRechargeFraction : 0.0f;
            DrawRectangleLinesEx(r, 2.0f, BLACK);
            if (frac > 0.0f) {
                Rectangle fill{ r.x, r.y, r.width * frac, r.height };
                DrawRectangleRec(fill, Fade(RED, 0.5f));
            }
        }
    }

    DrawText(TextFormat("SPD %.1f", pv.horizontalSpeed), dashX, dashY - 24, 16, BLACK);

    if (pv.hitFlash > 0.0f) {
        Color flash = Fade(RED, pv.hitFlash * 0.35f);
        DrawRectangle(0, 0, sw, sh, flash);
    }

    if (pv.isSliding) DrawText("SLIDE", sw / 2 - 30, sh - 80, 20, RED);
    if (pv.isDashing) DrawText("DASH",  sw / 2 - 28, sh - 80, 20, RED);
}

void Hud::DrawCrosshair() {
    int cx = GetScreenWidth() / 2;
    int cy = GetScreenHeight() / 2;
    DrawRectangle(cx - 1, cy - 6, 2, 4, BLACK);
    DrawRectangle(cx - 1, cy + 2, 2, 4, BLACK);
    DrawRectangle(cx - 6, cy - 1, 4, 2, BLACK);
    DrawRectangle(cx + 2, cy - 1, 4, 2, BLACK);
}

void Hud::DrawHitMarker() {
    if (hitMarkerTimer <= 0.0f) return;
    int cx = GetScreenWidth() / 2;
    int cy = GetScreenHeight() / 2;
    Color c = Fade(RED, hitMarkerTimer / 0.18f);
    DrawLine(cx - 10, cy - 10, cx - 4, cy - 4, c);
    DrawLine(cx + 10, cy - 10, cx + 4, cy - 4, c);
    DrawLine(cx - 10, cy + 10, cx - 4, cy + 4, c);
    DrawLine(cx + 10, cy + 10, cx + 4, cy + 4, c);
}

void Hud::DrawGunOverlay(float recoilKick) {
    const float cx = (float)GetScreenWidth();
    const float cy = (float)GetScreenHeight();
    const float kick = recoilKick * 80.0f;

    Vector2 muzzle  = { cx * 0.62f, cy * 0.78f + kick };
    Vector2 grip    = { cx * 0.74f, cy * 1.00f + kick };
    Vector2 barrelT = { cx * 0.58f, cy * 0.82f + kick };
    Vector2 barrelB = { cx * 0.66f, cy * 0.86f + kick };
    Vector2 backT   = { cx * 0.78f, cy * 0.96f + kick };
    Vector2 backB   = { cx * 0.82f, cy * 1.00f + kick };

    DrawTriangle(barrelT, barrelB, muzzle, WHITE);
    DrawTriangle(barrelB, backB, grip, WHITE);
    DrawTriangle(barrelT, barrelB, backB, WHITE);
    DrawTriangle(barrelT, backB, backT, WHITE);

    DrawLineEx(muzzle,  barrelT, 2.0f, BLACK);
    DrawLineEx(barrelT, backT,   2.0f, BLACK);
    DrawLineEx(backT,   backB,   2.0f, BLACK);
    DrawLineEx(backB,   grip,    2.0f, BLACK);
    DrawLineEx(grip,    barrelB, 2.0f, BLACK);
    DrawLineEx(barrelB, muzzle,  2.0f, BLACK);
    DrawLineEx(barrelB, backB,   2.0f, BLACK);

    Vector2 handA = { cx * 0.50f, cy * 0.92f + kick };
    Vector2 handB = { cx * 0.56f, cy * 0.88f + kick };
    Vector2 handC = { cx * 0.62f, cy * 1.00f + kick };
    DrawTriangle(handA, handC, handB, WHITE);
    DrawLineEx(handA, handB, 2.0f, BLACK);
    DrawLineEx(handB, handC, 2.0f, BLACK);
    DrawLineEx(handC, handA, 2.0f, BLACK);

    DrawRectangle((int)(cx * 0.63f), (int)(cy * 0.85f + kick), 40, 18, RED);
}
