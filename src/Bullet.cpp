#include "Bullet.h"
#include "Config.h"
#include "World.h"
#include "Enemy.h"
#include <raymath.h>
#include <cmath>

Bullet::Bullet(Vector3 spawn, Vector3 direction, float speed, float dmg, bool exp, Color c)
    : position(spawn)
    , prevPosition(spawn)
    , velocity(Vector3Scale(direction, speed))
    , lifeTimer(cfg::BULLET_LIFETIME)
    , damage(dmg)
    , alive(true)
    , explosive(exp)
    , tint(c)
{}

void Bullet::Update(float dt) {
    if (!alive) return;
    lifeTimer -= dt;
    if (lifeTimer <= 0.0f) { alive = false; return; }
    prevPosition = position;
    position = Vector3Add(position, Vector3Scale(velocity, dt));
}

int Bullet::Step(float dt, const World& world, Enemy* enemies, int enemyCount) {
    if (!alive) return -1;

    Vector3 startPos = position;
    Update(dt);
    if (!alive) return -1;

    Vector3 segment = Vector3Subtract(position, startPos);
    float   segLen  = Vector3Length(segment);
    if (segLen < 1e-5f) return -1;
    Vector3 dir = Vector3Scale(segment, 1.0f / segLen);

    int hitEnemy = -1;
    float bestT = segLen;

    HitInfo wh = world.Raycast(startPos, dir, segLen);
    if (wh.hit && wh.distance < bestT) {
        bestT = wh.distance;
        position = wh.point;
        alive = false;
    }

    for (int i = 0; i < enemyCount; ++i) {
        float t;
        Vector3 p;
        if (enemies[i].RaycastHit(startPos, dir, bestT, t, p)) {
            if (t < bestT) {
                bestT = t;
                position = p;
                hitEnemy = i;
                alive = false;
            }
        }
    }

    return hitEnemy;
}

void Bullet::Draw() const {
    if (!alive) return;

    Vector3 vdir = velocity;
    float vlen = Vector3Length(vdir);
    if (vlen < 1e-5f) return;
    vdir = Vector3Scale(vdir, 1.0f / vlen);

    Vector3 tail = Vector3Subtract(position, Vector3Scale(vdir, cfg::BULLET_TRAIL_LEN));

    if (explosive) {
        DrawLine3D(tail, position, RED);
        float r = cfg::BULLET_RADIUS * 3.0f;
        DrawCubeV(position, { r, r, r }, RED);
        DrawCubeWiresV(position, { r * 1.2f, r * 1.2f, r * 1.2f }, BLACK);
    } else {
        DrawLine3D(tail, position, tint);
        float r = cfg::BULLET_RADIUS * 2.0f;
        DrawCubeV(position, { r, r, r }, tint);
    }
}
