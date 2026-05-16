#pragma once
#include <raylib.h>
#include <vector>
#include "Types.h"

// Static level geometry: flat ground + a set of pillar AABBs.
// The Generate(seed) entry point picks one of several themes from the seed.
// Each theme is a self-contained procedural recipe that pushes AABBs into
// `pillars`. Always leaves a SPAWN_SAFE_RADIUS clear around the origin so the
// player never spawns inside geometry.
class World {
public:
    enum Theme {
        THEME_TOWER_CITY = 0,
        THEME_ZIGGURAT,
        THEME_SKYBRIDGE,
        THEME_SPIRAL_MAZE,
        THEME_CATHEDRAL,
        THEME_COUNT
    };

    World();
    ~World();

    // Bind to the texture library. Builds the textured ground & pillar models.
    // Pass the floor + structure textures and the array of skies + its length.
    void SetTextures(Texture2D* floor, Texture2D* structure,
                     Texture2D* skies, int skyCount);

    // Single entry point. Seed picks theme as `seed % THEME_COUNT`.
    void Generate(int seed);

    // Explicit theme variant (useful for debugging / theme-specific tuning).
    void Generate(int seed, Theme theme);

    // The 2D sky backdrop currently active (chosen at last Generate).
    Texture2D* CurrentSky() const { return currentSky; }

    Vector3 ResolvePlayerMove(Vector3 from, Vector3 desired, float radius, Vector3& velocityInOut) const;
    HitInfo Raycast(Vector3 origin, Vector3 dir, float maxDist) const;
    bool    PointInsideAnyPillar(Vector3 p, float margin = 0.0f) const;
    Vector3 ResolveEntityHorizontal(Vector3 from, Vector3 desired, float radius) const;
    bool    IsGroundedAt(Vector3 pos, float radius) const;
    void    Draw() const;

    const std::vector<AABB>& Pillars()   const { return pillars; }
    const char*              MapName()   const { return mapName; }
    int                      MapSeed()   const { return lastSeed; }

private:
    void AddPillar(float cx, float cz, float w, float h, float d, float baseY = 0.0f);
    void AddOuterWalls();
    bool TooCloseToSpawn(float cx, float cz, float w, float d) const;

    // Stack of stepped blocks along the line from (x0,z0) to (x1,z1).
    // Each block is `tread` deep along the line and `width` wide perpendicular.
    void AddStaircase(float x0, float z0, float x1, float z1,
                      int stepCount, float stepHeight, float width);

    // A long, very-shallow staircase — used as a "slide ramp" the player can
    // hit while sliding for momentum tricks. Goes from a high baseY down to 0.
    void AddSlideRamp(float x0, float z0, float x1, float z1,
                      float startHeight, float width);

    // Imposing far-edge silhouette towers used by every theme to give a sense
    // of being watched / surrounded.
    void AddHorrorSilhouettes(int count);

    // Ring of pillars around (cx, cz) with a single gap. Forms a "loop" the
    // player can run around.
    void AddPillarRing(float cx, float cz, float radius, int count,
                       float pillarW, float pillarH);

    // Two pillars + a horizontal beam between them — player can run/jump under.
    void AddArchway(float cx, float cz, float gap, float height, float thickness, bool axisX);

    // Curved chain of pillars along a sine arc between two points.
    void AddCurveChain(float x0, float z0, float x1, float z1,
                       float amplitude, int count, float pillarW, float pillarH);

    void GenTowerCity(int seed);
    void GenZiggurat(int seed);
    void GenSkybridge(int seed);
    void GenSpiralMaze(int seed);
    void GenCathedral(int seed);

    bool RayVsAABB(Vector3 origin, Vector3 dir, const AABB& box, float& tNear, Vector3& normal) const;

    std::vector<AABB> pillars;
    float             mapHalf;
    const char*       mapName;
    int               lastSeed;

    // Texture refs (owned by TextureLibrary in Game).
    Texture2D*        floorTex;
    Texture2D*        structureTex;
    Texture2D*        skyArray;
    int               skyCount;
    Texture2D*        currentSky;

    // Cached textured models. Built once in SetTextures, reused every draw.
    Model             groundModel;
    Model             pillarUnitModel;
    bool              modelsBuilt;
};
