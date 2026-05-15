#include "ViewModel.h"
#include "Weapon.h"
#include "Config.h"
#include <rlgl.h>
#include <raymath.h>

namespace {
    // Viewmodel-camera local anchor. In a camera at origin looking +Z with
    // up = +Y, screen-right is world -X. Negative X = screen right.
    constexpr float ANCHOR_X = -0.18f;
    constexpr float ANCHOR_Y = -0.18f;
    constexpr float ANCHOR_Z =  0.50f;
}

Vector3 ViewModel::MuzzleWorldPosition(const Camera3D& cam, float recoilKick) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 p   = Vector3Add(cam.position, Vector3Scale(fwd, 0.9f));
    p.y -= recoilKick * 0.1f;
    return p;
}

void ViewModel::Draw(const Camera3D& /*playerCam*/, const Weapon& weapon,
                     float recoilKick, bool firing, float swayPhase) const {
    Camera3D vm{};
    vm.position   = { 0.0f, 0.0f, 0.0f };
    vm.target     = { 0.0f, 0.0f, 1.0f };
    vm.up         = { 0.0f, 1.0f, 0.0f };
    vm.fovy       = 60.0f;
    vm.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(vm);
        rlDrawRenderBatchActive();
        rlDisableDepthTest();

        weapon.Draw({ ANCHOR_X, ANCHOR_Y, ANCHOR_Z }, recoilKick, firing, swayPhase);

        rlDrawRenderBatchActive();
        rlEnableDepthTest();
    EndMode3D();
}
