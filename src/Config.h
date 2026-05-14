#pragma once

namespace cfg {
    constexpr int   SCREEN_W            = 1280;
    constexpr int   SCREEN_H            = 720;

    constexpr float EYE_HEIGHT          = 1.7f;
    constexpr float CROUCH_HEIGHT       = 1.0f;
    constexpr float PLAYER_RADIUS       = 0.35f;
    constexpr float MOUSE_SENS          = 0.0022f;

    constexpr float WALK_SPEED          = 7.0f;
    constexpr float SPRINT_SPEED        = 12.0f;
    constexpr float MAX_AIR_SPEED       = 14.0f;
    constexpr float GROUND_ACCEL        = 80.0f;
    constexpr float AIR_ACCEL           = 28.0f;
    constexpr float GROUND_FRICTION     = 9.0f;
    constexpr float SLIDE_FRICTION      = 1.2f;

    constexpr float JUMP_VEL            = 8.5f;
    constexpr float GRAVITY             = 26.0f;
    constexpr float SLIDE_BOOST         = 4.0f;
    constexpr float SLIDE_MIN_SPEED     = 3.0f;
    constexpr float SLIDE_MAX_TIME      = 1.0f;

    constexpr float DASH_SPEED          = 28.0f;
    constexpr float DASH_DURATION       = 0.18f;
    constexpr int   DASH_MAX_CHARGES    = 3;
    constexpr float DASH_RECHARGE_TIME  = 1.6f;

    constexpr float STAMINA_DRAIN       = 30.0f;
    constexpr float STAMINA_REGEN       = 22.0f;
    constexpr float MAX_STAMINA         = 100.0f;
    constexpr float MAX_HP              = 100.0f;

    constexpr float FIRE_COOLDOWN       = 0.11f;
    constexpr float BULLET_DAMAGE       = 28.0f;
    constexpr float BULLET_RANGE        = 200.0f;
    constexpr float TRACER_LIFETIME     = 0.06f;
    constexpr float RECOIL_KICK         = 0.05f;

    constexpr float ENEMY_HP            = 60.0f;
    constexpr float ENEMY_SPEED         = 3.8f;
    constexpr float ENEMY_HEIGHT        = 1.8f;
    constexpr float ENEMY_RADIUS        = 0.45f;
    constexpr float ENEMY_DAMAGE        = 14.0f;
    constexpr float ENEMY_ATTACK_RANGE  = 1.2f;
    constexpr float ENEMY_ATTACK_CD     = 0.8f;
    constexpr float ENEMY_KNOCKBACK     = 6.0f;

    constexpr float MAP_HALF            = 45.0f;
}
