#include "Game.h"

int main(int argc, char** argv) {

    const int windowSizeX = 1200;
    const int windowSizeY = 1920;
    const int paddleSize = int(0.2 * windowSizeY);

    Game game(windowSizeX, windowSizeY, paddleSize, true);
    bool success = game.Initialize();
    if (success)
        game.GameLoop();
    game.ShutDownGame();
    return 0;
}