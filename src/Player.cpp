#include "Player.h"
#include "Config.h"
#include "World.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

namespace {
    // Forward at yaw=0 is +Z. Right is cross(Forward, Up) so that pressing D
    // strafes in the direction the camera sees as "right" on screen.
    Vector3 YawForward(float yaw) { return { sinf(yaw), 0.0f, cosf(yaw) }; }
    Vector3 YawRight(float yaw)   { return { -cosf(yaw), 0.0f, sinf(yaw) }; }

    Vector3 HorizontalProjection(Vector3 v) { return { v.x, 0.0f, v.z }; }
}

Player::Player()
    : position{ 0.0f, cfg::EYE_HEIGHT, 8.0f }
    , velocity{ 0.0f, 0.0f, 0.0f }
    , yaw(0.0f)
    , pitch(0.0f)
    , hp(cfg::MAX_HP)
    , stamina(cfg::MAX_STAMINA)
    , grounded(true)
    , sliding(false)
    , slideTimer(0.0f)
    , dashTimer(0.0f)
    , dashDir{ 0, 0, 0 }
    , dashCharges(cfg::DASH_MAX_CHARGES)
    , dashRechargeTimer(0.0f)
    , hitFlash(0.0f)
    , recoilKick(0.0f)
    , coyoteTimer(0.0f)
    , jumpBuffer(0.0f)
{}

Vector3 Player::Forward() const {
    return {
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw)
    };
}

float Player::HorizontalSpeed() const {
    return sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
}

float Player::DashRechargeFraction() const {
    if (dashCharges >= cfg::DASH_MAX_CHARGES) return 1.0f;
    return 1.0f - (dashRechargeTimer / cfg::DASH_RECHARGE_TIME);
}

void Player::TakeDamage(float dmg, Vector3 from) {
    hp = std::max(0.0f, hp - dmg);
    hitFlash = 1.0f;
    Vector3 dir = Vector3Normalize(Vector3Subtract(position, from));
    velocity.x += dir.x * 3.0f;
    velocity.z += dir.z * 3.0f;
}

void Player::UpdateLook(float dt) {
    Vector2 m = GetMouseDelta();
    yaw   -= m.x * cfg::MOUSE_SENS;
    pitch -= m.y * cfg::MOUSE_SENS;
    if (pitch >  1.5f) pitch =  1.5f;
    if (pitch < -1.5f) pitch = -1.5f;

    recoilKick = std::max(0.0f, recoilKick - dt * 0.6f);
    hitFlash   = std::max(0.0f, hitFlash   - dt * 1.5f);
}

void Player::UpdateDash(float dt) {
    if (dashCharges < cfg::DASH_MAX_CHARGES) {
        dashRechargeTimer -= dt;
        if (dashRechargeTimer <= 0.0f) {
            dashCharges++;
            dashRechargeTimer = (dashCharges < cfg::DASH_MAX_CHARGES) ? cfg::DASH_RECHARGE_TIME : 0.0f;
        }
    }

    if (dashTimer > 0.0f) {
        dashTimer -= dt;
        velocity.x = dashDir.x * cfg::DASH_SPEED;
        velocity.z = dashDir.z * cfg::DASH_SPEED;
        velocity.y = 0.0f;
        if (dashTimer <= 0.0f) {
            float scale = 0.6f;
            velocity.x *= scale;
            velocity.z *= scale;
        }
    }

    if (IsKeyPressed(KEY_LEFT_CONTROL) && dashCharges > 0 && dashTimer <= 0.0f) {
        Vector3 fwd = YawForward(yaw);
        Vector3 rgt = YawRight(yaw);
        Vector3 wish{ 0, 0, 0 };
        if (IsKeyDown(KEY_W)) wish = Vector3Add(wish, fwd);
        if (IsKeyDown(KEY_S)) wish = Vector3Subtract(wish, fwd);
        if (IsKeyDown(KEY_D)) wish = Vector3Add(wish, rgt);
        if (IsKeyDown(KEY_A)) wish = Vector3Subtract(wish, rgt);
        if (Vector3Length(wish) < 0.001f) wish = fwd;
        dashDir = Vector3Normalize(wish);
        dashTimer = cfg::DASH_DURATION;
        dashCharges--;
        if (dashRechargeTimer <= 0.0f) dashRechargeTimer = cfg::DASH_RECHARGE_TIME;
    }
}

void Player::ApplyFriction(float dt, float frictionCoeff) {
    Vector3 horiz = HorizontalProjection(velocity);
    float speed = Vector3Length(horiz);
    if (speed < 0.01f) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }
    float drop = speed * frictionCoeff * dt;
    float newSpeed = std::max(0.0f, speed - drop);
    float scale = newSpeed / speed;
    velocity.x *= scale;
    velocity.z *= scale;
}

void Player::Accelerate(Vector3 wishDir, float wishSpeed, float accel, float dt) {
    float currentSpeed = velocity.x * wishDir.x + velocity.z * wishDir.z;
    float addSpeed = wishSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;
    float accelSpeed = accel * dt * wishSpeed;
    if (accelSpeed > addSpeed) accelSpeed = addSpeed;
    velocity.x += accelSpeed * wishDir.x;
    velocity.z += accelSpeed * wishDir.z;
}

void Player::UpdateMovement(float dt) {
    Vector3 fwd = YawForward(yaw);
    Vector3 rgt = YawRight(yaw);
    Vector3 wish{ 0, 0, 0 };
    if (IsKeyDown(KEY_W)) wish = Vector3Add(wish, fwd);
    if (IsKeyDown(KEY_S)) wish = Vector3Subtract(wish, fwd);
    if (IsKeyDown(KEY_D)) wish = Vector3Add(wish, rgt);
    if (IsKeyDown(KEY_A)) wish = Vector3Subtract(wish, rgt);
    bool moving = Vector3Length(wish) > 0.001f;
    if (moving) wish = Vector3Normalize(wish);

    bool sprintHeld = IsKeyDown(KEY_LEFT_SHIFT);
    bool canSprint  = sprintHeld && stamina > 0.0f && moving;
    float wishSpeed = canSprint ? cfg::SPRINT_SPEED : cfg::WALK_SPEED;

    if (canSprint) stamina = std::max(0.0f, stamina - cfg::STAMINA_DRAIN * dt);
    else           stamina = std::min(cfg::MAX_STAMINA, stamina + cfg::STAMINA_REGEN * dt);

    bool slideKeyPressed = IsKeyPressed(KEY_C);
    if (slideKeyPressed && grounded && HorizontalSpeed() > cfg::SLIDE_MIN_SPEED && !sliding) {
        sliding = true;
        slideTimer = cfg::SLIDE_MAX_TIME;
        Vector3 dir = Vector3Normalize(HorizontalProjection(velocity));
        velocity.x += dir.x * cfg::SLIDE_BOOST;
        velocity.z += dir.z * cfg::SLIDE_BOOST;
    }
    if (sliding) {
        slideTimer -= dt;
        if (slideTimer <= 0.0f || !grounded || HorizontalSpeed() < cfg::SLIDE_MIN_SPEED || IsKeyReleased(KEY_C)) {
            sliding = false;
        }
    }

    if (dashTimer <= 0.0f) {
        if (grounded) {
            ApplyFriction(dt, sliding ? cfg::SLIDE_FRICTION : cfg::GROUND_FRICTION);
            Accelerate(wish, wishSpeed, cfg::GROUND_ACCEL, dt);
        } else {
            float airWish = std::min(wishSpeed, cfg::MAX_AIR_SPEED);
            Accelerate(wish, airWish, cfg::AIR_ACCEL, dt);
        }
    }

    // Coyote time + jump buffering for snappier-feeling jumps.
    if (grounded)                 coyoteTimer = cfg::COYOTE_TIME;
    else                          coyoteTimer = std::max(0.0f, coyoteTimer - dt);
    if (IsKeyPressed(KEY_SPACE))  jumpBuffer  = cfg::JUMP_BUFFER;
    else                          jumpBuffer  = std::max(0.0f, jumpBuffer - dt);

    if (jumpBuffer > 0.0f && coyoteTimer > 0.0f) {
        velocity.y  = cfg::JUMP_VEL;
        grounded    = false;
        sliding     = false;
        coyoteTimer = 0.0f;
        jumpBuffer  = 0.0f;
    }

    if (dashTimer <= 0.0f) velocity.y -= cfg::GRAVITY * dt;
}

void Player::Update(float dt, const World& world) {
    UpdateLook(dt);
    UpdateDash(dt);
    UpdateMovement(dt);

    Vector3 desired = Vector3Add(position, Vector3Scale(velocity, dt));
    Vector3 resolved = world.ResolvePlayerMove(position, desired, cfg::PLAYER_RADIUS, velocity);

    position = resolved;
    if (position.y <= cfg::EYE_HEIGHT + 0.001f) {
        position.y = cfg::EYE_HEIGHT;
        if (velocity.y < 0.0f) velocity.y = 0.0f;
        grounded = true;
    } else {
        grounded = false;
    }
}

Camera3D Player::BuildCamera() const {
    Camera3D cam{};
    Vector3 eye = position;
    eye.y -= recoilKick * 0.3f;
    cam.position   = eye;
    Vector3 dir = Forward();
    cam.target     = Vector3Add(eye, dir);
    cam.up         = { 0.0f, 1.0f, 0.0f };
    cam.fovy       = sliding ? 82.0f : 75.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}
