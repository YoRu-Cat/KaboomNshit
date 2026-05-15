#include "WeaponManager.h"
#include <raylib.h>

WeaponManager::WeaponManager() : current(0) {}

void WeaponManager::ConfigureAll() {
    for (int i = 0; i < Weapon::TYPE_COUNT; ++i) {
        weapons[i].Configure((Weapon::Type)i);
    }
    current = 0;
}

void WeaponManager::HandleInput() {
    float wheel = GetMouseWheelMove();
    if (wheel > 0.5f)  Next();
    if (wheel < -0.5f) Prev();

    if (IsKeyPressed(KEY_ONE))   Select(0);
    if (IsKeyPressed(KEY_TWO))   Select(1);
    if (IsKeyPressed(KEY_THREE)) Select(2);
    if (IsKeyPressed(KEY_FOUR))  Select(3);
}

void WeaponManager::Next() {
    current = (current + 1) % Weapon::TYPE_COUNT;
}

void WeaponManager::Prev() {
    current = (current - 1 + Weapon::TYPE_COUNT) % Weapon::TYPE_COUNT;
}

void WeaponManager::Select(int index) {
    if (index < 0 || index >= Weapon::TYPE_COUNT) return;
    current = index;
}
