#pragma once
#include "Game.hpp"

namespace dw
{
    // 싱글 모드: 로컬 입력만 사용하며 도움말/일시정지 시 시뮬레이션을 멈춤
    class SingleGame
    {
    public:
        void start(Game& game);
        void reset();
        void update(Game& game, Input& pending, float elapsed, bool running);

    private:
        float accumulator_ = 0;
    };
} // namespace dw
