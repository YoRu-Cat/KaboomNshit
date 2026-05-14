#pragma once
#include <raylib.h>
#include "Types.h"
#include "Config.h"

// Visual + gameplay blast. Lives for EXPLOSION_DURATION seconds. Damage is
// applied once on the first tick (so we don't multi-hit per frame); the rest
// of the lifetime is purely visual: expanding wireframe sphere + shrinking
// solid core + radial shock ring.
class Explosion : public IEntity {
public:
    Explosion(Vector3 origin);

    void    Update(float dt) override;
    void    Draw() const override;
    bool    IsAlive()  const override { return age < cfg::EXPLOSION_DURATION; }
    Vector3 Position() const override { return origin; }

    bool    DamageApplied() const { return damageDone; }
    void    MarkDamageApplied()   { damageDone = true; }

    float   Age()      const { return age; }
    float   Progress() const;

private:
    Vector3 origin;
    float   age;
    bool    damageDone;
};
