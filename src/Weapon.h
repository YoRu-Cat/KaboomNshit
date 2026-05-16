#pragma once
#include <raylib.h>

// A carried weapon. Geometry is built procedurally from raylib primitives.
// Owns its own runtime state (ammo, reload progress) so the rest of the game
// can treat the weapon as a black box: "try to fire", "start reload", "tick".
//
// Add a new weapon by:
//   1) adding an entry to the Type enum,
//   2) adding a row to DEFS in Weapon.cpp,
//   3) adding a case to DrawShape() and LocalMuzzlePos().
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

    // Per-frame state tick (handles reload timer).
    void Update(float dt);

    // Tries to consume a round. Returns true if the shot may happen.
    // Returns false if reloading, empty, or auto-reload was triggered.
    bool TryFire();

    // Begins a reload if it makes sense (not full, has reserve, not already reloading).
    void StartReload();

    // Tops up mag + reserve to max. Called on wave wipes / restart / map regen.
    void Refill();

    bool  IsReloading()    const { return reloadTimer > 0.0f; }
    float ReloadProgress() const;                  // 0..1 across the reload
    int   Mag()            const { return mag; }
    int   ReserveAmmo()    const { return reserve; }
    int   MagSize()        const { return magSize; }
    int   ReserveMax()     const { return reserveMax; }

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
    void    DrawShape(float recoilKick) const;
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

    // Ammo + reload runtime state:
    int   magSize;
    int   reserveMax;
    int   mag;
    int   reserve;
    float reloadDuration;
    float reloadTimer;     // counts DOWN from reloadDuration to 0 while reloading
};
