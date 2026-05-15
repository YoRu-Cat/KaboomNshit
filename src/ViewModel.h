#pragma once
#include <raylib.h>

class Weapon;

// Renders a Weapon at a fixed on-screen anchor using a dedicated viewmodel
// camera pass. The Weapon owns its own mesh + per-weapon offset; ViewModel
// just owns the camera setup and the local-space anchor.
class ViewModel {
public:
    void Draw(const Camera3D& playerCam, const Weapon& weapon,
              float recoilKick, bool firing, float swayPhase) const;

    static Vector3 MuzzleWorldPosition(const Camera3D& cam, float recoilKick);
};
