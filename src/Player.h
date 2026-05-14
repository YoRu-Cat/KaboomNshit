#pragma once
#include <raylib.h>
#include "Types.h"

class World;

// Player input/state/physics — Quake-style with Ultrakill-flavored dash & slide.
// Movement is decoupled from collision: Player computes desired velocity,
// World resolves the swept move and reports the corrected position.
class Player : public IDamageable {
public:
    Player();

    void Update(float dt, const World& world);
    Camera3D BuildCamera() const;

    // IDamageable
    void  TakeDamage(float dmg, Vector3 from) override;
    float Hp() const override { return hp; }

    // Read-only view used by HUD and other systems.
    Vector3 Position()        const { return position; }
    Vector3 Velocity()        const { return velocity; }
    Vector3 Forward()         const;
    float   Yaw()             const { return yaw; }
    float   Pitch()           const { return pitch; }
    float   Stamina()         const { return stamina; }
    int     DashCharges()     const { return dashCharges; }
    float   DashRechargeFraction() const;
    bool    IsSliding()       const { return sliding; }
    bool    IsDashing()       const { return dashTimer > 0.0f; }
    bool    IsGrounded()      const { return grounded; }
    float   HitFlash()        const { return hitFlash; }
    float   RecoilKick()      const { return recoilKick; }
    float   HorizontalSpeed() const;

    void AddRecoil(float amount) { recoilKick += amount; }

private:
    void UpdateLook(float dt);
    void UpdateDash(float dt);
    void UpdateMovement(float dt);
    void ApplyFriction(float dt, float frictionCoeff);
    void Accelerate(Vector3 wishDir, float wishSpeed, float accel, float dt);

    Vector3 position;
    Vector3 velocity;
    float   yaw;
    float   pitch;
    float   hp;
    float   stamina;
    bool    grounded;
    bool    sliding;
    float   slideTimer;
    float   dashTimer;
    Vector3 dashDir;
    int     dashCharges;
    float   dashRechargeTimer;
    float   hitFlash;
    float   recoilKick;
};
