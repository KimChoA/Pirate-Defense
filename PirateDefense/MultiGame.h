#pragma once

#include "Game.hpp"
#include "NetworkManager.h"

#include <array>

namespace dw
{
    struct Screen;

    class MultiGame
    {
    public:
        explicit MultiGame(NetworkManager& network);

        // 김초아 담당
        // Pirate 대기방의 기존 TCP 연결을 그대로 사용해서 게임 시작
        void start(Game& game);

        // MultiGame 내부 상태만 초기화
        // Pirate TCP 연결은 끊지 않음
        void stop();

        void restart(Game& game);

        bool active() const;
        bool isClient() const;
        bool disconnected() const;

        void update(
            Game& game,
            Input& pending,
            float elapsed
        );

        void updateScreen(
            Screen& screen
        ) const;

        Game visualState(
            const Game& game,
            float elapsed
        );

    private:
        NetworkManager& network_;

        float accumulator_ = 0.f;
        float inputClock_ = 0.f;

        std::array<Input, MaxPlayers> inputs_{};
        std::array<V, MaxPlayers> smooth_{};

        bool smoothing_ = false;
    };
}
