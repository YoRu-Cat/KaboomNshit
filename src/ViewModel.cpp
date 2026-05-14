#include "ViewModel.h"
#include "Config.h"
#include <rlgl.h>
#include <raymath.h>
#include <cmath>

namespace {
    struct Basis {
        Vector3 right;
        Vector3 up;
        Vector3 forward;
        Vector3 anchor;
    };

    Basis ComputeBasis(const Camera3D& cam, float recoilKick, float swayPhase) {
        Basis b{};
        b.forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
        b.right   = Vector3Normalize(Vector3CrossProduct(b.forward, cam.up));
        b.up      = Vector3CrossProduct(b.right, b.forward);

        float swayX = sinf(swayPhase) * 0.012f;
        float swayY = cosf(swayPhase * 1.7f) * 0.010f;

        b.anchor = cam.position;
        b.anchor = Vector3Add(b.anchor, Vector3Scale(b.forward, cfg::VIEWMODEL_FORWARD));
        b.anchor = Vector3Add(b.anchor, Vector3Scale(b.right,   cfg::VIEWMODEL_RIGHT + swayX));
        b.anchor = Vector3Add(b.anchor, Vector3Scale(b.up,     -cfg::VIEWMODEL_DOWN  + swayY));

        // Recoil: pull weapon back and up slightly.
        float pull = recoilKick * 1.4f;
        b.anchor = Vector3Subtract(b.anchor, Vector3Scale(b.forward, pull));
        b.anchor = Vector3Add(b.anchor,      Vector3Scale(b.up,      pull * 0.35f));
        return b;
    }

    void DrawPart(Vector3 c, Vector3 s, Color fill, Color wire) {
        DrawCubeV(c, s, fill);
        DrawCubeWiresV(c, s, wire);
    }
}

Vector3 ViewModel::MuzzleWorldPosition(const Camera3D& cam, float recoilKick) {
    Basis b = ComputeBasis(cam, recoilKick, 0.0f);
    Vector3 p = b.anchor;
    p = Vector3Add(p, Vector3Scale(b.forward, 0.46f));
    p = Vector3Add(p, Vector3Scale(b.up,      0.02f));
    return p;
}

void ViewModel::Draw(const Camera3D& cam, float recoilKick, bool firing, float swayPhase) const {
    Basis b = ComputeBasis(cam, recoilKick, swayPhase);

    rlDrawRenderBatchActive();
    rlDisableDepthTest();

    rlPushMatrix();
        rlTranslatef(b.anchor.x, b.anchor.y, b.anchor.z);

        float m[16] = {
            b.right.x,   b.right.y,   b.right.z,   0.0f,
            b.up.x,      b.up.y,      b.up.z,      0.0f,
            b.forward.x, b.forward.y, b.forward.z, 0.0f,
            0.0f,        0.0f,        0.0f,        1.0f
        };
        rlMultMatrixf(m);

        // Slight roll based on recoil for added punch.
        float roll = recoilKick * 0.6f;
        rlRotatef(roll * 57.2958f, 0.0f, 0.0f, 1.0f);

        // Receiver (main body).
        DrawPart({ 0.00f,  0.00f,  0.05f }, { 0.10f, 0.11f, 0.32f }, WHITE, BLACK);
        // Top rail.
        DrawPart({ 0.00f,  0.07f,  0.05f }, { 0.06f, 0.03f, 0.22f }, WHITE, BLACK);
        // Iron sight rear.
        DrawPart({ 0.00f,  0.10f, -0.04f }, { 0.03f, 0.04f, 0.02f }, BLACK, BLACK);
        // Iron sight front.
        DrawPart({ 0.00f,  0.10f,  0.16f }, { 0.02f, 0.04f, 0.02f }, BLACK, BLACK);
        // Barrel (long, thin).
        DrawPart({ 0.00f,  0.025f, 0.34f }, { 0.05f, 0.05f, 0.30f }, WHITE, BLACK);
        // Muzzle brake (slightly wider).
        DrawPart({ 0.00f,  0.025f, 0.48f }, { 0.07f, 0.07f, 0.04f }, WHITE, BLACK);
        // Stock / cheek-rest behind receiver.
        DrawPart({ 0.00f, -0.01f, -0.18f }, { 0.07f, 0.08f, 0.18f }, WHITE, BLACK);
        // Buttpad.
        DrawPart({ 0.00f, -0.03f, -0.30f }, { 0.07f, 0.13f, 0.04f }, BLACK, BLACK);
        // Pistol grip.
        DrawPart({ 0.00f, -0.13f, -0.05f }, { 0.06f, 0.16f, 0.07f }, WHITE, BLACK);
        // Trigger guard.
        DrawPart({ 0.00f, -0.10f,  0.02f }, { 0.05f, 0.03f, 0.07f }, BLACK, BLACK);
        // Magazine.
        DrawPart({ 0.00f, -0.16f,  0.08f }, { 0.06f, 0.16f, 0.09f }, WHITE, BLACK);
        // Forward grip / handguard.
        DrawPart({ 0.00f, -0.05f,  0.22f }, { 0.07f, 0.05f, 0.16f }, WHITE, BLACK);
        // Red detail (the signature splash from the reference screens).
        DrawPart({ 0.00f, -0.02f,  0.12f }, { 0.075f, 0.025f, 0.10f }, RED, BLACK);

        if (firing) {
            // Bright wireframe muzzle starburst — drawn at the barrel tip.
            Vector3 fc{ 0.0f, 0.025f, 0.52f };
            float fs = 0.18f;
            DrawCubeWiresV(fc, { fs, fs, fs }, BLACK);
            DrawCubeWiresV(fc, { fs * 0.6f, fs * 1.2f, fs * 0.6f }, BLACK);
            DrawCubeV(fc, { fs * 0.4f, fs * 0.4f, fs * 0.4f }, YELLOW);
        }

    rlPopMatrix();

    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}
