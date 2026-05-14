#include "Bullet.h"
#include "Config.h"
#include "World.h"
#include "Enemy.h"
#include <raymath.h>
#include <cmath>

Bullet::Bullet(Vector3 spawn, Vector3 direction, float speed, float dmg)
    : position(spawn)
    , prevPosition(spawn)
    , velocity(Vector3Scale(direction, speed))
    , lifeTimer(cfg::BULLET_LIFETIME)
    , damage(dmg)
    , alive(true)
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

    Vector3 dir = Vector3Subtract(position, prevPosition);
    float len   = Vector3Length(dir);
    if (len < 1e-5f) { dir = { 0, 0, 1 }; }
    else             { dir = Vector3Scale(dir, 1.0f / len); }

    Vector3 tail = Vector3Subtract(position, Vector3Scale(dir, cfg::BULLET_TRAIL_LEN));

    DrawLine3D(tail, position, RED);
    DrawSphere(position, cfg::BULLET_RADIUS, RED);
    DrawSphereWires(position, cfg::BULLET_RADIUS * 1.4f, 4, 6, BLACK);
}
