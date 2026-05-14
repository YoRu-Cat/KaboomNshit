#include "Game.h"

int main() {
    Game game;
    if (!game.Initialize()) return 1;

    while (!game.ShouldClose()) {
        game.Tick();
    }

    game.Shutdown();
    return 0;
}
