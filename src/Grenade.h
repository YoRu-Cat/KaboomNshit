#pragma once
#include <raylib.h>
#include "Types.h"

class World;

// Thrown grenade: parabolic flight, bounces off pillars and ground, fuse timer.
// Once the fuse expires (or it stops moving on the ground at the end), the
// owning Game spawns an Explosion at its position and removes it.
class Grenade : public IEntity {
public:
    Grenade(Vector3 spawn, Vector3 throwVelocity);

    // IEntity
    void    Update(float dt) override;                       // unused
    void    Draw() const override;
    bool    IsAlive()  const override { return !exploded; }
    Vector3 Position() const override { return position; }

    // Real per-tick update with world collisions.
    void Step(float dt, const World& world);

    bool ShouldExplode() const { return fuseTimer <= 0.0f; }
    void MarkExploded()        { exploded = true; }

private:
    Vector3 position;
    Vector3 velocity;
    float   fuseTimer;
    float   spinPhase;
    bool    exploded;
};
