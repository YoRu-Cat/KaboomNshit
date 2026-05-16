#include "World.h"
#include "Config.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <limits>

namespace {
    constexpr float SPAWN_SAFE_RADIUS = 10.0f;

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

    float RandRange(float lo, float hi) {
        float t = (float)GetRandomValue(0, 10000) / 10000.0f;
        return lo + (hi - lo) * t;
    }
}

World::World()
    : mapHalf(cfg::MAP_HALF)
    , mapName("(uninitialized)")
    , lastSeed(0)
    , floorTex(nullptr)
    , structureTex(nullptr)
    , skyArray(nullptr)
    , skyCount(0)
    , currentSky(nullptr)
    , modelsBuilt(false)
{
    groundModel = { 0 };
    pillarUnitModel = { 0 };
}

World::~World() {
    if (modelsBuilt) {
        UnloadModel(groundModel);
        UnloadModel(pillarUnitModel);
        modelsBuilt = false;
    }
}

void World::SetTextures(Texture2D* floor, Texture2D* structure,
                       Texture2D* skies, int count) {
    floorTex     = floor;
    structureTex = structure;
    skyArray     = skies;
    skyCount     = count;

    if (modelsBuilt) {
        UnloadModel(groundModel);
        UnloadModel(pillarUnitModel);
        modelsBuilt = false;
    }

    // Tiled ground plane: scale UV coords up so the texture repeats.
    Mesh groundMesh = GenMeshPlane(mapHalf * 2.0f, mapHalf * 2.0f, 1, 1);
    for (int i = 0; i < groundMesh.vertexCount; ++i) {
        groundMesh.texcoords[i * 2 + 0] *= mapHalf * 0.25f;
        groundMesh.texcoords[i * 2 + 1] *= mapHalf * 0.25f;
    }
    UpdateMeshBuffer(groundMesh, 1, groundMesh.texcoords,
                     groundMesh.vertexCount * 2 * sizeof(float), 0);
    groundModel = LoadModelFromMesh(groundMesh);

    pillarUnitModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    if (floorTex)     SetMaterialTexture(&groundModel.materials[0],
                                        MATERIAL_MAP_DIFFUSE, *floorTex);
    if (structureTex) SetMaterialTexture(&pillarUnitModel.materials[0],
                                        MATERIAL_MAP_DIFFUSE, *structureTex);

    modelsBuilt = true;

    // Pick an initial sky in case Generate isn't called yet.
    if (skyArray && skyCount > 0) currentSky = &skyArray[0];
}

bool World::TooCloseToSpawn(float cx, float cz, float w, float d) const {
    // Treat the pillar's bounding XZ as a circle for a coarse exclusion check.
    float r = 0.5f * sqrtf(w * w + d * d);
    return (cx * cx + cz * cz) < (SPAWN_SAFE_RADIUS + r) * (SPAWN_SAFE_RADIUS + r);
}

void World::AddPillar(float cx, float cz, float w, float h, float d, float baseY) {
    AABB box{};
    box.min = { cx - w * 0.5f, baseY,         cz - d * 0.5f };
    box.max = { cx + w * 0.5f, baseY + h,     cz + d * 0.5f };
    pillars.push_back(box);
}

void World::AddOuterWalls() {
    AABB n{}; n.min = { -mapHalf - 1, 0, -mapHalf - 1 }; n.max = { mapHalf + 1, 8, -mapHalf };
    AABB s{}; s.min = { -mapHalf - 1, 0,  mapHalf };     s.max = { mapHalf + 1, 8, mapHalf + 1 };
    AABB w{}; w.min = { -mapHalf - 1, 0, -mapHalf };     w.max = { -mapHalf,     8, mapHalf };
    AABB e{}; e.min = {  mapHalf,     0, -mapHalf };     e.max = { mapHalf + 1,  8, mapHalf };
    pillars.push_back(n);
    pillars.push_back(s);
    pillars.push_back(w);
    pillars.push_back(e);
}

void World::Generate(int seed) {
    Theme t = (Theme)(((unsigned)seed) % (unsigned)THEME_COUNT);
    Generate(seed, t);
}

void World::Generate(int seed, Theme theme) {
    pillars.clear();
    lastSeed = seed;
    SetRandomSeed((unsigned)seed);

    if (skyArray && skyCount > 0) {
        int idx = GetRandomValue(0, skyCount - 1);
        currentSky = &skyArray[idx];
    }

    switch (theme) {
        case THEME_TOWER_CITY:  GenTowerCity(seed);  break;
        case THEME_ZIGGURAT:    GenZiggurat(seed);   break;
        case THEME_SKYBRIDGE:   GenSkybridge(seed);  break;
        case THEME_SPIRAL_MAZE: GenSpiralMaze(seed); break;
        case THEME_CATHEDRAL:   GenCathedral(seed);  break;
        default:                GenTowerCity(seed);  break;
    }

    // Every map gets a staircase, a slide ramp, and distant silhouettes.
    int stairs = GetRandomValue(1, 2);
    for (int i = 0; i < stairs; ++i) {
        float ang0 = RandRange(0.0f, 6.28f);
        float r0   = RandRange(15.0f, 30.0f);
        float x0   = cosf(ang0) * r0;
        float z0   = sinf(ang0) * r0;
        float ang1 = ang0 + RandRange(-0.4f, 0.4f);
        float r1   = r0 + RandRange(6.0f, 12.0f);
        float x1   = cosf(ang1) * r1;
        float z1   = sinf(ang1) * r1;
        int   steps = GetRandomValue(8, 14);
        AddStaircase(x0, z0, x1, z1, steps, cfg::STAIRCASE_STEP_H, RandRange(3.0f, 4.5f));
    }

    int slides = GetRandomValue(1, 2);
    for (int i = 0; i < slides; ++i) {
        float ang0 = RandRange(0.0f, 6.28f);
        float r0   = RandRange(28.0f, 45.0f);
        float x0   = cosf(ang0) * r0;
        float z0   = sinf(ang0) * r0;
        float ang1 = ang0 + RandRange(2.5f, 3.8f); // roughly opposite
        float r1   = RandRange(12.0f, 22.0f);
        float x1   = cosf(ang1) * r1;
        float z1   = sinf(ang1) * r1;
        AddSlideRamp(x0, z0, x1, z1, RandRange(6.0f, 10.0f), RandRange(2.5f, 4.0f));
    }

    AddHorrorSilhouettes(GetRandomValue(6, 10));
    AddOuterWalls();
}

void World::AddStaircase(float x0, float z0, float x1, float z1,
                         int stepCount, float stepHeight, float width) {
    if (stepCount < 2) stepCount = 2;
    float dx = (x1 - x0) / (float)stepCount;
    float dz = (z1 - z0) / (float)stepCount;
    float tread = sqrtf(dx * dx + dz * dz);
    if (tread < 0.4f) tread = 0.4f;

    for (int i = 0; i < stepCount; ++i) {
        float cx = x0 + dx * (i + 0.5f);
        float cz = z0 + dz * (i + 0.5f);
        float h  = stepHeight * (i + 1);
        // Each step is a solid block from ground to its top. Stacking
        // simplifies player landing logic since steps are still AABBs.
        AABB b{};
        // Orient block roughly along the step axis: pick the dominant axis
        // to set the long dimension so the staircase looks aligned.
        float w, d;
        if (fabsf(dx) > fabsf(dz)) { w = tread + 0.05f; d = width; }
        else                       { w = width;        d = tread + 0.05f; }
        b.min = { cx - w * 0.5f, 0.0f, cz - d * 0.5f };
        b.max = { cx + w * 0.5f, h,    cz + d * 0.5f };
        pillars.push_back(b);
    }
}

void World::AddSlideRamp(float x0, float z0, float x1, float z1,
                         float startHeight, float width) {
    // Build a long, very shallow staircase descending from startHeight to 0.
    int steps = (int)(startHeight / cfg::SLIDE_STEP_H);
    if (steps < 6) steps = 6;
    float dx = (x1 - x0) / (float)steps;
    float dz = (z1 - z0) / (float)steps;
    float tread = sqrtf(dx * dx + dz * dz);
    if (tread < 0.6f) tread = 0.6f;

    for (int i = 0; i < steps; ++i) {
        float cx = x0 + dx * (i + 0.5f);
        float cz = z0 + dz * (i + 0.5f);
        float top = startHeight - cfg::SLIDE_STEP_H * i;
        if (top < 0.05f) break;
        AABB b{};
        float w, d;
        if (fabsf(dx) > fabsf(dz)) { w = tread + 0.05f; d = width; }
        else                       { w = width;        d = tread + 0.05f; }
        b.min = { cx - w * 0.5f, 0.0f, cz - d * 0.5f };
        b.max = { cx + w * 0.5f, top,  cz + d * 0.5f };
        pillars.push_back(b);
    }
}

void World::AddPillarRing(float cx, float cz, float radius, int count,
                          float pillarW, float pillarH) {
    int gap = GetRandomValue(0, count - 1);
    for (int i = 0; i < count; ++i) {
        if (i == gap) continue;
        float ang = (float)i / (float)count * 2.0f * PI;
        float x = cx + cosf(ang) * radius;
        float z = cz + sinf(ang) * radius;
        if (TooCloseToSpawn(x, z, pillarW, pillarW)) continue;
        AddPillar(x, z, pillarW, pillarH, pillarW);
    }
}

void World::AddArchway(float cx, float cz, float gap, float height, float thickness, bool axisX) {
    // Two side pillars + a top beam. axisX = true means arch opens along X
    // (player can run through in X direction; beam is along Z).
    float legW = thickness;
    float legH = height - thickness;
    float halfGap = gap * 0.5f;
    if (axisX) {
        AddPillar(cx, cz - halfGap - legW * 0.5f, legW, legH, legW);
        AddPillar(cx, cz + halfGap + legW * 0.5f, legW, legH, legW);
        AABB beam{};
        beam.min = { cx - thickness * 0.5f, legH,           cz - halfGap - legW };
        beam.max = { cx + thickness * 0.5f, legH + thickness, cz + halfGap + legW };
        pillars.push_back(beam);
    } else {
        AddPillar(cx - halfGap - legW * 0.5f, cz, legW, legH, legW);
        AddPillar(cx + halfGap + legW * 0.5f, cz, legW, legH, legW);
        AABB beam{};
        beam.min = { cx - halfGap - legW, legH,           cz - thickness * 0.5f };
        beam.max = { cx + halfGap + legW, legH + thickness, cz + thickness * 0.5f };
        pillars.push_back(beam);
    }
}

void World::AddCurveChain(float x0, float z0, float x1, float z1,
                          float amplitude, int count, float pillarW, float pillarH) {
    Vector3 a{ x0, 0, z0 };
    Vector3 b{ x1, 0, z1 };
    Vector3 dir = Vector3Subtract(b, a);
    float len = Vector3Length(dir);
    if (len < 0.01f) return;
    Vector3 perp = { -dir.z / len, 0.0f, dir.x / len };
    for (int i = 0; i < count; ++i) {
        float t = (i + 0.5f) / (float)count;
        float bend = sinf(t * PI) * amplitude;
        Vector3 p = Vector3Add(a, Vector3Scale(dir, t));
        p = Vector3Add(p, Vector3Scale(perp, bend));
        if (fabsf(p.x) > mapHalf - 3 || fabsf(p.z) > mapHalf - 3) continue;
        if (TooCloseToSpawn(p.x, p.z, pillarW, pillarW)) continue;
        AddPillar(p.x, p.z, pillarW, pillarH, pillarW);
    }
}

void World::AddHorrorSilhouettes(int count) {
    // Very tall, very thin towers near the map edge — they show up on the
    // horizon as ominous standing figures.
    for (int i = 0; i < count; ++i) {
        float ang = RandRange(0.0f, 6.28f);
        float r   = mapHalf - RandRange(2.5f, 5.0f);
        float x   = cosf(ang) * r;
        float z   = sinf(ang) * r;
        float w   = RandRange(0.6f, 1.4f);
        float h   = RandRange(28.0f, 48.0f);
        if (fabsf(x) > mapHalf - 1.5f || fabsf(z) > mapHalf - 1.5f) continue;
        AddPillar(x, z, w, h, w);
    }
}

// ----------------------------------------------------------------------------
// THEME 1: Tower City — clusters of tall thin spires of varying heights.
// ----------------------------------------------------------------------------
void World::GenTowerCity(int seed) {
    mapName = "TOWER CITY";
    SetRandomSeed((unsigned)seed);

    int clusters = GetRandomValue(10, 14);
    for (int c = 0; c < clusters; ++c) {
        float cx = RandRange(-mapHalf + 8, mapHalf - 8);
        float cz = RandRange(-mapHalf + 8, mapHalf - 8);
        int per = GetRandomValue(4, 8);
        for (int i = 0; i < per; ++i) {
            float dx = RandRange(-6.0f, 6.0f);
            float dz = RandRange(-6.0f, 6.0f);
            float x  = cx + dx;
            float z  = cz + dz;
            float w  = RandRange(0.9f, 2.4f);
            float d  = RandRange(0.9f, 2.4f);
            float h  = RandRange(9.0f, 28.0f);
            if (TooCloseToSpawn(x, z, w, d)) continue;
            if (fabsf(x) > mapHalf - 2 || fabsf(z) > mapHalf - 2) continue;
            AddPillar(x, z, w, h, d);
        }
    }

    // Sprinkle archways for parkour interest.
    int arches = GetRandomValue(3, 6);
    for (int i = 0; i < arches; ++i) {
        float ang = RandRange(0.0f, 6.28f);
        float r   = RandRange(18.0f, 35.0f);
        float x = cosf(ang) * r;
        float z = sinf(ang) * r;
        AddArchway(x, z, RandRange(3.0f, 4.5f), RandRange(5.0f, 8.0f), 0.8f,
                   GetRandomValue(0, 1) != 0);
    }

    // A couple of pillar rings — loops to run around.
    int rings = GetRandomValue(1, 3);
    for (int i = 0; i < rings; ++i) {
        float cx = RandRange(-mapHalf + 15, mapHalf - 15);
        float cz = RandRange(-mapHalf + 15, mapHalf - 15);
        AddPillarRing(cx, cz, RandRange(7.0f, 12.0f),
                      GetRandomValue(8, 14), RandRange(1.0f, 1.6f), RandRange(8.0f, 16.0f));
    }
}

// ----------------------------------------------------------------------------
// THEME 2: Ziggurat — central stepped pyramid + outer pillar ring.
// ----------------------------------------------------------------------------
void World::GenZiggurat(int seed) {
    mapName = "ZIGGURAT";
    SetRandomSeed((unsigned)seed);

    // Stepped pyramid offset away from spawn so player can run up to it.
    float cx = RandRange(-6.0f, 6.0f);
    float cz = RandRange(12.0f, 22.0f);
    int   levels = 6;
    float base = 18.0f;
    for (int i = 0; i < levels; ++i) {
        float w = base - i * 3.0f;
        float h = 2.5f;
        AddPillar(cx, cz, w, h, w, i * h);
    }

    // Outer ring of tall sentinel pillars.
    int ring = GetRandomValue(10, 14);
    for (int i = 0; i < ring; ++i) {
        float ang = (float)i / (float)ring * 2.0f * PI + RandRange(-0.15f, 0.15f);
        float r   = RandRange(mapHalf - 12, mapHalf - 6);
        float x   = cosf(ang) * r;
        float z   = sinf(ang) * r;
        float w   = RandRange(1.4f, 2.6f);
        float h   = RandRange(10.0f, 18.0f);
        if (TooCloseToSpawn(x, z, w, w)) continue;
        AddPillar(x, z, w, h, w);
    }

    // Scattered short cover.
    int cover = GetRandomValue(14, 20);
    for (int i = 0; i < cover; ++i) {
        float x = RandRange(-mapHalf + 6, mapHalf - 6);
        float z = RandRange(-mapHalf + 6, mapHalf - 6);
        float s = RandRange(1.5f, 3.0f);
        float h = RandRange(1.0f, 2.5f);
        if (TooCloseToSpawn(x, z, s, s)) continue;
        AddPillar(x, z, s, h, s);
    }

    // Outer concentric arches forming circular processions.
    for (int ring = 0; ring < 2; ++ring) {
        float r = 24.0f + ring * 16.0f;
        int n = (ring == 0) ? 6 : 9;
        for (int i = 0; i < n; ++i) {
            float ang = (float)i / (float)n * 2.0f * PI;
            float x = cosf(ang) * r;
            float z = sinf(ang) * r;
            if (fabsf(x) > mapHalf - 3 || fabsf(z) > mapHalf - 3) continue;
            AddArchway(x, z, 3.0f, 6.0f + ring * 1.5f, 0.9f, GetRandomValue(0, 1) != 0);
        }
    }
}

// ----------------------------------------------------------------------------
// THEME 3: Skybridge — tall pillars connected by floating walkways.
// Floating segments are AABBs with baseY > eye height so you can run under.
// ----------------------------------------------------------------------------
void World::GenSkybridge(int seed) {
    mapName = "SKYBRIDGE";
    SetRandomSeed((unsigned)seed);

    struct Tower { float x, z, h; };
    std::vector<Tower> towers;

    int n = GetRandomValue(12, 16);
    int attempts = 0;
    while ((int)towers.size() < n && attempts < n * 10) {
        ++attempts;
        float x = RandRange(-mapHalf + 8, mapHalf - 8);
        float z = RandRange(-mapHalf + 8, mapHalf - 8);
        float h = RandRange(14.0f, 28.0f);
        if (TooCloseToSpawn(x, z, 3.0f, 3.0f)) continue;
        towers.push_back({ x, z, h });
        AddPillar(x, z, 2.4f, h, 2.4f);
    }

    // Walkways: connect each tower to its two nearest neighbors with a thin
    // floating slab at half the lower tower's height.
    for (size_t i = 0; i < towers.size(); ++i) {
        // Find two nearest.
        int n1 = -1, n2 = -1;
        float d1 = 1e9f, d2 = 1e9f;
        for (size_t j = 0; j < towers.size(); ++j) {
            if (i == j) continue;
            float dx = towers[i].x - towers[j].x;
            float dz = towers[i].z - towers[j].z;
            float d  = dx * dx + dz * dz;
            if (d < d1) { d2 = d1; n2 = n1; d1 = d; n1 = (int)j; }
            else if (d < d2) { d2 = d; n2 = (int)j; }
        }
        int picks[2] = { n1, n2 };
        for (int p : picks) {
            if (p < 0 || p < (int)i) continue; // avoid duplicate bridges
            float mx = (towers[i].x + towers[p].x) * 0.5f;
            float mz = (towers[i].z + towers[p].z) * 0.5f;
            float dx = towers[p].x - towers[i].x;
            float dz = towers[p].z - towers[i].z;
            float len = sqrtf(dx * dx + dz * dz);
            if (len < 4.0f || len > 28.0f) continue;
            // Bridge is a long thin slab — axis-aligned approximation along
            // the dominant axis (we don't rotate; pick X-major or Z-major).
            float minH = (towers[i].h < towers[p].h ? towers[i].h : towers[p].h);
            float baseY = minH * 0.55f;
            if (fabsf(dx) > fabsf(dz)) {
                AddPillar(mx, mz, len - 3.0f, 0.6f, 1.6f, baseY);
            } else {
                AddPillar(mx, mz, 1.6f, 0.6f, len - 3.0f, baseY);
            }
        }
    }

    // A lot of floating pickup-style platforms.
    int floaters = GetRandomValue(10, 16);
    for (int i = 0; i < floaters; ++i) {
        float x = RandRange(-mapHalf + 6, mapHalf - 6);
        float z = RandRange(-mapHalf + 6, mapHalf - 6);
        float y = RandRange(5.0f, 14.0f);
        float s = RandRange(2.0f, 3.5f);
        if (TooCloseToSpawn(x, z, s, s)) continue;
        AddPillar(x, z, s, 0.5f, s, y);
    }

    // Ground-level cover among the tower bases.
    for (int i = 0; i < 12; ++i) {
        float x = RandRange(-mapHalf + 6, mapHalf - 6);
        float z = RandRange(-mapHalf + 6, mapHalf - 6);
        float s = RandRange(1.5f, 2.5f);
        if (TooCloseToSpawn(x, z, s, s)) continue;
        AddPillar(x, z, s, RandRange(1.0f, 2.0f), s);
    }
}

// ----------------------------------------------------------------------------
// THEME 4: Spiral Maze — pillars on an expanding spiral, increasing height.
// ----------------------------------------------------------------------------
void World::GenSpiralMaze(int seed) {
    mapName = "SPIRAL MAZE";
    SetRandomSeed((unsigned)seed);

    int   n      = GetRandomValue(50, 80);
    float a      = RandRange(0.0f, 6.28f);
    float dr     = RandRange(0.55f, 0.85f);
    float dtheta = RandRange(0.45f, 0.65f);
    float r0     = SPAWN_SAFE_RADIUS + 1.5f;

    for (int i = 0; i < n; ++i) {
        float r = r0 + dr * i;
        float ang = a + dtheta * i;
        float x = cosf(ang) * r;
        float z = sinf(ang) * r;
        if (fabsf(x) > mapHalf - 3 || fabsf(z) > mapHalf - 3) break;
        float h = 4.0f + i * 0.4f + RandRange(-1.0f, 1.0f);
        float w = RandRange(1.2f, 2.0f);
        AddPillar(x, z, w, h, w);
    }

    // Counter-spiral of short cover pillars for variety.
    for (int i = 0; i < n / 2; ++i) {
        float r   = r0 + 2.0f + dr * i * 1.3f;
        float ang = -a - dtheta * i * 1.15f;
        float x = cosf(ang) * r;
        float z = sinf(ang) * r;
        if (fabsf(x) > mapHalf - 3 || fabsf(z) > mapHalf - 3) break;
        if (TooCloseToSpawn(x, z, 1.5f, 1.5f)) continue;
        AddPillar(x, z, 1.4f, 2.0f, 1.4f);
    }

    // Two outer curve-chains crossing the map for "ribbon" silhouettes.
    AddCurveChain(-mapHalf + 8, -mapHalf + 8,  mapHalf - 8,  mapHalf - 8,
                  10.0f, 14, 1.6f, 12.0f);
    AddCurveChain(-mapHalf + 8,  mapHalf - 8,  mapHalf - 8, -mapHalf + 8,
                  10.0f, 14, 1.6f, 12.0f);
}

// ----------------------------------------------------------------------------
// THEME 5: Cathedral — two long parallel rows of pillars + cross-pieces.
// ----------------------------------------------------------------------------
void World::GenCathedral(int seed) {
    mapName = "CATHEDRAL";
    SetRandomSeed((unsigned)seed);

    float spacing = RandRange(5.0f, 7.0f);
    float aisle   = RandRange(8.0f, 12.0f);
    int   rows    = (int)((mapHalf * 1.6f) / spacing);

    for (int i = -rows / 2; i <= rows / 2; ++i) {
        float z = i * spacing;
        if (fabsf(z) > mapHalf - 4) continue;
        float h = RandRange(14.0f, 22.0f);
        float w = RandRange(1.6f, 2.4f);
        // Left and right colonnade.
        if (!TooCloseToSpawn(-aisle * 0.5f, z, w, w))
            AddPillar(-aisle * 0.5f, z, w, h, w);
        if (!TooCloseToSpawn( aisle * 0.5f, z, w, w))
            AddPillar( aisle * 0.5f, z, w, h, w);

        // Occasional cross-aisle slab high up.
        if (GetRandomValue(0, 100) < 35 && fabsf(z) > 6.0f) {
            AddPillar(0.0f, z, aisle, 0.7f, 1.2f, h * 0.75f);
        }
    }

    // Outer chapel pillars at corners.
    float corner = mapHalf - 8.0f;
    AddPillar( corner,  corner, 3.0f, 24.0f, 3.0f);
    AddPillar(-corner,  corner, 3.0f, 24.0f, 3.0f);
    AddPillar( corner, -corner, 3.0f, 24.0f, 3.0f);
    AddPillar(-corner, -corner, 3.0f, 24.0f, 3.0f);

    // Apse: ring of arches at the far end of the nave.
    for (int i = 0; i < 8; ++i) {
        float ang = (float)i / 8.0f * 2.0f * PI;
        float r   = 24.0f;
        float x   = cosf(ang) * r;
        float z   = sinf(ang) * r;
        if (TooCloseToSpawn(x, z, 4.0f, 4.0f)) continue;
        AddArchway(x, z, 3.0f, 7.0f, 0.9f, i % 2 == 0);
    }

    // A curved ribbon of pillars threading down the nave.
    AddCurveChain(0.0f, -mapHalf + 6, 0.0f, mapHalf - 6, 4.0f, 10, 1.4f, 4.0f);
}

// ----------------------------------------------------------------------------
// Collision / queries (unchanged from previous implementation).
// ----------------------------------------------------------------------------
bool World::PointInsideAnyPillar(Vector3 p, float margin) const {
    for (const AABB& b : pillars) {
        if (p.x > b.min.x - margin && p.x < b.max.x + margin &&
            p.z > b.min.z - margin && p.z < b.max.z + margin) return true;
    }
    return false;
}

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
    // Save the velocity in case step-up needs to keep it alive.
    Vector3 savedVel = velocityInOut;

    Vector3 horiz = ResolveHorizontalAxisAxis(from, desired, radius, pillars, &velocityInOut);
    Vector3 result = { horiz.x, desired.y, horiz.z };

    // -- Auto step-up ---------------------------------------------------------
    // If horizontal got blocked AND the obstacle's top is within STEP_UP_MAX of
    // the player's feet, we silently lift onto it. This is what makes staircases
    // and slide ramps feel natural to walk on instead of having to jump each step.
    bool blockedX = fabsf(horiz.x - desired.x) > 0.001f;
    bool blockedZ = fabsf(horiz.z - desired.z) > 0.001f;
    if (blockedX || blockedZ) {
        float feetY = from.y - cfg::EYE_HEIGHT;
        float bestTop = feetY;
        bool  found = false;
        for (const AABB& b : pillars) {
            if (b.max.y <= feetY + 0.001f) continue;
            if (b.max.y > feetY + cfg::STEP_UP_MAX) continue;
            if (desired.x > b.min.x - radius && desired.x < b.max.x + radius &&
                desired.z > b.min.z - radius && desired.z < b.max.z + radius) {
                if (b.max.y > bestTop) { bestTop = b.max.y; found = true; }
            }
        }
        if (found) {
            // Confirm nothing else blocks at the stepped-up height.
            Vector3 stepped = { desired.x, bestTop + cfg::EYE_HEIGHT, desired.z };
            bool clear = true;
            for (const AABB& b : pillars) {
                if (stepped.x > b.min.x - radius && stepped.x < b.max.x + radius &&
                    stepped.z > b.min.z - radius && stepped.z < b.max.z + radius) {
                    if (stepped.y - cfg::EYE_HEIGHT < b.max.y - 0.01f &&
                        stepped.y > b.min.y + 0.01f) {
                        clear = false; break;
                    }
                }
            }
            if (clear) {
                result = stepped;
                velocityInOut = savedVel;          // keep momentum
                if (velocityInOut.y < 0.0f) velocityInOut.y = 0.0f;
                return result;
            }
        }
    }
    // -------------------------------------------------------------------------

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

bool World::IsGroundedAt(Vector3 pos, float radius) const {
    float feetY = pos.y - cfg::EYE_HEIGHT;
    if (feetY <= 0.05f) return true;
    for (const AABB& b : pillars) {
        if (pos.x > b.min.x - radius && pos.x < b.max.x + radius &&
            pos.z > b.min.z - radius && pos.z < b.max.z + radius) {
            if (feetY <= b.max.y + 0.08f && feetY >= b.max.y - 0.08f) return true;
        }
    }
    return false;
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
    if (fminf(t5, t6) > bestT) { axis = 2; }

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
    if (modelsBuilt && floorTex) {
        DrawModel(groundModel, { 0, 0, 0 }, 1.0f, WHITE);
    } else {
        DrawPlane({ 0, 0, 0 }, { mapHalf * 2.0f, mapHalf * 2.0f }, WHITE);
        // Only show the abstract grid when we have no texture to fall back to.
        DrawGrid((int)(mapHalf * 2.0f), 1.0f);
    }

    for (const AABB& b : pillars) {
        Vector3 c = AABBCenter(b);
        Vector3 s = AABBSize(b);
        if (modelsBuilt && structureTex) {
            DrawModelEx(pillarUnitModel, c, { 0, 1, 0 }, 0.0f, s, WHITE);
            DrawCubeWiresV(c, s, BLACK);
        } else {
            DrawCubeV(c, s, WHITE);
            DrawCubeWiresV(c, s, BLACK);
        }
    }
}
