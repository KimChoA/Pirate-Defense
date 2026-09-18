#include "SingleGame.h"

namespace dw
{
    void SingleGame::start(Game& game)
    {
        reset();
        game.start();
    }

    void SingleGame::reset()
    {
        accumulator_ = 0;
    }

    void SingleGame::update(Game& game, Input& pending, float elapsed, bool running)
    {
        if (!running)
        {
            reset();
            return;
        }

        accumulator_ += elapsed;
        while (accumulator_ >= 1.f / 60)
        {
            std::array<Input, MaxPlayers> inputs{};
            inputs[0] = pending;
            game.update(1.f / 60, inputs);
            pending.edgesOff();
            accumulator_ -= 1.f / 60;
        }
    }
} // namespace dw
