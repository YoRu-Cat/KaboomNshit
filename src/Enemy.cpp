#include "Enemy.h"
#include "Config.h"
#include "World.h"
#include "Player.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

Enemy::Enemy(Vector3 spawn)
    : position(spawn)
    , velocity{ 0, 0, 0 }
    , knockback{ 0, 0, 0 }
    , hp(cfg::ENEMY_HP)
    , attackCooldown(0.0f)
    , hitFlash(0.0f)
    , deathTimer(0.0f)
{
    position.y = 0.0f;
}

void Enemy::Update(float) {}

void Enemy::Update(float dt, const World& world, Player& player) {
    hitFlash = std::max(0.0f, hitFlash - dt * 3.0f);

    if (!IsAlive()) {
        if (deathTimer > 0.0f) deathTimer = std::max(0.0f, deathTimer - dt);
        return;
    }

    Vector3 toPlayer = Vector3Subtract(player.Position(), position);
    toPlayer.y = 0.0f;
    float dist = Vector3Length(toPlayer);
    Vector3 dir = (dist > 0.001f) ? Vector3Scale(toPlayer, 1.0f / dist) : Vector3{ 0, 0, 0 };

    Vector3 chase = Vector3Scale(dir, cfg::ENEMY_SPEED);

    knockback.x *= (1.0f - 6.0f * dt);
    knockback.z *= (1.0f - 6.0f * dt);
    if (fabsf(knockback.x) < 0.05f) knockback.x = 0.0f;
    if (fabsf(knockback.z) < 0.05f) knockback.z = 0.0f;

    Vector3 step = {
        (chase.x + knockback.x) * dt,
        0.0f,
        (chase.z + knockback.z) * dt
    };
    Vector3 desired = Vector3Add(position, step);
    desired = world.ResolveEntityHorizontal(position, desired, cfg::ENEMY_RADIUS);
    position = desired;

    attackCooldown = std::max(0.0f, attackCooldown - dt);
    if (dist < cfg::ENEMY_ATTACK_RANGE && attackCooldown <= 0.0f) {
        player.TakeDamage(cfg::ENEMY_DAMAGE, position);
        attackCooldown = cfg::ENEMY_ATTACK_CD;
    }
}

void Enemy::TakeDamage(float dmg, Vector3 from) {
    if (!IsAlive()) return;
    hp = std::max(0.0f, hp - dmg);
    hitFlash = 1.0f;
    Vector3 push = Vector3Subtract(position, from);
    push.y = 0.0f;
    push = Vector3Normalize(push);
    knockback.x += push.x * cfg::ENEMY_KNOCKBACK;
    knockback.z += push.z * cfg::ENEMY_KNOCKBACK;
    if (hp <= 0.0f) deathTimer = 1.0f;
}

bool Enemy::RaycastHit(Vector3 origin, Vector3 dir, float maxDist, float& outT, Vector3& outPoint) const {
    if (!IsAlive()) return false;
    AABB body{};
    body.min = { position.x - cfg::ENEMY_RADIUS, position.y, position.z - cfg::ENEMY_RADIUS };
    body.max = { position.x + cfg::ENEMY_RADIUS, position.y + cfg::ENEMY_HEIGHT, position.z + cfg::ENEMY_RADIUS };

    float tx1 = (body.min.x - origin.x) / (dir.x != 0.0f ? dir.x : 1e-9f);
    float tx2 = (body.max.x - origin.x) / (dir.x != 0.0f ? dir.x : 1e-9f);
    float ty1 = (body.min.y - origin.y) / (dir.y != 0.0f ? dir.y : 1e-9f);
    float ty2 = (body.max.y - origin.y) / (dir.y != 0.0f ? dir.y : 1e-9f);
    float tz1 = (body.min.z - origin.z) / (dir.z != 0.0f ? dir.z : 1e-9f);
    float tz2 = (body.max.z - origin.z) / (dir.z != 0.0f ? dir.z : 1e-9f);

    float tmin = fmaxf(fmaxf(fminf(tx1, tx2), fminf(ty1, ty2)), fminf(tz1, tz2));
    float tmax = fminf(fminf(fmaxf(tx1, tx2), fmaxf(ty1, ty2)), fmaxf(tz1, tz2));
    if (tmax < 0.0f || tmin > tmax || tmin > maxDist) return false;

    outT = tmin > 0.0f ? tmin : tmax;
    outPoint = Vector3Add(origin, Vector3Scale(dir, outT));
    return true;
}

void Enemy::Draw() const {
    if (!IsAlive() && deathTimer <= 0.0f) return;

    float yScale = IsAlive() ? 1.0f : deathTimer;
    if (yScale <= 0.0f) return;

    Color fill = WHITE;
    Color line = BLACK;
    if (hitFlash > 0.0f) {
        fill = ColorAlphaBlend(WHITE, Fade(RED, hitFlash), WHITE);
        line = Fade(BLACK, 1.0f);
    }

    Vector3 bodyCenter = { position.x, (cfg::ENEMY_HEIGHT * 0.6f) * yScale * 0.5f, position.z };
    Vector3 bodySize   = { cfg::ENEMY_RADIUS * 1.6f, cfg::ENEMY_HEIGHT * 0.6f * yScale, cfg::ENEMY_RADIUS * 1.2f };
    DrawCubeV(bodyCenter, bodySize, fill);
    DrawCubeWiresV(bodyCenter, bodySize, line);

    Vector3 headCenter = { position.x, (cfg::ENEMY_HEIGHT * 0.6f + 0.3f) * yScale, position.z };
    Vector3 headSize   = { 0.45f, 0.45f * yScale, 0.45f };
    DrawCubeV(headCenter, headSize, fill);
    DrawCubeWiresV(headCenter, headSize, line);
}
