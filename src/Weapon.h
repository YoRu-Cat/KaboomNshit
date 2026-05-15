#pragma once
#include <raylib.h>

// A carried weapon. Geometry is built procedurally from raylib primitives —
// no asset loading, no scaling guesswork. Each weapon type has its own
// silhouette + stats. Add a new weapon by:
//   1) adding an entry to the Type enum,
//   2) adding a row to DEFS in Weapon.cpp,
//   3) adding a case to DrawShape().
class Weapon {
public:
    enum Type {
        HANDGUN = 0,
        SHOTGUN,
        SNIPER,
        RPG,
        TYPE_COUNT
    };

    Weapon();

    void Configure(Type t);

    // Drawn inside a viewmodel-camera BeginMode3D pass. `anchor` is the
    // viewmodel-space position to render at; `recoilKick` pulls the gun back.
    void Draw(Vector3 anchor, float recoilKick, bool firing, float swayPhase) const;

    Type        TypeId()        const { return type; }
    const char* Name()          const { return name; }
    float       FireCooldown()  const { return fireCooldown; }
    float       Damage()        const { return damage; }
    float       BulletSpeed()   const { return bulletSpeed; }
    int         PelletCount()   const { return pelletCount; }
    float       SpreadRadians() const { return spreadRad; }
    bool        IsExplosive()   const { return explosive; }
    float       Recoil()        const { return recoil; }
    bool        IsAutomatic()   const { return automatic; }

private:
    void DrawShape() const;
    Vector3 LocalMuzzlePos() const;

    Type        type;
    const char* name;

    // Gameplay stats:
    float fireCooldown;
    float damage;
    float bulletSpeed;
    int   pelletCount;
    float spreadRad;
    bool  explosive;
    float recoil;
    bool  automatic;
};
