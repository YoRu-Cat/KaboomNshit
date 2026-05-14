#pragma once
#include <raylib.h>
#include <vector>
#include "Types.h"

// Static level geometry: a flat ground + a set of pillar AABBs.
// Provides:
//   - Movement collision (slide along walls, supports gravity)
//   - Hitscan raycasts (used by weapons)
//   - Drawing (wireframe + filled, matches the art style)
class World {
public:
    World();

    void Generate(int seed, int numPillars);

    // Slide-resolve a swept move from `from` to `desired`.
    // `radius` is the player's horizontal capsule radius.
    // On collision, the corresponding component of `velocityInOut` is zeroed
    // so accel/gravity don't keep pushing into walls.
    Vector3 ResolvePlayerMove(Vector3 from, Vector3 desired, float radius, Vector3& velocityInOut) const;

    // Cast a ray from `origin` along `dir` (assumed normalized) up to `maxDist`.
    HitInfo Raycast(Vector3 origin, Vector3 dir, float maxDist) const;

    // Test whether a point is inside any pillar (used for enemy spawn validation).
    bool PointInsideAnyPillar(Vector3 p, float margin = 0.0f) const;

    // Resolve a horizontal AABB push-out for an entity (used by enemies).
    Vector3 ResolveEntityHorizontal(Vector3 from, Vector3 desired, float radius) const;

    void Draw() const;

    const std::vector<AABB>& Pillars() const { return pillars; }

private:
    bool SweepAABB(Vector3 from, Vector3 desired, float radius, const AABB& box, Vector3& outResolved, int& hitAxis) const;
    bool RayVsAABB(Vector3 origin, Vector3 dir, const AABB& box, float& tNear, Vector3& normal) const;

    std::vector<AABB> pillars;
    float mapHalf;
};
