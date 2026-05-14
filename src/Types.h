#pragma once
#include <raylib.h>

// Lifecycle interface for anything updated & drawn each frame.
class IEntity {
public:
    virtual ~IEntity() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() const = 0;
    virtual bool IsAlive() const = 0;
    virtual Vector3 Position() const = 0;
};

// Combat interface for anything that can take damage.
class IDamageable {
public:
    virtual ~IDamageable() = default;
    virtual void  TakeDamage(float dmg, Vector3 from) = 0;
    virtual float Hp() const = 0;
};

// Axis-aligned bounding box used for collision and raycasts.
struct AABB {
    Vector3 min;
    Vector3 max;
};

// Raycast result. Hit==false means no intersection within range.
struct HitInfo {
    bool     hit      = false;
    float    distance = 0.0f;
    Vector3  point    = { 0, 0, 0 };
    Vector3  normal   = { 0, 0, 0 };
    int      pillarIndex = -1;
};
