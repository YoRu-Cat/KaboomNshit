#include "Grenade.h"
#include "Config.h"
#include "World.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

Grenade::Grenade(Vector3 spawn, Vector3 throwVelocity)
    : position(spawn)
    , velocity(throwVelocity)
    , fuseTimer(cfg::GRENADE_FUSE)
    , spinPhase(0.0f)
    , exploded(false)
{}

void Grenade::Update(float) {}

void Grenade::Step(float dt, const World& world) {
    if (exploded) return;
    fuseTimer -= dt;
    spinPhase += dt * 12.0f;

    velocity.y -= cfg::GRENADE_GRAVITY * dt;

    Vector3 next = Vector3Add(position, Vector3Scale(velocity, dt));

    // Ground bounce.
    if (next.y < cfg::GRENADE_RADIUS) {
        next.y = cfg::GRENADE_RADIUS;
        if (velocity.y < 0.0f) velocity.y = -velocity.y * cfg::GRENADE_BOUNCE;
        velocity.x *= cfg::GRENADE_FRICTION;
        velocity.z *= cfg::GRENADE_FRICTION;
    }

    // Horizontal pillar collision: per-axis push out and reflect.
    Vector3 horizFrom{ position.x, next.y, position.z };
    Vector3 horizDes{  next.x,     next.y, next.z     };
    Vector3 resolved = world.ResolveEntityHorizontal(horizFrom, horizDes, cfg::GRENADE_RADIUS);
    if (fabsf(resolved.x - horizDes.x) > 0.001f) velocity.x = -velocity.x * cfg::GRENADE_BOUNCE;
    if (fabsf(resolved.z - horizDes.z) > 0.001f) velocity.z = -velocity.z * cfg::GRENADE_BOUNCE;
    next.x = resolved.x;
    next.z = resolved.z;

    position = next;
}

void Grenade::Draw() const {
    if (exploded) return;

    Vector3 size{ cfg::GRENADE_RADIUS * 1.6f, cfg::GRENADE_RADIUS * 1.6f, cfg::GRENADE_RADIUS * 1.6f };
    DrawCubeV(position, size, WHITE);
    DrawCubeWiresV(position, size, BLACK);

    // Spinning red band fakes a tumble cue.
    float t = sinf(spinPhase) * 0.5f + 0.5f;
    Color band{ 255, (unsigned char)(60 + 80 * t), (unsigned char)(60 + 80 * t), 255 };
    Vector3 stripe = { cfg::GRENADE_RADIUS * 1.7f, cfg::GRENADE_RADIUS * 0.4f, cfg::GRENADE_RADIUS * 1.7f };
    DrawCubeV(position, stripe, band);
    DrawCubeWiresV(position, stripe, BLACK);

    // Flashing pin/light when fuse is almost out.
    if (fuseTimer < 0.5f && (int)(fuseTimer * 12.0f) % 2 == 0) {
        Vector3 top = { position.x, position.y + cfg::GRENADE_RADIUS, position.z };
        DrawCubeV(top, { 0.08f, 0.08f, 0.08f }, RED);
    }
}
