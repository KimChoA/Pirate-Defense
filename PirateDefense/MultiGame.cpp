// ============================================================================
// [김초아 담당] 멀티플레이 코드 통합 지점
// NetworkManager의 TCP/IP 통신과 실제 Game 업데이트 루프를 연결합니다.
// 클라이언트 입력 전송, 서버 권위 업데이트, 게임 상태 브로드캐스트를 통합합니다.
// ============================================================================
#include "MultiGame.h"
#include "Render.hpp"

#include <algorithm>
#include <cmath>

namespace dw
{
    MultiGame::MultiGame(
        NetworkManager& network
    )
        : network_(network)
    {
    }

    void MultiGame::start(
        Game& game
    )
    {
        accumulator_ = 0.f;
        inputClock_ = 0.f;
        smoothing_ = false;
        inputs_ = {};

        game = Game{};

        if (isClient())
        {
            // 참가자는 방장의 GAME_STATE를 받아서 실제 상태를 덮어씁니다.
            network_.clearGameStarted();
            return;
        }

        // 방장은 실제 접속 인원만큼 플레이어를 활성화
        int playerId = 1;

        for (const auto& name :
            network_.getPlayerNicknames())
        {
            if (!name.isEmpty() &&
                playerId < MaxPlayers)
            {
                game.addPlayer(playerId);
                ++playerId;
            }
        }

        game.start();
    }

    void MultiGame::stop()
    {
        // 기존 Pirate TCP 연결은 여기서 종료하지 않습니다.
        accumulator_ = 0.f;
        inputClock_ = 0.f;
        smoothing_ = false;
        inputs_ = {};
    }

    void MultiGame::restart(
        Game& game
    )
    {
        if (isClient())
        {
            return;
        }

        game.start();
        inputs_ = {};
        accumulator_ = 0.f;
        inputClock_ = 0.f;
        smoothing_ = false;
    }

    bool MultiGame::active() const
    {
        return
            network_.isServerRunning() ||
            network_.isConnected();
    }

    bool MultiGame::isClient() const
    {
        return
            network_.isConnected() &&
            !network_.isServerRunning();
    }

    bool MultiGame::disconnected() const
    {
        return !active();
    }

    // [김초아 담당] 60Hz 입력 송수신 및 서버 권위 게임 상태 동기화
    void MultiGame::update(
        Game& game,
        Input& pending,
        float elapsed
    )
    {
        if (!active())
        {
            return;
        }

        // 참가자
        if (isClient())
        {
            inputClock_ += elapsed;

            if (inputClock_ >=
                1.f / 60.f)
            {
                network_.sendGameInput(
                    pending
                );

                pending.edgesOff();

                inputClock_ = 0.f;
            }

            // 방장이 보낸 최신 게임 상태를 적용
            network_.consumeGameState(
                game
            );

            return;
        }

        // 방장

        // 참가자 입력을 서버에서 가져옴
        network_.copyGameInputs(
            inputs_
        );

        // 방장 입력은 P1
        inputs_[0] = pending;

        accumulator_ += elapsed;

        constexpr float fixedStep =
            1.f / 60.f;

        while (accumulator_ >=
            fixedStep)
        {
            game.update(
                fixedStep,
                inputs_
            );

            // 1회성 입력은 한 번만 사용
            pending.edgesOff();

            for (auto& input :
                inputs_)
            {
                input.edgesOff();
            }

            accumulator_ -=
                fixedStep;
        }

        // 권위 있는 게임 상태를 참가자에게 전송
        network_.broadcastGameState(
            game
        );
    }

    void MultiGame::updateScreen(
        Screen& screen
    ) const
    {
        screen.online =
            active();

        screen.client =
            isClient();

        screen.local =
            isClient()
            ? network_.getLocalPlayerId()
            : 0;
    }

    Game MultiGame::visualState(
        const Game& game,
        float elapsed
    )
    {
        Game visual =
            game;

        if (!isClient())
        {
            smoothing_ = false;
            return visual;
        }

        for (int i = 0;
            i < MaxPlayers;
            ++i)
        {
            if (!smoothing_ ||
                dist(
                    smooth_[i],
                    game.players[i].p
                ) > 80.f)
            {
                smooth_[i] =
                    game.players[i].p;
            }
            else
            {
                smooth_[i] +=
                    (
                        game.players[i].p -
                        smooth_[i]
                        )
                    *
                    (
                        1.f -
                        std::exp(
                            -elapsed * 30.f
                        )
                        );
            }

            visual.players[i].p =
                smooth_[i];
        }

        smoothing_ = true;

        return visual;
    }
}
