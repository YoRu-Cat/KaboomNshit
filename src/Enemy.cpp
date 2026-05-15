#include "Enemy.h"
#include "Config.h"
#include "World.h"
#include "Player.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

namespace {
    struct KindStats {
        float hp;
        float speed;
        float damage;
        float attackRange;     // melee range (0 = ranged-only)
        float attackCooldown;
        float radius;
        float height;
        float scale;
        float preferredRange;  // for SHOOTER: stand-off distance
        float shotCooldown;    // for SHOOTER
        Color fill;
        Color wire;
    };

    const KindStats KINDS[Enemy::KIND_COUNT] = {
        // CHASER
        { 60.0f,  4.0f, 14.0f, 1.2f, 0.8f, 0.45f, 1.8f, 1.0f,
          0.0f,  0.0f,  WHITE, BLACK },
        // SHOOTER
        { 40.0f,  3.2f, 0.0f, 0.0f, 0.0f, 0.40f, 1.8f, 1.0f,
          10.0f, 1.1f,  { 220, 220, 220, 255 }, RED },
        // TANK
        { 220.0f, 2.0f, 26.0f, 1.6f, 1.1f, 0.85f, 2.2f, 1.55f,
          0.0f,  0.0f,  { 200, 200, 200, 255 }, BLACK },
    };
}

Enemy::Enemy(Vector3 spawn, Kind k)
    : kind(k)
    , position(spawn)
    , velocity{ 0, 0, 0 }
    , knockback{ 0, 0, 0 }
    , hp(KINDS[k].hp)
    , maxHp(KINDS[k].hp)
    , attackCooldown(0.0f)
    , shotCooldown(0.0f)
    , shotPending(false)
    , shotOrigin{ 0, 0, 0 }
    , shotDir{ 0, 0, 0 }
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

    const KindStats& s = KINDS[kind];

    Vector3 toPlayer = Vector3Subtract(player.Position(), position);
    toPlayer.y = 0.0f;
    float dist = Vector3Length(toPlayer);
    Vector3 dir = (dist > 0.001f) ? Vector3Scale(toPlayer, 1.0f / dist) : Vector3{ 0, 0, 0 };

    Vector3 wish{ 0, 0, 0 };
    if (kind == SHOOTER) {
        // Maintain preferred distance: advance if far, retreat if close.
        if (dist > s.preferredRange + 1.5f)      wish = Vector3Scale(dir, s.speed);
        else if (dist < s.preferredRange - 1.5f) wish = Vector3Scale(dir, -s.speed);
        else {
            // Strafe slightly around the player.
            Vector3 perp = { -dir.z, 0, dir.x };
            float side = (sinf(GetTime() * 1.5f + position.x * 0.3f) > 0) ? 1.0f : -1.0f;
            wish = Vector3Scale(perp, side * s.speed * 0.7f);
        }
    } else {
        wish = Vector3Scale(dir, s.speed);
    }

    knockback.x *= (1.0f - 6.0f * dt);
    knockback.z *= (1.0f - 6.0f * dt);
    if (fabsf(knockback.x) < 0.05f) knockback.x = 0.0f;
    if (fabsf(knockback.z) < 0.05f) knockback.z = 0.0f;

    Vector3 step = { (wish.x + knockback.x) * dt, 0.0f, (wish.z + knockback.z) * dt };
    Vector3 desired = Vector3Add(position, step);
    desired = world.ResolveEntityHorizontal(position, desired, s.radius);
    position = desired;

    attackCooldown = std::max(0.0f, attackCooldown - dt);
    if (s.attackRange > 0.0f && dist < s.attackRange && attackCooldown <= 0.0f) {
        player.TakeDamage(s.damage, position);
        attackCooldown = s.attackCooldown;
    }

    if (kind == SHOOTER) {
        shotCooldown = std::max(0.0f, shotCooldown - dt);
        if (shotCooldown <= 0.0f && dist < s.preferredRange + 6.0f && dist > 2.0f) {
            shotCooldown = s.shotCooldown;
            Vector3 muzzle = { position.x, position.y + s.height * 0.6f, position.z };
            Vector3 aimDir = Vector3Subtract(player.Position(), muzzle);
            float aimLen = Vector3Length(aimDir);
            if (aimLen > 0.001f) aimDir = Vector3Scale(aimDir, 1.0f / aimLen);
            shotOrigin  = muzzle;
            shotDir     = aimDir;
            shotPending = true;
        }
    }
}

bool Enemy::ConsumePendingShot(Vector3& outOrigin, Vector3& outDir) {
    if (!shotPending) return false;
    outOrigin = shotOrigin;
    outDir    = shotDir;
    shotPending = false;
    return true;
}

void Enemy::TakeDamage(float dmg, Vector3 from) {
    if (!IsAlive()) return;
    hp = std::max(0.0f, hp - dmg);
    hitFlash = 1.0f;
    Vector3 push = Vector3Subtract(position, from);
    push.y = 0.0f;
    if (Vector3Length(push) > 0.001f) push = Vector3Normalize(push);
    float kbScale = (kind == TANK) ? 0.4f : 1.0f;
    knockback.x += push.x * cfg::ENEMY_KNOCKBACK * kbScale;
    knockback.z += push.z * cfg::ENEMY_KNOCKBACK * kbScale;
    if (hp <= 0.0f) deathTimer = 1.0f;
}

bool Enemy::RaycastHit(Vector3 origin, Vector3 dir, float maxDist, float& outT, Vector3& outPoint) const {
    if (!IsAlive()) return false;
    const KindStats& s = KINDS[kind];
    AABB body{};
    body.min = { position.x - s.radius, position.y, position.z - s.radius };
    body.max = { position.x + s.radius, position.y + s.height * s.scale,
                 position.z + s.radius };

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
    const KindStats& s = KINDS[kind];
    float yScale = IsAlive() ? 1.0f : deathTimer;
    if (yScale <= 0.0f) return;

    Color fill = s.fill;
    Color wire = s.wire;
    if (hitFlash > 0.0f) {
        fill = ColorAlphaBlend(fill, Fade(RED, hitFlash), WHITE);
    }

    float bodyH = s.height * 0.6f * s.scale * yScale;
    float bodyW = s.radius * 1.6f * s.scale;
    float bodyD = s.radius * 1.2f * s.scale;

    Vector3 bodyCenter = { position.x, bodyH * 0.5f, position.z };
    Vector3 bodySize   = { bodyW, bodyH, bodyD };
    DrawCubeV(bodyCenter, bodySize, fill);
    DrawCubeWiresV(bodyCenter, bodySize, wire);

    float headSize = 0.45f * s.scale * yScale;
    Vector3 headCenter = { position.x, bodyH + headSize * 0.5f, position.z };
    Vector3 headSizeV  = { headSize, headSize, headSize };
    DrawCubeV(headCenter, headSizeV, fill);
    DrawCubeWiresV(headCenter, headSizeV, wire);

    // Kind-specific accents:
    if (kind == SHOOTER) {
        // Tiny red barrel on the chest — telegraphs "this one shoots".
        Vector3 b = { position.x, bodyH * 0.7f, position.z };
        DrawCubeV(b, { 0.18f, 0.18f, 0.5f * s.scale }, RED);
    } else if (kind == TANK) {
        // Shoulders / armor plates.
        Vector3 lpaul = { position.x - bodyW * 0.55f, bodyH * 0.85f, position.z };
        Vector3 rpaul = { position.x + bodyW * 0.55f, bodyH * 0.85f, position.z };
        Vector3 paulSize = { 0.35f, 0.35f, bodyD * 1.1f };
        DrawCubeV(lpaul, paulSize, fill);
        DrawCubeWiresV(lpaul, paulSize, wire);
        DrawCubeV(rpaul, paulSize, fill);
        DrawCubeWiresV(rpaul, paulSize, wire);
    }
}
