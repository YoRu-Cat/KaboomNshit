#pragma once

namespace cfg {
    constexpr int   SCREEN_W            = 1280;
    constexpr int   SCREEN_H            = 720;

    constexpr float EYE_HEIGHT          = 1.7f;
    constexpr float CROUCH_HEIGHT       = 1.0f;
    constexpr float PLAYER_RADIUS       = 0.35f;
    constexpr float MOUSE_SENS          = 0.0022f;

    constexpr float WALK_SPEED          = 11.0f;
    constexpr float SPRINT_SPEED        = 19.0f;
    constexpr float MAX_AIR_SPEED       = 24.0f;
    constexpr float GROUND_ACCEL        = 150.0f;
    constexpr float AIR_ACCEL           = 60.0f;
    constexpr float GROUND_FRICTION     = 11.0f;
    constexpr float SLIDE_FRICTION      = 0.8f;

    constexpr float JUMP_VEL            = 13.0f;
    constexpr float GRAVITY             = 34.0f;
    constexpr float COYOTE_TIME         = 0.12f;
    constexpr float JUMP_BUFFER         = 0.14f;
    constexpr float SLIDE_BOOST         = 6.0f;
    constexpr float SLIDE_MIN_SPEED     = 4.0f;
    constexpr float SLIDE_MAX_TIME      = 1.2f;

    constexpr float DASH_SPEED          = 40.0f;
    constexpr float DASH_DURATION       = 0.18f;
    constexpr int   DASH_MAX_CHARGES    = 3;
    constexpr float DASH_RECHARGE_TIME  = 1.4f;

    constexpr float STAMINA_DRAIN       = 30.0f;
    constexpr float STAMINA_REGEN       = 22.0f;
    constexpr float MAX_STAMINA         = 100.0f;
    constexpr float MAX_HP              = 100.0f;

    constexpr float FIRE_COOLDOWN       = 0.09f;
    constexpr float BULLET_DAMAGE       = 28.0f;
    constexpr float BULLET_SPEED        = 75.0f;
    constexpr float BULLET_LIFETIME     = 2.5f;
    constexpr float BULLET_RADIUS       = 0.08f;
    constexpr float BULLET_TRAIL_LEN    = 1.6f;
    constexpr float MUZZLE_FLASH_TIME   = 0.06f;
    constexpr float RECOIL_KICK         = 0.08f;   // radians of camera pitch up per shot

    constexpr float VIEWMODEL_FORWARD   = 0.55f;
    constexpr float VIEWMODEL_RIGHT     = 0.28f;
    constexpr float VIEWMODEL_DOWN      = 0.30f;

    constexpr float CAMERA_SHAKE_DECAY  = 7.0f;
    constexpr float SHAKE_ON_FIRE       = 0.04f;
    constexpr float SHAKE_ON_HIT        = 0.08f;
    constexpr float SHAKE_ON_EXPLOSION  = 0.45f;

    constexpr float HITSTOP_ON_KILL     = 0.06f;
    constexpr float HITSTOP_TIME_SCALE  = 0.10f;

    constexpr float GRENADE_THROW_SPEED = 22.0f;
    constexpr float GRENADE_UP_BIAS     = 5.0f;
    constexpr float GRENADE_GRAVITY     = 22.0f;
    constexpr float GRENADE_FUSE        = 1.8f;
    constexpr float GRENADE_RADIUS      = 0.22f;
    constexpr float GRENADE_BOUNCE      = 0.45f;
    constexpr float GRENADE_FRICTION    = 0.85f;
    constexpr float GRENADE_THROW_CD    = 0.7f;

    constexpr float EXPLOSION_RADIUS    = 6.5f;
    constexpr float EXPLOSION_DAMAGE    = 120.0f;
    constexpr float EXPLOSION_DURATION  = 0.55f;
    constexpr float EXPLOSION_KNOCKBACK = 14.0f;
    constexpr float SELF_DAMAGE_MULT    = 0.35f;

    constexpr int   PARTICLES_PER_HIT   = 6;
    constexpr int   PARTICLES_PER_BOOM  = 24;
    constexpr float PARTICLE_LIFETIME   = 0.45f;

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
