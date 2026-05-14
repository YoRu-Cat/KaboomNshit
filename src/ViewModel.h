#pragma once
#include <raylib.h>

// 3D first-person weapon viewmodel. Draws a rifle composed of primitive
// boxes, transformed into the camera's local space via rlgl. Designed to be
// swapped later for a loaded mesh (LoadModel/.obj) without changing callers.
class ViewModel {
public:
    // Anchor offsets (from cfg) are applied automatically; pass the camera and
    // the player's current recoil & "firing" state so the model can react.
    void Draw(const Camera3D& cam, float recoilKick, bool firing, float swayPhase) const;

    // World-space position of the gun muzzle, used as the bullet spawn point.
    static Vector3 MuzzleWorldPosition(const Camera3D& cam, float recoilKick);
};
