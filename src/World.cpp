#include "World.h"
#include "Config.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <limits>

namespace {
    Vector3 AABBCenter(const AABB& b) {
        return {
            (b.min.x + b.max.x) * 0.5f,
            (b.min.y + b.max.y) * 0.5f,
            (b.min.z + b.max.z) * 0.5f,
        };
    }
    Vector3 AABBSize(const AABB& b) {
        return {
            b.max.x - b.min.x,
            b.max.y - b.min.y,
            b.max.z - b.min.z,
        };
    }
}

World::World() : mapHalf(cfg::MAP_HALF) {}

void World::Generate(int seed, int numPillars) {
    pillars.clear();
    SetRandomSeed(seed);

    for (int i = 0; i < numPillars; ++i) {
        float x = (float)GetRandomValue(-(int)mapHalf + 4, (int)mapHalf - 4);
        float z = (float)GetRandomValue(-(int)mapHalf + 4, (int)mapHalf - 4);
        float h = (float)GetRandomValue(4, 16);
        float w = (float)GetRandomValue(1, 3);

        if (fabsf(x) < 4.0f && fabsf(z) < 4.0f) { x += 6.0f; z += 6.0f; }

        AABB box{};
        box.min = { x - w * 0.5f, 0.0f,         z - w * 0.5f };
        box.max = { x + w * 0.5f, (float)h,     z + w * 0.5f };
        pillars.push_back(box);
    }

    AABB outerN{}; outerN.min = { -mapHalf - 1, 0, -mapHalf - 1 }; outerN.max = { mapHalf + 1, 8, -mapHalf };
    AABB outerS{}; outerS.min = { -mapHalf - 1, 0,  mapHalf };     outerS.max = { mapHalf + 1, 8, mapHalf + 1 };
    AABB outerW{}; outerW.min = { -mapHalf - 1, 0, -mapHalf };     outerW.max = { -mapHalf, 8, mapHalf };
    AABB outerE{}; outerE.min = {  mapHalf,     0, -mapHalf };     outerE.max = { mapHalf + 1, 8, mapHalf };
    pillars.push_back(outerN);
    pillars.push_back(outerS);
    pillars.push_back(outerW);
    pillars.push_back(outerE);
}

bool World::PointInsideAnyPillar(Vector3 p, float margin) const {
    for (const AABB& b : pillars) {
        if (p.x > b.min.x - margin && p.x < b.max.x + margin &&
            p.z > b.min.z - margin && p.z < b.max.z + margin) return true;
    }
    return false;
}

// Per-axis horizontal slide resolution against pillars.
// We expand each pillar by the player radius (Minkowski sum) and test the
// candidate point. If it would be inside an expanded box, we snap to the
// nearest face along the moving axis and zero the velocity on that axis.
static Vector3 ResolveHorizontalAxisAxis(Vector3 from, Vector3 desired, float radius,
                                         const std::vector<AABB>& pillars,
                                         Vector3* velocityInOut) {
    Vector3 result = from;

    result.x = desired.x;
    for (const AABB& b : pillars) {
        float minX = b.min.x - radius;
        float maxX = b.max.x + radius;
        if (result.x > minX && result.x < maxX &&
            result.z > b.min.z - radius && result.z < b.max.z + radius) {
            if (from.x <= minX) result.x = minX - 0.001f;
            else if (from.x >= maxX) result.x = maxX + 0.001f;
            if (velocityInOut) velocityInOut->x = 0.0f;
        }
    }

    result.z = desired.z;
    for (const AABB& b : pillars) {
        float minZ = b.min.z - radius;
        float maxZ = b.max.z + radius;
        if (result.z > minZ && result.z < maxZ &&
            result.x > b.min.x - radius && result.x < b.max.x + radius) {
            if (from.z <= minZ) result.z = minZ - 0.001f;
            else if (from.z >= maxZ) result.z = maxZ + 0.001f;
            if (velocityInOut) velocityInOut->z = 0.0f;
        }
    }

    return result;
}

Vector3 World::ResolvePlayerMove(Vector3 from, Vector3 desired, float radius, Vector3& velocityInOut) const {
    Vector3 horiz = ResolveHorizontalAxisAxis(from, desired, radius, pillars, &velocityInOut);

    Vector3 result = { horiz.x, desired.y, horiz.z };

    for (const AABB& b : pillars) {
        if (result.x > b.min.x - radius && result.x < b.max.x + radius &&
            result.z > b.min.z - radius && result.z < b.max.z + radius) {
            float feetY = result.y - cfg::EYE_HEIGHT;
            float headY = result.y;
            if (headY > b.min.y && feetY < b.max.y) {
                if (from.y - cfg::EYE_HEIGHT >= b.max.y - 0.01f) {
                    result.y = b.max.y + cfg::EYE_HEIGHT;
                    if (velocityInOut.y < 0.0f) velocityInOut.y = 0.0f;
                } else if (from.y <= b.min.y) {
                    result.y = b.min.y - 0.001f;
                    if (velocityInOut.y > 0.0f) velocityInOut.y = 0.0f;
                }
            }
        }
    }

    return result;
}

Vector3 World::ResolveEntityHorizontal(Vector3 from, Vector3 desired, float radius) const {
    Vector3 ignore{};
    return ResolveHorizontalAxisAxis(from, desired, radius, pillars, &ignore);
}

bool World::RayVsAABB(Vector3 origin, Vector3 dir, const AABB& box, float& tNear, Vector3& normal) const {
    float t1 = (box.min.x - origin.x) / (dir.x != 0.0f ? dir.x : 1e-9f);
    float t2 = (box.max.x - origin.x) / (dir.x != 0.0f ? dir.x : 1e-9f);
    float t3 = (box.min.y - origin.y) / (dir.y != 0.0f ? dir.y : 1e-9f);
    float t4 = (box.max.y - origin.y) / (dir.y != 0.0f ? dir.y : 1e-9f);
    float t5 = (box.min.z - origin.z) / (dir.z != 0.0f ? dir.z : 1e-9f);
    float t6 = (box.max.z - origin.z) / (dir.z != 0.0f ? dir.z : 1e-9f);

    float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));

    if (tmax < 0.0f || tmin > tmax) return false;

    tNear = tmin >= 0.0f ? tmin : tmax;

    int axis = 0;
    float bestT = fminf(t1, t2);
    if (fminf(t3, t4) > bestT) { bestT = fminf(t3, t4); axis = 1; }
    if (fminf(t5, t6) > bestT) { bestT = fminf(t5, t6); axis = 2; }

    normal = { 0, 0, 0 };
    if (axis == 0) normal.x = dir.x < 0 ? 1.0f : -1.0f;
    if (axis == 1) normal.y = dir.y < 0 ? 1.0f : -1.0f;
    if (axis == 2) normal.z = dir.z < 0 ? 1.0f : -1.0f;
    return true;
}

HitInfo World::Raycast(Vector3 origin, Vector3 dir, float maxDist) const {
    HitInfo best{};
    best.distance = maxDist;

    if (dir.y != 0.0f) {
        float tGround = (0.0f - origin.y) / dir.y;
        if (tGround > 0.0f && tGround < best.distance) {
            best.hit = true;
            best.distance = tGround;
            best.point = Vector3Add(origin, Vector3Scale(dir, tGround));
            best.normal = { 0, 1, 0 };
            best.pillarIndex = -2;
        }
    }

    for (int i = 0; i < (int)pillars.size(); ++i) {
        float t;
        Vector3 n;
        if (RayVsAABB(origin, dir, pillars[i], t, n)) {
            if (t > 0.0f && t < best.distance) {
                best.hit = true;
                best.distance = t;
                best.point = Vector3Add(origin, Vector3Scale(dir, t));
                best.normal = n;
                best.pillarIndex = i;
            }
        }
    }

    return best;
}

void World::Draw() const {
    DrawPlane({ 0, 0, 0 }, { mapHalf * 2.0f, mapHalf * 2.0f }, WHITE);
    DrawGrid((int)(mapHalf * 2.0f), 1.0f);

    for (const AABB& b : pillars) {
        Vector3 c = AABBCenter(b);
        Vector3 s = AABBSize(b);
        DrawCubeV(c, s, WHITE);
        DrawCubeWiresV(c, s, BLACK);
    }
}
