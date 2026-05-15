#pragma once
#include <raylib.h>
#include "Types.h"

class World;
class Player;

// Enemy with three behavior variants picked via Kind.
//   - CHASER : runs at the player, melee
//   - SHOOTER: keeps distance, fires red bullets toward the player
//   - TANK   : slow, hits hard, has lots of HP and a bigger silhouette
//
// Per-kind tuning lives in Enemy.cpp KIND_STATS table. Add a new archetype by
// extending the enum, adding a row, and (optionally) tweaking Update/Draw.
class Enemy : public IEntity, public IDamageable {
public:
    enum Kind { CHASER = 0, SHOOTER, TANK, KIND_COUNT };

    Enemy(Vector3 spawn, Kind kind = CHASER);

    void Update(float dt) override;
    void Update(float dt, const World& world, Player& player);
    void Draw() const override;

    // IDamageable
    void  TakeDamage(float dmg, Vector3 from) override;
    float Hp() const override { return hp; }

    // IEntity
    bool    IsAlive()  const override { return hp > 0.0f; }
    Vector3 Position() const override { return position; }

    bool RaycastHit(Vector3 origin, Vector3 dir, float maxDist, float& outT, Vector3& outPoint) const;

    // Shooters set this when they fire; Game consumes it once per fire and spawns the bullet.
    bool    ConsumePendingShot(Vector3& outOrigin, Vector3& outDir);
    Kind    GetKind()    const { return kind; }
    float   DeathFade()  const { return deathTimer; }

private:
    Kind    kind;
    Vector3 position;
    Vector3 velocity;
    Vector3 knockback;
    float   hp;
    float   maxHp;
    float   attackCooldown;
    float   shotCooldown;
    bool    shotPending;
    Vector3 shotOrigin;
    Vector3 shotDir;
    float   hitFlash;
    float   deathTimer;
};
