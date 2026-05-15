#pragma once
#include <raylib.h>
#include "Types.h"

class World;
class Enemy;

// Visible projectile fired by the player. Travels at constant velocity,
// collides with pillars and enemies, despawns on contact or after a lifetime.
// Implements IEntity so it can later be stored in a generic entity container.
class Bullet : public IEntity {
public:
    Bullet(Vector3 spawn, Vector3 direction, float speed, float damage,
           bool explosive = false, Color color = BLACK);

    // IEntity
    void    Update(float dt) override;
    void    Draw() const override;
    bool    IsAlive()  const override { return alive; }
    Vector3 Position() const override { return position; }

    // Per-frame world+enemy interaction (game-side, since IEntity has no deps).
    // Returns the enemy index that was hit, or -1 if no enemy was hit this tick.
    int     Step(float dt, const World& world, Enemy* enemies, int enemyCount);

    float   Damage()      const { return damage; }
    bool    IsExplosive() const { return explosive; }
    void    Kill() { alive = false; }

private:
    Vector3 position;
    Vector3 prevPosition;
    Vector3 velocity;
    float   lifeTimer;
    float   damage;
    bool    alive;
    bool    explosive;
    Color   tint;
};
