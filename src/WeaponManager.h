#pragma once
#include "Weapon.h"

// Owns the four procedural weapons + the currently equipped index.
// Cycle with mouse wheel; quick-select with number keys 1..N.
class WeaponManager {
public:
    WeaponManager();

    // Sets up every weapon's stats. Call once at startup.
    void ConfigureAll();

    // Reads input (mouse wheel + number keys) and updates current index.
    void HandleInput();

    void Next();
    void Prev();
    void Select(int index);

    int            CurrentIndex() const { return current; }
    const Weapon&  Current()      const { return weapons[current]; }
    Weapon&        Current()            { return weapons[current]; }

    static constexpr int Count = Weapon::TYPE_COUNT;

private:
    Weapon weapons[Weapon::TYPE_COUNT];
    int    current;
};
