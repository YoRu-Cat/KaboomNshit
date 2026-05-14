#include "Explosion.h"
#include <raymath.h>
#include <cmath>

Explosion::Explosion(Vector3 o)
    : origin(o)
    , age(0.0f)
    , damageDone(false)
{}

void Explosion::Update(float dt) {
    age += dt;
}

float Explosion::Progress() const {
    float p = age / cfg::EXPLOSION_DURATION;
    return p < 0.0f ? 0.0f : (p > 1.0f ? 1.0f : p);
}

void Explosion::Draw() const {
    float p = Progress();

    // Expanding wireframe shockwave sphere.
    float radius = cfg::EXPLOSION_RADIUS * p;
    int   rings  = 8;
    int   slices = 14;
    DrawSphereWires(origin, radius, rings, slices, BLACK);

    // Solid core that briefly grows then collapses.
    float coreP = (p < 0.35f) ? (p / 0.35f) : (1.0f - (p - 0.35f) / 0.65f);
    if (coreP > 0.0f) {
        float coreR = 1.4f * coreP;
        DrawSphere(origin, coreR, Fade(RED, 0.9f));
        DrawSphereWires(origin, coreR * 1.1f, 6, 10, BLACK);
    }

    // Inner second ring trails the outer for layered impact.
    float innerR = cfg::EXPLOSION_RADIUS * 0.65f * p;
    DrawSphereWires(origin, innerR, 6, 10, RED);

    // Vertical jet line — fast, makes the blast feel directional.
    if (p < 0.5f) {
        Vector3 jetTop = origin;
        jetTop.y += 4.0f * (1.0f - p * 2.0f) * (1.0f - p * 2.0f);
        DrawLine3D(origin, jetTop, BLACK);
    }
}
