#include "Weapon.h"
#include "Config.h"
#include <rlgl.h>
#include <cmath>
#include <algorithm>

namespace {
    struct WeaponDef {
        const char* name;
        float fireCooldown;
        float damage;
        float bulletSpeed;
        int   pellets;
        float spreadDeg;
        bool  explosive;
        float recoil;
        bool  automatic;
        int   magSize;
        int   reserveMax;
        float reloadDuration;
    };

    const WeaponDef DEFS[Weapon::TYPE_COUNT] = {
        // HANDGUN — snappy auto rifle, big mag, fast reload
        { "HANDGUN", 0.09f,  28.0f,  75.0f, 1,  0.0f, false, 0.030f, true,
          14, 84,  1.10f },
        // SHOTGUN — small mag, hard kick, slow reload
        { "SHOTGUN", 0.70f,  16.0f,  65.0f, 8,  9.0f, false, 0.130f, false,
          6, 30,  1.70f },
        // SNIPER — tiny mag, huge punch, very slow reload
        { "SNIPER",  1.10f, 140.0f, 200.0f, 1,  0.0f, false, 0.220f, false,
          5, 20,  2.10f },
        // RPG-7 — single shot magazine, explosive, slowest reload
        { "RPG-7",   1.50f,   0.0f,  38.0f, 1,  0.0f, true,  0.260f, false,
          1, 5,   2.40f },
    };

    void Part(Vector3 c, Vector3 s, Color fill, Color wire) {
        DrawCubeV(c, s, fill);
        DrawCubeWiresV(c, s, wire);
    }
    void Block(Vector3 c, Vector3 s) { Part(c, s, WHITE, BLACK); }
    void Dark (Vector3 c, Vector3 s) { Part(c, s, BLACK, BLACK); }
    void Red  (Vector3 c, Vector3 s) { Part(c, s, RED,   BLACK); }

    // Slide animation: clamp the slide displacement so it never wanders too far.
    float SlidePull(float recoilKick, float factor, float maxPull) {
        float v = recoilKick * factor;
        return v < maxPull ? v : maxPull;
    }
}

Weapon::Weapon()
    : type(HANDGUN)
    , name("(none)")
    , fireCooldown(0.1f)
    , damage(0)
    , bulletSpeed(0)
    , pelletCount(1)
    , spreadRad(0)
    , explosive(false)
    , recoil(0)
    , automatic(true)
    , magSize(0)
    , reserveMax(0)
    , mag(0)
    , reserve(0)
    , reloadDuration(1.0f)
    , reloadTimer(0.0f)
{}

void Weapon::Configure(Type t) {
    type = t;
    const WeaponDef& d = DEFS[t];
    name           = d.name;
    fireCooldown   = d.fireCooldown;
    damage         = d.damage;
    bulletSpeed    = d.bulletSpeed;
    pelletCount    = d.pellets;
    spreadRad      = d.spreadDeg * DEG2RAD;
    explosive      = d.explosive;
    recoil         = d.recoil;
    automatic      = d.automatic;
    magSize        = d.magSize;
    reserveMax     = d.reserveMax;
    reloadDuration = d.reloadDuration;
    Refill();
}

void Weapon::Refill() {
    mag         = magSize;
    reserve     = reserveMax;
    reloadTimer = 0.0f;
}

void Weapon::Update(float dt) {
    if (reloadTimer > 0.0f) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0f) {
            reloadTimer = 0.0f;
            // Move ammo from reserve into the magazine.
            int needed = magSize - mag;
            int taken  = (needed < reserve) ? needed : reserve;
            mag       += taken;
            reserve   -= taken;
        }
    }
}

bool Weapon::TryFire() {
    if (reloadTimer > 0.0f) return false;
    if (mag <= 0) {
        // Auto-reload if there's reserve, otherwise just fail.
        if (reserve > 0) StartReload();
        return false;
    }
    mag -= 1;
    return true;
}

void Weapon::StartReload() {
    if (reloadTimer > 0.0f) return;
    if (mag >= magSize) return;
    if (reserve <= 0)   return;
    reloadTimer = reloadDuration;
}

float Weapon::ReloadProgress() const {
    if (reloadDuration <= 0.0f) return 0.0f;
    if (reloadTimer <= 0.0f)    return 0.0f;
    return 1.0f - (reloadTimer / reloadDuration);
}

Vector3 Weapon::LocalMuzzlePos() const {
    switch (type) {
        case HANDGUN: return { 0.00f, 0.03f, 0.18f };
        case SHOTGUN: return { 0.00f, 0.02f, 0.58f };
        case SNIPER:  return { 0.00f, 0.02f, 0.88f };
        case RPG:     return { 0.00f, 0.02f, 0.62f };
        default:      return { 0.00f, 0.00f, 0.20f };
    }
}

void Weapon::DrawShape(float recoilKick) const {
    switch (type) {

    case HANDGUN: {
        float slide = SlidePull(recoilKick, 0.7f, 0.04f);
        Block({ 0.00f,  0.03f,  0.06f - slide }, { 0.06f, 0.06f, 0.20f });
        Block({ 0.00f, -0.01f,  0.04f },         { 0.05f, 0.05f, 0.16f });
        Block({ 0.00f, -0.10f, -0.02f },         { 0.05f, 0.12f, 0.07f });
        Dark ({ 0.00f, -0.05f,  0.03f },         { 0.04f, 0.02f, 0.06f });
        Dark ({ 0.00f,  0.08f,  0.14f - slide }, { 0.015f,0.015f,0.015f });
        Red  ({ 0.00f, -0.01f, -0.02f },         { 0.062f,0.025f,0.04f });
    } break;

    case SHOTGUN: {
        float pump = SlidePull(recoilKick, 0.6f, 0.06f);
        Block({ 0.00f,  0.02f,  0.35f },         { 0.07f, 0.07f, 0.50f });
        Block({ 0.00f, -0.03f,  0.25f - pump },  { 0.09f, 0.06f, 0.16f });
        Block({ 0.00f,  0.00f,  0.05f },         { 0.08f, 0.09f, 0.22f });
        Block({ 0.00f, -0.01f, -0.18f },         { 0.07f, 0.08f, 0.22f });
        Dark ({ 0.00f, -0.05f, -0.32f },         { 0.07f, 0.13f, 0.04f });
        Block({ 0.00f, -0.13f, -0.02f },         { 0.06f, 0.13f, 0.07f });
        Dark ({ 0.00f, -0.07f,  0.02f },         { 0.05f, 0.02f, 0.07f });
        Red  ({ 0.00f,  0.07f,  0.08f },         { 0.075f,0.020f,0.10f });
        // Ejecting shell during recoil — small red square pops up beside the receiver.
        if (recoilKick > 0.04f) {
            float h = (recoilKick - 0.04f) * 4.0f;
            Red({ -0.06f, 0.06f + h, 0.04f }, { 0.025f, 0.025f, 0.04f });
        }
    } break;

    case SNIPER: {
        float bolt = SlidePull(recoilKick, 0.5f, 0.05f);
        Block({ 0.00f,  0.02f,  0.50f },         { 0.045f, 0.045f, 0.80f });
        Dark ({ 0.00f,  0.02f,  0.90f },         { 0.06f,  0.06f,  0.04f });
        Block({ 0.00f,  0.00f, -0.02f },         { 0.07f,  0.09f,  0.28f });
        Dark ({ 0.00f,  0.11f,  0.04f - bolt },  { 0.05f,  0.05f,  0.22f });
        Dark ({ 0.00f,  0.11f,  0.15f - bolt },  { 0.07f,  0.07f,  0.04f });
        Dark ({ 0.00f,  0.11f, -0.08f - bolt },  { 0.07f,  0.07f,  0.04f });
        Block({ 0.00f, -0.02f, -0.25f },         { 0.06f,  0.08f,  0.26f });
        Dark ({ 0.00f, -0.04f, -0.40f },         { 0.06f,  0.12f,  0.04f });
        Block({ 0.00f, -0.13f, -0.12f },         { 0.06f,  0.13f,  0.07f });
        Dark ({ 0.00f, -0.16f,  0.30f },         { 0.018f, 0.18f,  0.018f });
        Red  ({ 0.00f,  0.05f, -0.06f },         { 0.072f, 0.020f, 0.10f });
    } break;

    case RPG: {
        Block({ 0.00f,  0.02f,  0.20f },         { 0.12f, 0.12f, 0.55f });
        Block({ 0.00f,  0.02f,  0.55f },         { 0.16f, 0.16f, 0.16f });
        Dark ({ 0.00f,  0.02f,  0.66f },         { 0.10f, 0.10f, 0.06f });
        Block({ 0.00f,  0.02f,  0.45f },         { 0.20f, 0.04f, 0.05f });
        Block({ 0.00f,  0.02f,  0.45f },         { 0.04f, 0.20f, 0.05f });
        Block({ 0.00f, -0.10f,  0.05f },         { 0.06f, 0.10f, 0.08f });
        Dark ({ 0.00f, -0.05f, -0.08f },         { 0.05f, 0.04f, 0.05f });
        Dark ({ 0.00f,  0.13f,  0.10f },         { 0.04f, 0.06f, 0.18f });
        Block({ 0.00f,  0.02f, -0.18f },         { 0.10f, 0.10f, 0.10f });
        Red  ({ 0.00f,  0.02f,  0.42f },         { 0.165f, 0.025f, 0.05f });
        Red  ({ 0.00f, -0.04f,  0.10f },         { 0.12f,  0.020f, 0.20f });
        // Backblast ember during recoil.
        if (recoilKick > 0.04f) {
            float s = std::min(0.18f, recoilKick * 0.6f);
            Red({ 0.00f, 0.02f, -0.30f }, { s, s, s });
        }
    } break;

    default: break;
    }
}

void Weapon::Draw(Vector3 anchor, float recoilKick, bool firing, float swayPhase) const {
    float bobX = sinf(swayPhase) * 0.012f;
    float bobY = cosf(swayPhase * 2.0f) * 0.008f;
    float pull = recoilKick * 1.4f;
    float roll = recoilKick * 0.6f * 57.2958f;

    // Reload dip — triangle wave peaking at progress 0.5.
    float rp  = ReloadProgress();
    float dip = (rp <= 0.0f) ? 0.0f : (rp < 0.5f ? rp * 2.0f : (1.0f - rp) * 2.0f);
    float drop      = dip * 0.10f;
    float tiltDeg   = dip * 45.0f;
    float pushAway  = dip * 0.05f;

    Vector3 pos = {
        anchor.x + bobX,
        anchor.y + bobY - drop,
        anchor.z - pull - pushAway
    };

    rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(tiltDeg, 1.0f, 0.0f, 0.0f);
        rlRotatef(roll,    0.0f, 0.0f, 1.0f);
        DrawShape(recoilKick);

        // Magazine animation: drop it visibly near mid-reload.
        if (dip > 0.4f) {
            float magDrop = (dip - 0.4f) * 0.6f;
            Red({ 0.00f, -0.18f - magDrop, 0.02f }, { 0.06f, 0.10f, 0.07f });
        }
    rlPopMatrix();

    if (firing && !IsReloading()) {
        Vector3 m = LocalMuzzlePos();
        Vector3 fc{ pos.x + m.x, pos.y + m.y, pos.z + m.z };
        DrawSphere(fc, 0.06f, YELLOW);
        DrawSphereWires(fc, 0.09f, 4, 6, BLACK);
        DrawCubeV(fc, { 0.05f, 0.05f, 0.05f }, RED);
    }
}
