#include "Weapon.h"
#include "Config.h"
#include <rlgl.h>
#include <cmath>

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
    };

    const WeaponDef DEFS[Weapon::TYPE_COUNT] = {
        // HANDGUN — snappy auto rifle
        { "HANDGUN", 0.09f,  28.0f,  75.0f, 1,  0.0f, false, 0.020f, true  },
        // SHOTGUN — 8 pellets, big spread
        { "SHOTGUN", 0.70f,  16.0f,  65.0f, 8,  9.0f, false, 0.080f, false },
        // SNIPER — single high-damage shot
        { "SNIPER",  1.20f, 140.0f, 200.0f, 1,  0.0f, false, 0.130f, false },
        // RPG-7 — explosive rocket
        { "RPG-7",   1.50f,   0.0f,  38.0f, 1,  0.0f, true,  0.150f, false },
    };

    // Helper for "filled white box with black wires" — the canvas style.
    void Part(Vector3 c, Vector3 s, Color fill, Color wire) {
        DrawCubeV(c, s, fill);
        DrawCubeWiresV(c, s, wire);
    }
    void Block(Vector3 c, Vector3 s) { Part(c, s, WHITE, BLACK); }
    void Dark (Vector3 c, Vector3 s) { Part(c, s, BLACK, BLACK); }
    void Red  (Vector3 c, Vector3 s) { Part(c, s, RED,   BLACK); }
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
{}

void Weapon::Configure(Type t) {
    type = t;
    const WeaponDef& d = DEFS[t];
    name         = d.name;
    fireCooldown = d.fireCooldown;
    damage       = d.damage;
    bulletSpeed  = d.bulletSpeed;
    pelletCount  = d.pellets;
    spreadRad    = d.spreadDeg * DEG2RAD;
    explosive    = d.explosive;
    recoil       = d.recoil;
    automatic    = d.automatic;
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

void Weapon::DrawShape() const {
    switch (type) {

    case HANDGUN: {
        // Compact pistol.
        Block({ 0.00f,  0.03f,  0.06f }, { 0.06f, 0.06f, 0.20f });   // slide
        Block({ 0.00f, -0.01f,  0.04f }, { 0.05f, 0.05f, 0.16f });   // frame
        Block({ 0.00f, -0.10f, -0.02f }, { 0.05f, 0.12f, 0.07f });   // grip
        Dark ({ 0.00f, -0.05f,  0.03f }, { 0.04f, 0.02f, 0.06f });   // trigger guard
        Dark ({ 0.00f,  0.08f,  0.14f }, { 0.015f,0.015f,0.015f });  // front sight
        Red  ({ 0.00f, -0.01f, -0.02f }, { 0.062f,0.025f,0.04f });   // accent
    } break;

    case SHOTGUN: {
        // Pump action. Long thick barrel + wood-y stock.
        Block({ 0.00f,  0.02f,  0.35f }, { 0.07f, 0.07f, 0.50f });   // barrel
        Block({ 0.00f, -0.03f,  0.25f }, { 0.09f, 0.06f, 0.16f });   // pump handle
        Block({ 0.00f,  0.00f,  0.05f }, { 0.08f, 0.09f, 0.22f });   // receiver
        Block({ 0.00f, -0.01f, -0.18f }, { 0.07f, 0.08f, 0.22f });   // stock
        Dark ({ 0.00f, -0.05f, -0.32f }, { 0.07f, 0.13f, 0.04f });   // butt
        Block({ 0.00f, -0.13f, -0.02f }, { 0.06f, 0.13f, 0.07f });   // grip
        Dark ({ 0.00f, -0.07f,  0.02f }, { 0.05f, 0.02f, 0.07f });   // trigger guard
        Red  ({ 0.00f,  0.07f,  0.08f }, { 0.075f,0.020f,0.10f });   // accent stripe
    } break;

    case SNIPER: {
        // Long thin barrel, scope on top.
        Block({ 0.00f,  0.02f,  0.50f }, { 0.045f, 0.045f, 0.80f }); // long barrel
        Dark ({ 0.00f,  0.02f,  0.90f }, { 0.06f,  0.06f,  0.04f }); // muzzle brake
        Block({ 0.00f,  0.00f, -0.02f }, { 0.07f,  0.09f,  0.28f }); // receiver
        Dark ({ 0.00f,  0.11f,  0.04f }, { 0.05f,  0.05f,  0.22f }); // scope tube
        Dark ({ 0.00f,  0.11f,  0.15f }, { 0.07f,  0.07f,  0.04f }); // scope front lens
        Dark ({ 0.00f,  0.11f, -0.08f }, { 0.07f,  0.07f,  0.04f }); // scope eyepiece
        Block({ 0.00f, -0.02f, -0.25f }, { 0.06f,  0.08f,  0.26f }); // stock
        Dark ({ 0.00f, -0.04f, -0.40f }, { 0.06f,  0.12f,  0.04f }); // butt
        Block({ 0.00f, -0.13f, -0.12f }, { 0.06f,  0.13f,  0.07f }); // grip
        Dark ({ 0.00f, -0.16f,  0.30f }, { 0.018f, 0.18f,  0.018f });// bipod left
        Red  ({ 0.00f,  0.05f, -0.06f }, { 0.072f, 0.020f, 0.10f }); // accent
    } break;

    case RPG: {
        // Tube launcher with warhead.
        Block({ 0.00f,  0.02f,  0.20f }, { 0.12f, 0.12f, 0.55f });   // main tube
        Block({ 0.00f,  0.02f,  0.55f }, { 0.16f, 0.16f, 0.16f });   // warhead body
        Dark ({ 0.00f,  0.02f,  0.66f }, { 0.10f, 0.10f, 0.06f });   // warhead tip
        Block({ 0.00f,  0.02f,  0.45f }, { 0.20f, 0.04f, 0.05f });   // warhead fin H
        Block({ 0.00f,  0.02f,  0.45f }, { 0.04f, 0.20f, 0.05f });   // warhead fin V
        Block({ 0.00f, -0.10f,  0.05f }, { 0.06f, 0.10f, 0.08f });   // grip
        Dark ({ 0.00f, -0.05f, -0.08f }, { 0.05f, 0.04f, 0.05f });   // trigger
        Dark ({ 0.00f,  0.13f,  0.10f }, { 0.04f, 0.06f, 0.18f });   // sight rail
        Block({ 0.00f,  0.02f, -0.18f }, { 0.10f, 0.10f, 0.10f });   // rear cap
        Red  ({ 0.00f,  0.02f,  0.42f }, { 0.165f, 0.025f, 0.05f }); // warhead band
        Red  ({ 0.00f, -0.04f,  0.10f }, { 0.12f,  0.020f, 0.20f }); // tube stripe
    } break;

    default: break;
    }
}

void Weapon::Draw(Vector3 anchor, float recoilKick, bool firing, float swayPhase) const {
    float bobX = sinf(swayPhase) * 0.012f;
    float bobY = cosf(swayPhase * 2.0f) * 0.008f;
    float pull = recoilKick * 1.4f;
    float roll = recoilKick * 0.6f * 57.2958f;

    Vector3 pos = {
        anchor.x + bobX,
        anchor.y + bobY,
        anchor.z - pull
    };

    rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(roll, 0.0f, 0.0f, 1.0f);
        DrawShape();
    rlPopMatrix();

    // Muzzle flash at the actual barrel tip of the active gun.
    if (firing) {
        Vector3 m = LocalMuzzlePos();
        Vector3 fc{ pos.x + m.x, pos.y + m.y, pos.z + m.z };
        DrawSphere(fc, 0.06f, YELLOW);
        DrawSphereWires(fc, 0.09f, 4, 6, BLACK);
        DrawCubeV(fc, { 0.05f, 0.05f, 0.05f }, RED);
    }
}
