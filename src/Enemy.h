#pragma once
#include <raylib.h>
#include "Types.h"

class World;
class Player;

// Simple chasing enemy. Single state machine: pursue -> attack-in-range -> dead.
// Designed to be expanded later (Strategy pattern: replace this entire class
// or compose behaviors behind an IBehavior interface).
class Enemy : public IEntity, public IDamageable {
public:
    Enemy(Vector3 spawn);

    void Update(float dt) override;                            // unused (no world/player)
    void Update(float dt, const World& world, Player& player); // real update
    void Draw() const override;

    // IDamageable
    void  TakeDamage(float dmg, Vector3 from) override;
    float Hp() const override { return hp; }

    // IEntity
    bool    IsAlive()  const override { return hp > 0.0f; }
    Vector3 Position() const override { return position; }

    // Capsule approximation: returns true if ray hits enemy and writes t/point.
    bool RaycastHit(Vector3 origin, Vector3 dir, float maxDist, float& outT, Vector3& outPoint) const;

    float DeathFade() const { return deathTimer; }

private:
    Vector3 position;
    Vector3 velocity;
    Vector3 knockback;
    float   hp;
    float   attackCooldown;
    float   hitFlash;
    float   deathTimer; // counts down from 1.0 -> 0.0 after death for fade-out
};
