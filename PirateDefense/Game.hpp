#pragma once
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "ItemSystem.hpp"

namespace dw
{
    constexpr float StageSeconds = 90, ShipMax = 1000, Pi = 3.14159265f;
    constexpr float WaveSeconds = 30.f;
    constexpr int MaxStage = 3;
    constexpr int MaxWave = 3;
    constexpr int MaxPlayers = 5;
    constexpr int CannonCount = 3;

    // 640 x 360 기준 새 맵 배치
    // 왼쪽: 좁은 바다 + 낚시 공간
    // 중앙: 플레이어 갑판
    // 오른쪽: 적이 들어오는 전투 구역
    constexpr float DeckLeft = 92.f;
    constexpr float DeckRight = 332.f;
    constexpr float DeckTop = 58.f;
    constexpr float DeckBottom = 322.f;

    constexpr float ShipHitLeft = 88.f;
    constexpr float ShipHitRight = 339.f;

    using V = sf::Vector2f;

    inline const V AmmoPoint{ 165.f, 180.f };
    inline const V FishingPoint{ 62.f, 180.f };
    inline constexpr float FishingInteractRadius = 60.f;
    inline float length(V v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }
    inline float dist(V a, V b)
    {
        return length(a - b);
    }
    inline V unit(V v)
    {
        float l = length(v);
        return l > 0 ? v / l : V{ 1, 0 };
    }
    enum class Phase : int
    {
        Lobby,
        Play,
        Reward,
        Won,
        Lost,
        Shop
    };

    // 전투 테마. 각 테마는 Stage 1~3을 독립적으로 사용합니다.
    // Sea  : 기존 바다 전투
    // Island: 섬 앞 전투(전투 판정은 재사용하고 외형/연출만 육지 테마로 변경)
    enum class Theme : int
    {
        Sea = 0,
        Island = 1
    };

    enum Shell : int
    {
        Normal = 0,
        Spread,
        Pierce,
        Blast,
        Flame,
        Heavy,
        ShellCount
    };

    inline const char* shellName(int n)
    {
        static const char* names[] =
        {
            "IRON",
            "SPREAD",
            "PIERCE",
            "BLAST",
            "FLAME",
            "HEAVY"
        };

        return n >= 0 && n < ShellCount
            ? names[n]
            : "NONE";
    }
    struct Input
    {
        float x = 0, y = 0;
        V aim{ 0, 180 };

        bool hold = false;
        bool tap = false;
        bool drop = false;
        bool fire = false;


        bool melee = false;
        bool brace = false;

        // 물고기 섭취(회복) 입력
        bool useFish = false;

        // 상점 입력
        int buy = -1;              // 구매 제안 상품 번호
        int sell = 0;              // 판매할 물고기 수
        bool ready = false;        // 상점 준비 완료

        int shopVisit = 0;         // 현재 몇 번째 상점인지

        int purchaseVote = -1;     // -1 미투표 / 0 반대 / 1 찬성
        int purchaseVoteId = 0;    // 현재 구매 투표 번호

        int select = -1;
        int vote = -1;

        void edgesOff()
        {
            tap = drop = fire = melee = useFish = false;

            select = vote = buy = -1;

            sell = 0;
            ready = false;
            purchaseVote = -1;
        }
    };

    inline void mergeInput(
        Input& pending,
        const Input& fresh)
    {
        Input merged = fresh;

        merged.tap |= pending.tap;
        merged.drop |= pending.drop;
        merged.fire |= pending.fire;
        merged.melee |= pending.melee;
        merged.useFish |= pending.useFish;
        merged.ready |= pending.ready;

        if (merged.select < 0)
            merged.select = pending.select;

        if (merged.vote < 0)
            merged.vote = pending.vote;

        if (merged.buy < 0)
            merged.buy = pending.buy;

        if (merged.sell == 0)
            merged.sell = pending.sell;

        if (merged.purchaseVote < 0)
            merged.purchaseVote =
            pending.purchaseVote;

        if (merged.shopVisit == 0)
            merged.shopVisit =
            pending.shopVisit;

        if (merged.purchaseVoteId == 0)
            merged.purchaseVoteId =
            pending.purchaseVoteId;

        pending = merged;
    }

    struct Player
    {
        bool active = false;
        V p{ 320, 218 };
        int held = -1, selected = 0, mount = -1, vote = -1;

        // 낚시 상태는 플레이어에 귀속
        bool fishing = false;
        float fishProgress = 0.f; // 0.0 ~ 1.0
        bool fishCancelArmed = false; // 시작 E가 끝난 뒤 다음 E부터 취소 허용

        // 낚시 획득 연출
        float fishCatchFx = 0.f;
        int fishCatchAmount = 0;
    };

    struct Cannon
    {
        V p;
        int side = 0, ammo = -1, owner = -1, loader = -1;
        float progress = 0, angle = 0;
    };
    struct Hazard
    {
        V p;
        bool fire = false;
        float progress = 0, age = 0;
    };
    struct Drop
    {
        V p;
        int type = 0;
    };
    struct Enemy
    {
        int id = 0, type = 0, side = 0;
        V p;

        float hp = 0, maxHp = 0, cooldown = 0, burn = 0, freeze = 0, age = 0;

        // 잡몹 방어력
        float defense = 0.f;
    };
    struct Shot
    {
        V p, v;
        int type = 0;

        bool hostile = false;

        float damage = 0;
        float life = 4;

        std::vector<int> hit;

        bool black = false;
    };

    struct Effect
    {
        V p;
        float life = 0, maxLife = 0;
        int kind = 0;
    };

    enum class BossType
    {
        None = 0,
        GhostShip,   // 스테이지 1
        Leviathan,   // 스테이지 2
        Kraken       // 스테이지 3
    };

    struct Boss
    {
        bool active = false;
        V p{ 565, 180 };
        float hp = 0, maxHp = 0, cooldown = 5, warning = 0, burn = 0, freeze = 0;
        int pattern = 0, side = 1;
        BossType type = BossType::None;
        float specialCooldown = 0.f;
    };

    // 보스 사각형 장판 공격
    struct AreaAttack
    {
        float x;
        float y;
        float width;
        float height;
        float damage;

        bool contains(const V& pos, float margin = 6.f) const
        {
            return
                pos.x >= x - margin &&
                pos.x <= x + width + margin &&
                pos.y >= y - margin &&
                pos.y <= y + height + margin;
        }
    };

    inline const AreaAttack TentacleSlamArea{
        240.f, 238.f, 160.f, 24.f, 40.f
    };

    inline const AreaAttack FireSurgeArea{
        240.f, 118.f, 160.f, 24.f, 30.f
    };

    struct CrewCombat
    {
        float hp = 100.f;             // 개인 체력
        float maxHp = 100.f;          // 개인 최대 체력
        float invulnerable = 0.f;     // 피격 후 무적시간
        float revive = 0.f;           // 부활 대기시간

        float attackCooldown = 0.f;   // C 근접공격 쿨타임
        float swing = 0.f;            // 근접공격 애니메이션 시간

        bool bracing = false;         // C 홀드 파도 방어
    };



    // 유령선 보스 - 유령 선원

    struct GhostSailor
    {
        V p;                          // 현재 위치
        V strike;                     // 공격 목표 위치

        float hp = 60.f;

        float boarding = 1.2f;        // 승선 시간
        float cooldown = 0.6f;        // 공격 쿨타임
        float windup = 0.f;           // 공격 준비시간
    };



    // 크라켄 보스 - 촉수

    struct DeckTentacle
    {
        V p;

        float hp = 90.f;

        float warning = 1.6f;
        float slamTimer = 5.f;
        float flash = 0.f;

        float slamAnimation = 0.f;

        // 촉수가 어느 방향으로 쓰러질지
        float slamDirection() const
        {
            return p.x > (DeckLeft + DeckRight) * 0.5f
                ? -1.f
                : 1.f;
        }

        // 촉수 공격 지점
        V slamPoint() const
        {
            return p + V{ slamDirection() * 48.f, 0.f };
        }

        // 촉수 회전각
        float slamAngle() const
        {
            if (slamAnimation > 0.f)
            {
                float recovery =
                    std::clamp(
                        (0.45f - slamAnimation - 0.08f) / 0.37f,
                        0.f,
                        1.f
                    );

                recovery =
                    recovery * recovery * (3.f - 2.f * recovery);

                return (Pi * 0.5f) * (1.f - recovery);
            }

            if (slamTimer > 0.75f)
                return 0.f;

            if (slamTimer > 0.16f)
            {
                float t = (0.75f - slamTimer) / 0.59f;
                return -0.6f * t * t * (3.f - 2.f * t);
            }

            float t =
                std::clamp(
                    (0.16f - slamTimer) / 0.16f,
                    0.f,
                    1.f
                );

            return -0.6f + (Pi * 0.5f + 0.6f) * t * t;
        }

        // 촉수 그릴 때 각 마디 위치
        V segmentPoint(int j, float age) const
        {
            float f = j / 8.f;
            float angle = slamAngle();

            bool moving =
                slamAnimation > 0.f ||
                slamTimer <= 0.75f;

            return p + V{
                slamDirection() *
                    std::sin(angle) *
                    48.f *
                    f,

                -std::cos(angle) *
                    48.f *
                    f
            }
                + V{
                    0.f,
                    moving
                        ? 0.f
                        : std::sin(age * 2.f + j * 0.4f)
                            * 3.f * f
            };
        }
    };


    // 레비아탄 / 크라켄 - 큰 파도
    struct GreatWave
    {
        bool active = false;          // 파도 존재 여부
        bool impacting = false;       // 실제 피격 판정 시작 여부

        float warning = 0.f;          // 경고 시간
        float front = DeckRight + 30.f;

        // 이미 맞은 플레이어 기록
        std::uint32_t hitPlayers = 0;
    };
    class Game
    {
    public:
        Phase phase = Phase::Lobby;
        Theme theme = Theme::Sea; // 최종 진행: 바다 Stage 1부터 시작
        int stage = 1, wave = 1, kills = 0, nextId = 1, reward = 0;
        // 현재 웨이브 안에서 몇 번째 적 무리가 생성됐는지
        int spawnGroup = 0;

        float waveTimer = WaveSeconds;
        float waveTransitionTimer = 0.f;
        bool bossWave = false;

        float hp = ShipMax, maxHp = ShipMax, time = WaveSeconds, age = 0, spawn = 2, repairBonus = 0, leakScale = 1;


        std::uint32_t rng = 91827, shotsFired = 0, impacts = 0;
        std::array<Player, MaxPlayers> players{};

        AmmoManager ammo;
        GoldManager gold;
        UpgradeManager upgrades;

        int fishCount = 0;

        // 상점 상태
        int shopVisit = 0;

        float shopRemaining = 0.f;

        // 각 플레이어 준비 여부
        std::array<bool, MaxPlayers> shopReady{};

        // 구매 결과 메시지용
        // 0 없음 / 1 구매성공 / 2 골드부족
        // 3 최대강화 / 4 판매완료 / 5 투표부결
        std::array<int, MaxPlayers> shopResult{};

        std::array<float, MaxPlayers> shopResultTime{};


        // 구매 투표
        bool purchaseVoteActive = false;

        int purchaseVoteItem = -1;
        int purchaseVoteProposer = -1;
        int purchaseVoteId = 0;

        float purchaseVoteRemaining = 0.f;

        // -1 미투표 / 0 반대 / 1 찬성
        std::array<int, MaxPlayers> purchaseBallots{
            -1, -1, -1, -1, -1
        };

        std::array<CrewCombat, MaxPlayers> crew{};

        // 오른쪽 한쪽에 대포 3문 배치
        std::array<Cannon, CannonCount> cannons{
             {{{307.f, 110.f}, 1},
              {{307.f, 180.f}, 1},
              {{307.f, 250.f}, 1}}
        };
        std::vector<Enemy> enemies;
        std::vector<Shot> shots;
        std::vector<Hazard> hazards;
        std::vector<Drop> drops;
        std::vector<Effect> effects;

        Boss boss;

        std::vector<GhostSailor> sailors;
        std::vector<DeckTentacle> tentacles;

        GreatWave greatWave;
        float deckSpawn = 3.f;

        Game()
        {
            players[0].active = true;
            crew[0].hp = 100.f;
            crew[0].maxHp = 100.f;
            players[0].fishing = false;
            players[0].fishProgress = 0.f;
            players[0].fishCancelArmed = false;
            players[0].fishCatchFx = 0.f;
            players[0].fishCatchAmount = 0;
        }

        bool isIslandTheme() const
        {
            return theme == Theme::Island;
        }

        const char* themeName() const
        {
            return isIslandTheme() ? "ISLAND" : "SEA";
        }

        // 현재 테마에서 다음 스테이지로 이동
        // 바다 Stage 3 다음에는 섬 Stage 1로 넘어가고,
        // 섬 Stage 3을 끝낸 뒤에만 최종 승리
        void advanceThemeStage()
        {
            if (theme == Theme::Sea && stage >= MaxStage)
            {
                theme = Theme::Island;
                stage = 1;
            }
            else
            {
                ++stage;
            }
        }

        // 스테이지에 따른 보스 종류 반환
        // Island 테마도 공격 패턴은 기존 3종을 그대로 재사용
        BossType bossTypeForStage() const
        {
            switch (stage)
            {
            case 1:
                return BossType::GhostShip;

            case 2:
                return BossType::Leviathan;

            case 3:
                return BossType::Kraken;

            default:
                return BossType::None;
            }
        }

        void openShop()
        {
            phase = Phase::Shop;

            ++shopVisit;

            // 상점 이용 시간 30초
            shopRemaining = 30.f;

            // 플레이어 준비 상태 초기화
            shopReady.fill(false);

            // 결과 메시지 초기화
            shopResult.fill(0);
            shopResultTime.fill(0.f);

            // 구매 투표 초기화
            purchaseVoteActive = false;

            purchaseVoteItem = -1;
            purchaseVoteProposer = -1;

            purchaseVoteRemaining = 0.f;

            purchaseBallots.fill(-1);
        }

        float purchaseVoteWeight(int choice) const
        {
            float total = 0.f;

            for (int id = 0; id < MaxPlayers; ++id)
            {
                if (
                    players[id].active &&
                    purchaseBallots[id] == choice
                    )
                {
                    
                    total += (id == 0) ? 1.5f : 1.0f;
                }
            }

            return total;
        }

        void finishPurchaseVote()
        {
            if (!purchaseVoteActive)
                return;

            purchaseVoteActive = false;
            purchaseVoteRemaining = 0.f;

            // 기본값: 부결
            int result = 5;

            // 찬성이 반대보다 많아야 통과
            // 동률은 부결
            if (
                purchaseVoteWeight(1) >
                purchaseVoteWeight(0)
                )
            {
                ShopManager shop(
                    gold,
                    ammo,
                    upgrades
                );

                const auto item =
                    ShopItems[purchaseVoteItem];

                // 배 HP 강화 전 보너스 저장
                int oldHpBonus =
                    upgrades.getShipHpBonus();

                if (shop.isMaxLevel(item))
                {
                    result = 3;
                }
                else if (shop.buyItem(item))
                {
                    result = 1;

                    // 배 HP 강화 구매 시
                    // 실제 Game maxHp에도 적용
                    int newHpBonus =
                        upgrades.getShipHpBonus();

                    int extraHp =
                        newHpBonus - oldHpBonus;

                    if (extraHp > 0)
                    {
                        maxHp +=
                            static_cast<float>(extraHp);

                        hp = std::min(
                            maxHp,
                            hp + static_cast<float>(extraHp)
                        );
                    }
                }
                else
                {
                    // 골드 부족 등
                    result = 2;
                }
            }

            // 모든 플레이어에게 결과 표시
            for (int id = 0; id < MaxPlayers; ++id)
            {
                if (!players[id].active)
                    continue;

                shopResult[id] = result;
                shopResultTime[id] = 3.f;
            }

            purchaseVoteItem = -1;
            purchaseVoteProposer = -1;
        }


        // 상점 업데이트
        void updateShop(
            float dt,
            const std::array<Input, MaxPlayers>& inputs)
        {
            // 구매 투표 중에는 상점 30초 타이머 정지
            if (!purchaseVoteActive)
            {
                shopRemaining =
                    std::max(
                        0.f,
                        shopRemaining - dt
                    );
            }

            // 구매 투표는 별도 10초
            if (purchaseVoteActive)
            {
                purchaseVoteRemaining =
                    std::max(
                        0.f,
                        purchaseVoteRemaining - dt
                    );
            }

            bool proposedThisStep = false;

            // 플레이어별 상점 입력
            if (shopRemaining > 0.f)
            {
                for (int id = 0; id < MaxPlayers; ++id)
                {
                    shopResultTime[id] =
                        std::max(
                            0.f,
                            shopResultTime[id] - dt
                        );

                    if (!players[id].active)
                        continue;

                    const auto& in =
                        inputs[id];

                    // 이전 상점 입력 무시
                    if (in.shopVisit != shopVisit)
                        continue;


                    // 구매 찬반 투표
                    if (
                        purchaseVoteActive &&
                        in.purchaseVoteId == purchaseVoteId &&
                        (
                            in.purchaseVote == 0 ||
                            in.purchaseVote == 1
                            ) &&
                        purchaseBallots[id] < 0
                        )
                    {
                        purchaseBallots[id] =
                            in.purchaseVote;
                    }


                    // 준비 완료한 플레이어는 새 구매 제안 불가
                    if (shopReady[id])
                        continue;

                    // 구매 제안
                    if (
                        in.buy >= 0 &&
                        in.buy < ShopItemCount &&
                        !purchaseVoteActive &&
                        !proposedThisStep &&
                        in.purchaseVoteId == purchaseVoteId
                        )
                    {
                        ShopManager shop(
                            gold,
                            ammo,
                            upgrades
                        );

                        const auto item =
                            ShopItems[in.buy];

                        // 최대 강화
                        if (shop.isMaxLevel(item))
                        {
                            shopResult[id] = 3;
                            shopResultTime[id] = 2.f;
                        }

                        // 골드 부족
                        else if (
                            !gold.canAfford(
                                shop.getPrice(item)
                            )
                            )
                        {
                            shopResult[id] = 2;
                            shopResultTime[id] = 2.f;
                        }

                        else
                        {
                            // 구매 투표 시작
                            purchaseVoteActive = true;
                            proposedThisStep = true;

                            purchaseVoteItem =
                                in.buy;

                            purchaseVoteProposer =
                                id;

                            ++purchaseVoteId;

                            purchaseVoteRemaining =
                                10.f;

                            purchaseBallots.fill(-1);
                        }
                    }


                    // 준비 완료
                    if (
                        in.ready &&
                        !purchaseVoteActive
                        )
                    {
                        shopReady[id] = true;
                    }
                }
            }


            // 구매 투표 종료
            if (purchaseVoteActive)
            {
                bool allVoted = true;

                for (int id = 0; id < MaxPlayers; ++id)
                {
                    if (
                        players[id].active &&
                        purchaseBallots[id] < 0
                        )
                    {
                        allVoted = false;
                    }
                }

                if (
                    allVoted ||
                    purchaseVoteRemaining <= 0.f
                    )
                {
                    finishPurchaseVote();
                }
            }


            // 전원 준비 확인
            bool allReady = true;

            for (int id = 0; id < MaxPlayers; ++id)
            {
                if (
                    players[id].active &&
                    !shopReady[id]
                    )
                {
                    allReady = false;
                }
            }


            // 상점 종료
            if (
                shopRemaining <= 0.f ||
                (
                    allReady &&
                    !purchaseVoteActive
                    )
                )
            {
                closeShopAndStartNextStage();
            }
        }

        void closeShopAndStartNextStage()
        {
            // 바다 3스테이지 이후에는 섬 1스테이지로 전환.
            // 그 외에는 현재 테마 안에서 다음 스테이지로 이동합니다.
            advanceThemeStage();

            wave = 1;

            phase = Phase::Play;

            // 일반 웨이브 30초
            time = WaveSeconds;

            spawn = 3.f;
            spawnGroup = 0;

            // 이전 전투 상태 정리
            enemies.clear();
            shots.clear();
            hazards.clear();
            drops.clear();
            effects.clear();

            sailors.clear();
            tentacles.clear();
            greatWave = {};

            boss = Boss{};

            // 대포 초기화
            for (auto& c : cannons)
            {
                c.ammo = -1;
                c.owner = -1;
                c.loader = -1;
                c.progress = 0.f;
            }

            // 플레이어 위치/상태 초기화
            for (int i = 0; i < MaxPlayers; ++i)
            {
                auto& p = players[i];

                p.mount = -1;
                p.held = -1;
                p.vote = -1;

                crew[i] = CrewCombat{};
                p.fishing = false;
                p.fishProgress = 0.f;
                p.fishCancelArmed = false;
                p.fishCatchFx = 0.f;
                p.fishCatchAmount = 0;

                if (p.active)
                {
                    p.p = {
                        178.f + i * 15.f,
                        218.f
                    };
                }
            }

            // 상점 상태 종료
            shopRemaining = 0.f;
            shopReady.fill(false);

            purchaseVoteActive = false;
            purchaseVoteItem = -1;
            purchaseVoteProposer = -1;
            purchaseVoteRemaining = 0.f;
            purchaseBallots.fill(-1);
        }

        float enemyDamage(
            const Enemy& enemy,
            float rawDamage) const
        {
            // 방어력만큼 직접 피해 감소
            // 아무리 방어력이 높아도 최소 1 피해
            return std::max(
                1.f,
                rawDamage - enemy.defense
            );
        }

        void damageEnemy(
            Enemy& enemy,
            float rawDamage)
        {
            enemy.hp -= enemyDamage(enemy, rawDamage);
        }
        int ammoCount(int type) const
        {
            return ammo.getAmmoCount(
                itemAmmo(type)
            );
        }
        int count() const
        {
            return static_cast<int>(
                std::count_if(players.begin(), players.end(), [](const Player& p) { return p.active; }));
        }
        int difficulty() const
        {
            // 1~5인 각각 독립 난이도. 인원이 많을수록 체력/피해/스폰 압박이 단계적으로 증가합니다.
            return std::clamp(count(), 1, MaxPlayers);
        }
        float difficultyHealthScale() const
        {
            static const float scale[MaxPlayers] = { 0.72f, 0.90f, 1.00f, 1.15f, 1.30f };
            return scale[difficulty() - 1];
        }

        float difficultyDamageScale() const
        {
            static const float scale[MaxPlayers] = { 0.72f, 0.90f, 1.00f, 1.12f, 1.25f };
            return scale[difficulty() - 1];
        }

        float difficultySpawnIntervalScale() const
        {
            static const float scale[MaxPlayers] = { 1.35f, 1.15f, 1.00f, 0.86f, 0.74f };
            return scale[difficulty() - 1];
        }

        // 전체 6구간 난이도 진행
        // 바다  : Stage 1 -> 난이도 1, Stage 2 -> 2, Stage 3 -> 3
        // 섬    : Stage 1 -> 난이도 2, Stage 2 -> 3, Stage 3 -> 4
        // 기존 난이도 1~3의 인원수 배율은 위 함수를 그대로 사용하고,
        // 난이도 4에서만 추가 보정을 적용합니다.
        int stageDifficultyLevel() const
        {
            if (theme == Theme::Sea)
                return std::clamp(stage, 1, 3);

            return std::clamp(stage + 1, 2, 4);
        }

        float stageHealthScale() const
        {
            return stageDifficultyLevel() == 4 ? 1.18f : 1.00f;
        }

        float stageDamageScale() const
        {
            return stageDifficultyLevel() == 4 ? 1.15f : 1.00f;
        }

        float stageBossCooldownScale() const
        {
            return stageDifficultyLevel() == 4 ? 0.85f : 1.00f;
        }
        float random()
        {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            return (rng % 100000) / 100000.f;
        }
        int nearCannon(const Player& p) const
        {
            int n = -1;
            float best = 25;
            for (int i = 0; i < static_cast<int>(cannons.size()); ++i)
                if (float d = dist(p.p, cannons[i].p); d < best)
                {
                    best = d;
                    n = i;
                }
            return n;
        }
        int nearHazard(const Player& p) const
        {
            int n = -1;
            float best = 20;
            for (int i = 0; i < static_cast<int>(hazards.size()); ++i)
                if (float d = dist(p.p, hazards[i].p); d < best)
                {
                    best = d;
                    n = i;
                }
            return n;
        }
        void addPlayer(int id)
        {
            players[id] = Player{};
            players[id].active = true;
            players[id].p = { 178.f + id * 15.f, 218.f };

            crew[id] = CrewCombat{};
            crew[id].hp = 100.f;
            crew[id].maxHp = 100.f;

            players[id].fishing = false;
            players[id].fishProgress = 0.f;
            players[id].fishCancelArmed = false;
            players[id].fishCatchFx = 0.f;
            players[id].fishCatchAmount = 0;
        }
        void leave(int id)
        {
            auto& p = players[id];
            if (p.held >= 0 && drops.size() < 32)
                drops.push_back({ p.p, p.held });
            for (auto& c : cannons)
            {
                if (c.owner == id)
                    c.owner = -1;
                if (c.loader == id)
                {
                    c.loader = -1;
                    c.progress = 0;
                }
            }
            p = Player{};
        }
        void start()
        {
            auto old = players;
            *this = Game{};
            for (int i = 0; i < 5; ++i)
            {
                players[i].active = old[i].active;
                players[i].p = { 178.f + i * 15.f, 218.f };
                players[i].fishing = false;
                players[i].fishProgress = 0.f;
                players[i].fishCancelArmed = false;
                players[i].fishCatchFx = 0.f;
                players[i].fishCatchAmount = 0;

                crew[i] = CrewCombat{};
                crew[i].hp = 100.f;
                crew[i].maxHp = 100.f;
            }
            phase = Phase::Play;
        }
        void effect(V p, int kind, float life = .45f)
        {
            if (effects.size() < 100)
                effects.push_back({ p, life, life, kind });
        }
        void shipHit(V p, float amount, bool fire, bool deckImpact = false)
        {
            hp = std::max(0.f, hp - amount);
            ++impacts;
            effect(p, 1);
            // 촉수처럼 갑판을 직접 찍는 공격은 실제 타격점을 그대로 사용합니다.
            // 일반 포탄/파도는 오른쪽에만 파손이 몰리지 않도록 갑판 전체의 안전 지점으로 분산합니다.
            V site{};
            if (deckImpact)
            {
                site = {
                    std::clamp(p.x, DeckLeft + 8.f, DeckRight - 8.f),
                    std::clamp(p.y, DeckTop + 8.f, DeckBottom - 8.f)
                };
            }
            else
            {
                static const V repairSites[] =
                {
                    { 118.f, 92.f }, { 205.f, 88.f }, { 292.f, 108.f },
                    { 125.f, 145.f }, { 278.f, 174.f },
                    { 118.f, 252.f }, { 205.f, 268.f }, { 292.f, 246.f }
                };

                const int siteCount =
                    static_cast<int>(sizeof(repairSites) / sizeof(repairSites[0]));
                const int firstSlot =
                    static_cast<int>((impacts + static_cast<std::uint32_t>(std::abs(p.y))) %
                        siteCount);

                site = repairSites[firstSlot];

                // 이미 고장난 위치와 겹치면 다음 좌/중/우 지점을 순회해 빈 곳을 찾습니다.
                for (int offset = 0; offset < siteCount; ++offset)
                {
                    const V candidate = repairSites[(firstSlot + offset) % siteCount];
                    const bool occupied = std::any_of(
                        hazards.begin(), hazards.end(),
                        [&](const Hazard& h) { return dist(h.p, candidate) < 22; });

                    if (!occupied)
                    {
                        site = candidate;
                        break;
                    }
                }
            }
            if (hazards.size() < 10 && std::none_of(hazards.begin(), hazards.end(),
                [&](const Hazard& h) { return dist(h.p, site) < 22; }))
                hazards.push_back({ site, fire });
        }
        void spawnEnemyGroup()
        {
            ++spawnGroup;

            int population = std::clamp(count(), 1, MaxPlayers);

            // 1인 1척부터 5인 5척까지 단계적으로 증가.
            // 싱글은 기존보다 확실히 가볍게 시작하도록 동시 적 수를 낮춥니다.
            int spawnCount = population;
            int cap = 2 + population;

            int present =
                static_cast<int>(
                    std::count_if(
                        enemies.begin(),
                        enemies.end(),
                        [](const Enemy& e)
                        {
                            return
                                e.side == 1 &&
                                e.hp > 0;
                        }
                    )
                    );

            for (
                int j = 0;
                j < spawnCount &&
                present < cap;
                ++j
                )
            {
                const int stageLevel = stageDifficultyLevel();

                int type =
                    stageLevel == 1
                    ? (
                        spawnGroup % 4 == 0
                        ? 1
                        : 0
                        )
                    : static_cast<int>(
                        random() * 3
                        );

                float health =
                    (type == 2
                        ? 110.f
                        : type == 1
                        ? 40.f
                        : 55.f)
                    * difficultyHealthScale()
                    * stageHealthScale();

                float defense =
                    type == 2
                    ? 10.f
                    : type == 1
                    ? 0.f
                    : 4.f;

                static constexpr std::array<float, 8> enemyLanes = {
                    78.f, 110.f, 142.f, 174.f,
                    206.f, 238.f, 270.f, 302.f
                };

                float spawnY = enemyLanes[0];
                float bestClearance = -1.f;

                for (const float laneY : enemyLanes)
                {
                    float nearest = 9999.f;

                    for (const auto& other : enemies)
                    {
                        if (other.side != 1 || other.hp <= 0.f)
                            continue;

                        nearest = std::min(
                            nearest,
                            std::abs(other.p.y - laneY)
                        );
                    }
                    if (nearest > bestClearance)
                    {
                        bestClearance = nearest;
                        spawnY = laneY;
                    }
                }

                enemies.push_back({
                    nextId++,
                    type,
                    1,
                    {
                        660.f,
                        spawnY
                    },
                    health,
                    health,
                    4.f +
                    random() * 3.f
                    });

                enemies.back().defense =
                    defense;

                ++present;
            }

            const int stageLevel = stageDifficultyLevel();

            float baseSpawn =
                13.f -
                stageLevel * 1.8f -
                (wave - 1) * 0.8f;

            spawn = baseSpawn * difficultySpawnIntervalScale();
        }

        void startBoss()
        {
            // 밤/보스전으로 전환되어도 낮·저녁에 살아 있던 잡몹과 탄환은 유지
            // 기존 적 위에 해당 스테이지의 메인 보스가 추가로 등장
            boss = Boss{};

            // 보스전 특수 오브젝트 초기화
            sailors.clear();
            tentacles.clear();
            greatWave = {};

            deckSpawn = 3.f;

            // 플레이어 전투 상태 초기화
            for (auto& c : crew)
            {
                c = CrewCombat{};
            }

            boss.active = true;
            boss.side = 1;
            boss.p = { 565.f, 180.f };

            // 현재 스테이지에 맞는 보스 종류
            boss.type = bossTypeForStage();

            // 난이도 단계별 보스 체력.
            // 난이도 1~3은 기존 300 / 500 / 800을 그대로 사용
            // 새 난이도 4만 1100으로 추가
            static constexpr float bossHpByLevel[4] =
            {
                300.f, 500.f, 800.f, 1100.f
            };

            const int stageLevel = stageDifficultyLevel();
            boss.hp =
                bossHpByLevel[stageLevel - 1] *
                difficultyHealthScale();
            boss.maxHp = boss.hp;

            // 난이도 4에서만 보스 공격 주기를 약 15% 빠르게
            boss.cooldown = 6.f * stageBossCooldownScale();
            boss.specialCooldown = 6.f * stageBossCooldownScale();

            effect(boss.p, 3, 1.5f);
        }

        void chooseReward(int option)
        {
            if (phase != Phase::Reward || option < 0 || option > 2)
                return;

            // 보상 적용
            if (option == 0)
            {
                maxHp += 150.f;
                hp = std::min(maxHp, hp + 250.f);
            }

            if (option == 1)
            {
                repairBonus += 25.f;
                hp = std::min(maxHp, hp + 160.f);
            }

            if (option == 2)
            {
                leakScale *= 0.65f;
                hp = std::min(maxHp, hp + 160.f);
            }

            reward = option;

            // 유동호 병합: 다음 라운드 진입 전 전원 완전 부활
            reviveAllPlayers(1.0f);

            // 이전 전투 상태 정리
            enemies.clear();
            shots.clear();
            hazards.clear();
            effects.clear();
            drops.clear();

            for (auto& c : cannons)
            {
                c.ammo = -1;
                c.owner = -1;
                c.loader = -1;
                c.progress = 0.f;
            }

            for (auto& p : players)
            {
                p.mount = -1;
                p.held = -1;
                p.vote = -1;

                p.p = { 320.f, 218.f };
            }

            // 다음 Wave / Stage 결정
            if (wave < MaxWave)
            {
                // 같은 스테이지의 다음 웨이브
                ++wave;
            }
            else
            {
                // Wave 3 보스까지 끝났으면 다음 스테이지/테마로 이동
                advanceThemeStage();

                wave = 1;
            }

            // 다음 전투 준비
            phase = Phase::Play;

            time = WaveSeconds;

            spawn = 3.f;
            spawnGroup = 0;

            boss = Boss{};


            // Wave 3은 일반 웨이브가 아니라 보스전
            if (wave == MaxWave)
            {
                startBoss();
            }
        }
        void releaseCannon(int id)
        {
            for (auto& c : cannons)
            {
                if (c.owner == id)
                    c.owner = -1;

                if (c.loader == id)
                {
                    c.loader = -1;
                    c.progress = 0.f;
                }
            }

            players[id].mount = -1;
        }

        bool hurtCrew(
            int id,
            float damage,
            V push = { 0.f, 0.f })
        {
            auto& c = crew[id];
            auto& p = players[id];

            // 비활성 플레이어
            if (!p.active)
                return false;

            // 이미 쓰러진 플레이어는 추가 피격 없음
            if (c.hp <= 0.f)
                return false;

            // 피격 무적시간 중
            if (c.invulnerable > 0.f)
                return false;


            // 개인 HP 감소
            c.hp = std::max(
                0.f,
                c.hp - damage
            );

            // 연속 피격 방지
            c.invulnerable = 0.8f;


            // 대포 사용 중이었다면 해제
            releaseCannon(id);


            // 밀려나는 효과
            p.p += push;

            // 배 바깥으로 밀려나지 않도록 제한
            p.p.x = std::clamp(
                p.p.x,
                DeckLeft + 10.f,
                DeckRight - 10.f
            );

            p.p.y = std::clamp(
                p.p.y,
                DeckTop + 10.f,
                DeckBottom - 10.f
            );


            effect(
                p.p,
                1,
                0.35f
            );



            // HP 0 → 사망

            if (c.hp <= 0.f)
            {
                c.hp = 0.f;

                //자동 부활하지 않고 다음 보상/스테이지까지 DOWN 유지
                c.revive = 0.f;
                c.bracing = false;


                // 들고 있던 포탄이 있으면 바닥에 떨어뜨림
                if (
                    p.held >= 0 &&
                    drops.size() < 32
                    )
                {
                    drops.push_back({
                        p.p,
                        p.held
                        });
                }

                p.held = -1;
            }

            return true;
        }

        void fireCannon(Cannon& cannon)
        {
            const int type = cannon.ammo;

            // 포탄 종류별 기본 속도 / 데미지
            const auto stats = shellStats(type);

            // 일반탄만 공격력 강화 적용
            float damage = stats.damage;

            if (type == Normal)
            {
                damage *= upgrades.getNormalDamageMultiplier();
            }

            // 확산탄만 5발 발사
            const int shotCount =
                type == Spread ? 5 : 1;

            for (int i = 0; i < shotCount; ++i)
            {
                // 확산탄:
                // -20도 / -10도 / 0도 / +10도 / +20도
                float angle =
                    cannon.angle +
                    (
                        type == Spread
                        ? (i - 2) * Pi / 18.f
                        : 0.f
                        );

                V velocity{
                    std::cos(angle) * cannon.side * stats.speed,
                    std::sin(angle) * stats.speed
                };

                shots.push_back({
                    cannon.p + unit(velocity) * 16.f,
                    velocity,
                    type,
                    false,
                    damage,
                    4.f,
                    {}
                    });
            }

            // 장전된 포탄 소모
            cannon.ammo = -1;

            ++shotsFired;

            effect(
                cannon.p,
                0,
                0.15f
            );
        }

        void meleeAttack(int id)
        {
            auto& c = crew[id];

            // 죽어 있거나 쿨타임 중이면 공격 불가
            if (c.hp <= 0.f || c.attackCooldown > 0.f)
                return;

            // 대포 사용 중이었다면 해제
            releaseCannon(id);

            // 공격 쿨타임 0.4초
            c.attackCooldown = 0.4f;

            // 나중에 Render에서 칼 휘두르는 연출용
            c.swing = 0.18f;

            V p = players[id].p;


            // 유령 선원 공격
            for (auto& e : sailors)
            {
                // 아직 배에 올라오는 중이면 공격 불가
                if (e.boarding > 0.f)
                    continue;

                if (e.hp <= 0.f)
                    continue;

                // 플레이어와 42픽셀 이내
                if (dist(p, e.p) <= 42.f)
                {
                    e.hp = std::max(
                        0.f,
                        e.hp - 30.f
                    );

                    effect(e.p, 2, 0.2f);
                }
            }


            // 크라켄 촉수 공격
            for (auto& t : tentacles)
            {
                // 등장 경고 중에는 공격 불가
                if (t.warning > 0.f)
                    continue;

                if (t.hp <= 0.f)
                    continue;

                // 플레이어와 45픽셀 이내
                if (dist(p, t.p) <= 45.f)
                {
                    t.hp = std::max(
                        0.f,
                        t.hp - 30.f
                    );

                    effect(t.p, 2, 0.2f);

                    // 촉수 파괴 시 보스에게 추가 피해
                    if (t.hp == 0.f)
                    {
                        boss.hp = std::max(
                            0.f,
                            boss.hp - 20.f
                        );
                    }
                }
            }
        }
        void updateCrew(
            float dt,
            const std::array<Input, MaxPlayers>& inputs)
        {
            for (int i = 0; i < MaxPlayers; ++i)
            {
                auto& c = crew[i];

                if (!players[i].active)
                    continue;


                // 각종 타이머 감소
                c.invulnerable =
                    std::max(0.f, c.invulnerable - dt);

                c.attackCooldown =
                    std::max(0.f, c.attackCooldown - dt);

                c.swing =
                    std::max(0.f, c.swing - dt);


                // 기본적으로 가드 해제
                c.bracing = false;

                //쓰러진 플레이어는 다음 부활 처리 전까지 전투 조작 불가
                if (c.hp <= 0.f)
                {
                    c.bracing = false;
                    continue;
                }


                // 보스전이 아니면 근접 전투 기능 사용 안 함
                if (!boss.active || boss.hp <= 0.f)
                    continue;


                // 레비아탄 / 크라켄에서 C 홀드 = 파도 가드
                c.bracing =
                    (
                        boss.type == BossType::Leviathan ||
                        boss.type == BossType::Kraken
                        )
                    &&
                    inputs[i].brace;


                // C 누른 순간 = 근접공격
                // 크라켄은 C 홀드 상태여도
                // 촉수 공격이 가능하도록 처리
                if (
                    inputs[i].melee &&
                    (
                        !c.bracing ||
                        boss.type == BossType::Kraken
                        )
                    )
                {
                    meleeAttack(i);
                }


                // 가드 중이면 대포 사용 해제
                if (c.bracing)
                {
                    releaseCannon(i);
                }
            }
        }

        static float segmentDistance(V p, V a, V b) {
            V d = b - a;
            float q = d.x * d.x + d.y * d.y;
            float t = q > 0 ? std::clamp(((p.x - a.x) * d.x + (p.y - a.y) * d.y) / q, 0.f, 1.f) : 0;
            return dist(p, a + d * t);
        }
        V occupiedPosition(int offset) const {
            int n = 0;
            for (int i = 0; i < MaxPlayers; ++i) if (players[i].active && crew[i].hp > 0) ++n;
            int pick = offset % std::max(1, n);
            for (int i = 0; i < MaxPlayers; ++i) if (players[i].active && crew[i].hp > 0 && pick-- == 0)
                return players[i].p;
            return { 220,180 };
        }
        void addHostileShot(Shot shot) {
            shot.black = shot.hostile && random() < .3f;
            shots.push_back(shot);
        }
        void updateBossBarrage(float dt) {
            if (!boss.active || boss.hp <= 0) return;
            boss.cooldown -= dt * (boss.freeze > 0 ? .6f : 1.f);
            boss.warning = boss.cooldown > 0 && boss.cooldown <= 1.f ? boss.cooldown : 0.f;
            if (boss.cooldown > 0) return;
            boss.cooldown += 6.f * stageBossCooldownScale();
            auto fire = [&](V origin, V target, float speed, int shell, float damage) {
                addHostileShot({ origin,unit(target - origin) * speed,shell,true,damage * difficultyDamageScale() * stageDamageScale(),8,{} });
                effect(origin, 0, .2f);
                };
            if (boss.type == BossType::GhostShip) {
                for (int i = 0; i < 3; ++i) {
                    V muzzle = boss.p + V{ -48.f,(i - 1) * 24.f };
                    fire(muzzle, { ShipHitRight - 4,muzzle.y }, 105, Normal, 12);
                }
            }
            else if (boss.type == BossType::Leviathan) {
                for (int i = 0; i < 5; ++i)
                    fire(boss.p + V{ -30,0 }, { ShipHitRight - 4,75.f + i * 55.f }, 80, Normal, 9);
            }
            else {
                for (int i = 0; i < 3; ++i) {
                    fire(boss.p + V{ -25,-24 }, { ShipHitRight - 4,195.f + i * 45.f }, 90, Pierce, 8);
                    fire(boss.p + V{ -25,24 }, { ShipHitRight - 4,75.f + i * 45.f }, 90, Pierce, 8);
                }
            }
        }
        void updateSailors(float dt) {
            if (!boss.active || boss.hp <= 0 || boss.type != BossType::GhostShip) return;
            deckSpawn -= dt;
            if (deckSpawn <= 0) {
                int number = (boss.maxHp > 0 && boss.hp <= boss.maxHp * .5f) ? 3 : 2;
                for (int i = 0; i < number && sailors.size() < 4; ++i) {
                    GhostSailor e;
                    e.p = { DeckRight - 18.f,95.f + random() * 185.f };
                    sailors.push_back(e);
                }
                deckSpawn = (boss.maxHp > 0 && boss.hp <= boss.maxHp * .5f) ? 9.f : 12.f;
            }
            for (auto& e : sailors) {
                if (e.hp <= 0) continue;
                if (e.boarding > 0) { e.boarding = std::max(0.f, e.boarding - dt); continue; }
                int target = -1; float nearest = 10000;
                for (int i = 0; i < MaxPlayers; ++i) if (players[i].active && crew[i].hp > 0) {
                    float d = dist(e.p, players[i].p);
                    if (d < nearest) { nearest = d; target = i; }
                }
                if (e.windup > 0) {
                    e.windup = std::max(0.f, e.windup - dt);
                    if (e.windup == 0) {
                        for (int i = 0; i < MaxPlayers; ++i) if (dist(players[i].p, e.strike) < 23.f)
                            hurtCrew(i, 15.f * difficultyDamageScale() * stageDamageScale(), unit(players[i].p - e.p) * 12.f);
                        effect(e.strike, 1, .2f); e.cooldown = 1.2f;
                    }
                    continue;
                }
                if (target < 0) continue;
                e.cooldown = std::max(0.f, e.cooldown - dt);
                if (nearest > 21.f) e.p += unit(players[target].p - e.p) * std::min(nearest - 21.f, dt * 38.f);
                else if (e.cooldown == 0) {
                    e.strike = players[target].p;
                    e.windup = .65f;
                }
            }
            sailors.erase(std::remove_if(sailors.begin(), sailors.end(), [](const GhostSailor& e) {return e.hp <= 0; }), sailors.end());
        }

        void updateGreatWave(float dt) {
            if (!boss.active || boss.hp <= 0 || (boss.type != BossType::Leviathan && boss.type != BossType::Kraken)) return;
            boss.specialCooldown -= dt;
            if (boss.specialCooldown <= 0) {
                boss.specialCooldown += 6.f * stageBossCooldownScale();
                greatWave = {};
                greatWave.active = true;
            }
            else if (!greatWave.active && boss.specialCooldown <= 1.f) {
                greatWave = {};
                greatWave.active = true;
                greatWave.warning = boss.specialCooldown;
            }
            if (!greatWave.active) return;
            if (greatWave.warning > 0) {
                greatWave.warning = boss.specialCooldown;
                return;
            }
            if (!greatWave.impacting) {
                greatWave.impacting = true;
                shipHit({ 320,180 }, 20.f * difficultyDamageScale() * stageDamageScale(), false); // Bracing protects the crew, not the hull.
            }
            const float previous = greatWave.front;
            greatWave.front -= 180.f * dt;
            for (int i = 0; i < MaxPlayers; ++i) {
                if (!players[i].active || crew[i].hp <= 0 || (greatWave.hitPlayers & (1u << i))) continue;
                float x = players[i].p.x;
                if (x >= greatWave.front - 16 && x <= previous + 16) {
                    greatWave.hitPlayers |= (1u << i);
                    if (!crew[i].bracing) hurtCrew(i, 35.f * difficultyDamageScale() * stageDamageScale(), { -28,0 });
                    else effect(players[i].p, 2, .3f);
                }
            }
            if (greatWave.front < DeckLeft - 40) greatWave.active = false;
        }

        void updateTentacles(float dt) {
            if (!boss.active || boss.hp <= 0 || boss.type != BossType::Kraken) return;
            deckSpawn -= dt;
            if (deckSpawn <= 0) {
                const int number = (boss.maxHp > 0 && boss.hp <= boss.maxHp * .5f) ? 2 : 1;
                for (int i = 0; i < number && tentacles.size() < 6; ++i) {
                    V target = occupiedPosition(i);
                    if (i) target.y += 55;
                    target.x = std::clamp(target.x, DeckLeft + 25, DeckRight - 25);
                    target.y = std::clamp(target.y, DeckTop + 30, DeckBottom - 25);
                    bool clear = true;
                    for (const auto& t : tentacles) if (dist(t.p, target) < 40) clear = false;
                    if (!clear) {
                        target = { DeckLeft + 25.f + random() * (DeckRight - DeckLeft - 50.f),90.f + random() * 200.f };
                        clear = true;
                        for (const auto& t : tentacles) if (dist(t.p, target) < 40) clear = false;
                    }
                    if (clear) { DeckTentacle t; t.p = target; tentacles.push_back(t); }
                }
                deckSpawn =
                    ((boss.maxHp > 0 && boss.hp <= boss.maxHp * .5f) ? 8.f : 10.f) *
                    stageBossCooldownScale();
            }
            for (auto& t : tentacles) {
                if (t.hp <= 0) continue;
                t.flash = std::max(0.f, t.flash - dt);
                t.slamAnimation = std::max(0.f, t.slamAnimation - dt);
                if (t.warning > 0) {
                    t.warning = std::max(0.f, t.warning - dt);
                    if (t.warning == 0) {
                        shipHit(t.p, 10.f * difficultyDamageScale() * stageDamageScale(), false, true); t.flash = .35f;
                        for (int i = 0; i < MaxPlayers; ++i) if (dist(players[i].p, t.p) < 29.f)
                            hurtCrew(i, 25.f * difficultyDamageScale() * stageDamageScale(), unit(players[i].p - t.p) * 16.f);
                    }
                    continue;
                }
                t.slamTimer -= dt;
                if (t.slamTimer <= 0) {
                    shipHit(t.slamPoint(), 18.f * difficultyDamageScale() * stageDamageScale(), false, true); t.flash = .35f;
                    t.slamAnimation = .45f;
                    t.slamTimer += 5.f * stageBossCooldownScale();
                }
            }
            tentacles.erase(std::remove_if(tentacles.begin(), tentacles.end(), [](const DeckTentacle& t) {return t.hp <= 0; }), tentacles.end());
        }

        void update(float dt, const std::array<Input, 5>& inputs)
        {
            if (phase == Phase::Reward)
            {
                int voted = 0;
                std::array<int, 3> votes{};
                for (int i = 0; i < 5; ++i)
                    if (players[i].active)
                    {
                        if (inputs[i].vote >= 0 && inputs[i].vote < 3)
                            players[i].vote = inputs[i].vote;
                        if (players[i].vote >= 0)
                        {
                            ++voted;
                            ++votes[players[i].vote];
                        }
                    }
                if (voted == count() && voted > 0)
                    chooseReward(static_cast<int>(std::max_element(votes.begin(), votes.end()) - votes.begin()));
                return;
            }

            // 상점 업데이트
            if (phase == Phase::Shop)
            {
                updateShop(dt, inputs);
                return;
            }

            if (phase != Phase::Play)
                return;

            age += dt;

            updateCrew(dt, inputs);

            // Stage 1 보스 - 유령 선원
            updateSailors(dt);

            // Stage 2 보스 - 레비아탄 큰 파도
            updateGreatWave(dt);

            // Stage 3 - 크라켄 촉수
            updateTentacles(dt);

            std::array<bool, CannonCount> loading{};
            std::vector<bool> repairing(hazards.size(), false);

            for (int id = 0; id < 5; ++id)
            {
                auto& p = players[id];
                if (!p.active)
                    continue;

                auto& c = crew[id];

                if (p.fishCatchFx > 0.f)
                {
                    p.fishCatchFx = std::max(0.f, p.fishCatchFx - dt * 1.45f);

                    if (p.fishCatchFx <= 0.f)
                        p.fishCatchAmount = 0;
                }

                // 쓰러진 플레이어는 이동/대포/수리/낚시 조작 불가
                if (c.hp <= 0.f)
                {
                    p.fishing = false;
                    p.fishProgress = 0.f;
                    p.fishCancelArmed = false;
                    continue;
                }

                const auto& in = inputs[id];

                // H키: 물고기 1마리 섭취 후 체력 회복
                if (in.useFish)
                    useFishToHeal(id, 35.f);
                if (in.select >= 0 && in.select < ShellCount)
                    p.selected = in.select;

                V movement{ in.x, in.y };
                if (length(movement) > 1)
                    movement = unit(movement);
                p.p += movement * 85.f * dt;
                p.p.x = std::clamp(p.p.x, DeckLeft + 10.f, DeckRight - 10.f);
                p.p.y = std::clamp(p.p.y, 58.f, 322.f);

                // 낚시 진행/취소
                bool isMoving =
                    (in.x != 0.f || in.y != 0.f);

                if (p.fishing)
                {
                    // 시작에 사용된 E 입력이 네트워크 지연으로 한 번 더 들어와도
                    // 바로 취소되지 않도록, E가 완전히 떼어진 프레임을 확인한 뒤
                    // 다음 E 입력부터 취소 입력으로 인정
                    if (!p.fishCancelArmed)
                    {
                        if (!in.hold && !in.tap)
                            p.fishCancelArmed = true;
                    }

                    if (isMoving ||
                        (p.fishCancelArmed && in.tap))
                    {
                        p.fishing = false;
                        p.fishProgress = 0.f;
                        p.fishCancelArmed = false;
                    }
                    else
                    {
                        // 기본 2.5초, 낚시 강화 배율 적용
                        p.fishProgress +=
                            dt * 0.4f *
                            upgrades.getFishingSpeedMultiplier();

                        if (p.fishProgress >= 1.f)
                        {
                            addFish(1);
                            p.fishing = false;
                            p.fishProgress = 0.f;
                            p.fishCancelArmed = false;
                            p.fishCatchFx = 1.f;
                            p.fishCatchAmount = 1;
                            effect(FishingPoint, 2, 0.30f);
                            effect(p.p, 2, 0.4f);
                        }

                        // 낚시 중에는 다른 행동 차단
                        continue;
                    }
                }

                if (in.drop && p.held >= 0 && drops.size() < 32)
                {
                    drops.push_back({ p.p, p.held });
                    p.held = -1;
                }

                if (
                    in.tap &&
                    p.held < 0 &&
                    dist(p.p, FishingPoint) < FishingInteractRadius
                    )
                {
                    p.fishing = true;
                    p.fishProgress = 0.f;
                    p.fishCancelArmed = false;
                    continue;
                }

                int h = nearHazard(p), n = nearCannon(p);
                if (h >= 0 && in.hold)
                {
                    repairing[h] = true;
                    continue;
                }
                if (n >= 0)
                {
                    auto& c = cannons[n];

                    c.angle = std::clamp(
                        std::atan2(
                            in.aim.y - c.p.y,
                            std::max(
                                8.f,
                                (in.aim.x - c.p.x) * c.side
                            )
                        ),
                        -1.02f,
                        1.02f
                    );

                    if (
                        c.ammo >= 0 &&
                        in.fire
                        )
                    {
                        fireCannon(c);
                        continue;
                    }

                    if (
                        p.held >= 0 &&
                        c.ammo < 0 &&
                        in.hold &&
                        (c.loader < 0 || c.loader == id)
                        )
                    {
                        c.loader = id;
                        loading[n] = true;

                        c.progress +=
                            dt / upgrades.getReloadMultiplier();

                        if (c.progress >= 1.f)
                        {
                            c.ammo = p.held;
                            p.held = -1;
                            c.progress = 0.f;
                            c.loader = -1;

                            effect(c.p, 4, 0.28f);
                        }
                    }
                }
                else if (in.tap && p.held < 0)
                {
                    auto it = std::find_if(drops.begin(), drops.end(),
                        [&](const Drop& d) { return dist(p.p, d.p) < 18; });
                    if (it != drops.end())
                    {
                        p.held = it->type;
                        drops.erase(it);
                    }
                    else if (
                        dist(p.p, AmmoPoint) < 32.f &&
                        ammo.useAmmo(
                            itemAmmo(p.selected)
                        )
                        )
                    {
                        p.held = p.selected;
                    }
                }
            }
            for (int i = 0; i < static_cast<int>(cannons.size()); ++i)
                if (!loading[i])
                {
                    cannons[i].progress = 0;
                    cannons[i].loader = -1;
                }
            for (int i = static_cast<int>(hazards.size()) - 1; i >= 0; --i)
            {
                auto& h = hazards[i];
                h.age += dt;
                hp -= dt * (h.fire ? 2.6f : 1.8f) *
                    (1 + std::min(h.age / 30.f, 1.f)) *
                    leakScale *
                    (stageDifficultyLevel() == 4 ? 1.10f : 1.00f);
                if (repairing[i])
                    h.progress +=
                    dt / upgrades.getRepairMultiplier();
                else
                    h.progress = 0;
                if (h.progress >= (h.fire ? 1.5f : 2.f))
                {
                    hp = std::min(maxHp, hp + 55 + repairBonus);

                    effect(h.p, 2);
                    hazards.erase(hazards.begin() + i);
                }
            }
            if (!boss.active)
            {
                spawn -= dt;
                if (spawn <= 0)
                    spawnEnemyGroup();
            }
            for (auto& e : enemies)
            {
                e.age += dt;
                if (e.burn > 0)
                {
                    e.hp -= 12 * dt;
                    e.burn -= dt;
                }
                e.freeze = std::max(0.f, e.freeze - dt);
                if (e.hp <= 0)
                    continue;
                float target =
                    e.type == 1
                    ? 382.f
                    : 404.f;

                float speed =
                    e.type == 1
                    ? 32.f
                    : e.type == 2
                    ? 12.f
                    : 18.f;

                if (e.freeze > 0)
                    speed *= .35f;

                if (std::abs(e.p.x - target) > 2.f)
                {
                    // 오른쪽에서 왼쪽으로 접근
                    e.p.x -= speed * dt;
                }
                else
                {
                    e.cooldown -= dt * (e.freeze > 0 ? .4f : 1.f);

                    if (e.cooldown <= 0)
                    {
                        V destination{
                            DeckRight - 6.f,
                            std::clamp(
                                e.p.y + (random() - .5f) * 50.f,
                                DeckTop + 4.f,
                                DeckBottom - 7.f
                            )
                        };
                        shots.push_back({ e.p,
                                         unit(destination - e.p) * (e.type == 1 ? 95.f : 75.f),
                                         e.type == 2 ? Flame : Normal,
                                         true,
                                         (e.type == 2 ? 24.f : 16.f) * stageDamageScale(),
                                         8,
                                         {} });

                        shots.back().black = random() < 0.30f;

                        e.cooldown = e.type == 1 ? 7.f : e.type == 2 ? 10.f : 9.f;
                    }
                }
            }
            if (boss.active && boss.hp > 0)
            {
                if (boss.burn > 0) {
                    boss.hp -= 12.f * std::min(dt, boss.burn);
                    boss.burn = std::max(0.f, boss.burn - dt);
                }
                boss.freeze = std::max(0.f, boss.freeze - dt);
                boss.p.y = 180 + std::sin(age * .6f) * 45;
                updateBossBarrage(dt);
            }
            for (auto& s : shots)
            {
                const V previous = s.p;
                s.life -= dt;
                s.p += s.v * dt;
                if (s.life <= 0)
                    continue;
                if (s.hostile)
                {
                    // 검은 탄막
                    // 배를 통과하고 플레이어를 직접 공격
                    if (s.black)
                    {
                        for (int i = 0; i < MaxPlayers; ++i) {
                            if (!players[i].active || crew[i].hp <= 0 ||
                                std::find(s.hit.begin(), s.hit.end(), i) != s.hit.end()) continue;
                            if (segmentDistance(players[i].p, previous, s.p) <= 12.f) {
                                s.hit.push_back(i);
                                hurtCrew(i, s.damage, unit(s.v) * 6.f);
                            }
                        }
                    }

                    // 일반 적 탄환
                    // 기존처럼 배를 공격
                    else
                    {
                        if (
                            s.p.x >= ShipHitLeft &&
                            s.p.x <= ShipHitRight &&
                            s.p.y > 48.f &&
                            s.p.y < 329.f
                            )
                        {
                            shipHit(
                                s.p,
                                s.damage,
                                s.type == Flame ||
                                random() < .2f
                            );

                            s.life = 0.f;
                        }
                    }
                }
                else
                {
                    for (auto& e : enemies)
                        if (e.hp > 0 && dist(s.p, e.p) < (e.type == 2 ? 21 : 17) &&
                            std::find(s.hit.begin(), s.hit.end(), e.id) == s.hit.end())
                        {
                            s.hit.push_back(e.id);
                            damageEnemy(e, s.damage);
                            effect(e.p, 1);
                            if (s.type == Flame)
                                e.burn = 4;

                            if (s.type == Blast)
                            {
                                effect(s.p, 3);
                                for (auto& other : enemies)
                                    if (other.id != e.id && dist(other.p, e.p) < 65)
                                        damageEnemy(other, 45.f);
                            }
                            if (s.type != Pierce)
                            {
                                s.life = 0;
                                break;
                            }
                        }
                    if (s.life > 0 && boss.active && segmentDistance(boss.p, previous, s.p) < (boss.type == BossType::GhostShip ? 52.f : 43.f) &&
                        std::find(s.hit.begin(), s.hit.end(), -1) == s.hit.end())
                    {
                        boss.hp -= s.damage;
                        s.hit.push_back(-1);
                        effect(s.p, 1);
                        if (s.type == Flame)
                            boss.burn = 4;

                        s.life = 0;
                    }
                }
                if (s.p.x < -40 || s.p.x > 680 || s.p.y < 35 || s.p.y > 340)
                    s.life = 0;
            }
            for (const auto& e : enemies)
                if (e.hp <= 0)
                {
                    ++kills;
                    effect(e.p, 3);
                }
            enemies.erase(
                std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return e.hp <= 0; }),
                enemies.end());
            shots.erase(std::remove_if(shots.begin(), shots.end(), [](const Shot& s) { return s.life <= 0; }),
                shots.end());
            for (auto& e : effects)
                e.life -= dt;
            effects.erase(
                std::remove_if(effects.begin(), effects.end(), [](const Effect& e) { return e.life <= 0; }),
                effects.end());
            hp = std::max(0.f, hp);
            if (hp <= 0)
            {
                phase = Phase::Lost;
                return;
            }

            // 유동호 병합: 활성 플레이어 전원이 쓰러지면 패배
            int activeCount = 0;
            int downedCount = 0;

            for (int i = 0; i < MaxPlayers; ++i)
            {
                if (!players[i].active)
                    continue;

                ++activeCount;

                if (crew[i].hp <= 0.f)
                    ++downedCount;
            }

            if (
                activeCount > 0 &&
                downedCount == activeCount
                )
            {
                phase = Phase::Lost;
                return;
            }

            if (boss.active && boss.hp <= 0)
            {
                boss.hp = 0.f;
                boss.active = false;

                sailors.clear();
                tentacles.clear();
                greatWave = {};

                // 스테이지 클리어 보상: 팀 골드 2000G
                gold.addGold(2000);

                enemies.clear();
                shots.clear();

                // 바다 Stage 3은 게임 종료가 아니라 섬 테마로 넘어가기 전 상점.
                // 섬 Stage 3 보스까지 잡아야 최종 승리합니다.
                if (theme == Theme::Island && stage == MaxStage)
                {
                    phase = Phase::Won;
                }
                else
                {
                    openShop();
                }

                return;
            }
            if (!boss.active)
            {
                time = std::max(
                    0.f,
                    time - dt
                );

                if (time <= 0.f)
                {
                    // 웨이브 전환 시 살아 있는 적을 삭제하지 않는다.
                    // 낮 -> 저녁 -> 밤으로 넘어가도 기존 적이 계속 남아 싸운다.
                    if (wave < MaxWave)
                    {
                        ++wave;

                        time = WaveSeconds;
                        spawn = 3.f;
                        spawnGroup = 0;

                        if (wave == MaxWave)
                        {
                            startBoss();
                        }
                    }

                    return;
                }
            }
        }

        void damagePlayer(int id, float amount)
        {
            if (
                id < 0 ||
                id >= MaxPlayers ||
                !players[id].active ||
                amount <= 0.f
                )
            {
                return;
            }

            if (crew[id].hp <= 0.f)
                return;

            // 기존 개인 전투 시스템에 연결하여
            // 대포 해제, 피격 이펙트, 포탄 드롭 처리를 함께 사용
            hurtCrew(id, amount);
        }

        void healPlayer(int id, float amount)
        {
            if (
                id < 0 ||
                id >= MaxPlayers ||
                !players[id].active ||
                amount <= 0.f
                )
            {
                return;
            }

            // 쓰러진 플레이어는 일반 회복으로 부활하지 않음
            if (crew[id].hp <= 0.f)
                return;

            crew[id].hp =
                std::min(
                    crew[id].maxHp,
                    crew[id].hp + amount
                );

            effect(
                players[id].p,
                2,
                0.25f
            );
        }

        void reviveAllPlayers(float healRatio = 1.0f)
        {
            healRatio =
                std::clamp(
                    healRatio,
                    0.1f,
                    1.0f
                );

            for (int i = 0; i < MaxPlayers; ++i)
            {
                if (!players[i].active)
                    continue;

                crew[i].hp =
                    crew[i].maxHp * healRatio;

                crew[i].revive = 0.f;
                crew[i].invulnerable = 1.f;
                crew[i].bracing = false;

                players[i].fishing = false;
                players[i].fishProgress = 0.f;
                players[i].fishCancelArmed = false;
                players[i].fishCatchFx = 0.f;
                players[i].fishCatchAmount = 0;

                effect(
                    players[i].p,
                    2,
                    0.35f
                );
            }
        }

        void healLivingPlayers(float amount)
        {
            if (amount <= 0.f)
                return;

            for (int i = 0; i < MaxPlayers; ++i)
            {
                if (
                    players[i].active &&
                    crew[i].hp > 0.f
                    )
                {
                    healPlayer(i, amount);
                }
            }
        }

        void healLivingPlayersRatio(float ratio)
        {
            if (ratio <= 0.f)
                return;

            for (int i = 0; i < MaxPlayers; ++i)
            {
                if (
                    players[i].active &&
                    crew[i].hp > 0.f
                    )
                {
                    healPlayer(
                        i,
                        crew[i].maxHp * ratio
                    );
                }
            }
        }

        bool isPlayerAlive(int id) const
        {
            if (
                id < 0 ||
                id >= MaxPlayers ||
                !players[id].active
                )
            {
                return false;
            }

            return crew[id].hp > 0.f;
        }

        void addFish(int count = 1)
        {
            if (count > 0)
                fishCount += count;
        }

        bool consumeFish(int count = 1)
        {
            if (
                count <= 0 ||
                fishCount < count
                )
            {
                return false;
            }

            fishCount -= count;
            return true;
        }

        bool useFishToHeal(
            int playerId,
            float healAmount = 35.f)
        {
            if (
                playerId < 0 ||
                playerId >= MaxPlayers ||
                !players[playerId].active
                )
            {
                return false;
            }

            auto& c = crew[playerId];

            if (
                c.hp <= 0.f ||
                c.hp >= c.maxHp
                )
            {
                return false;
            }

            if (!consumeFish(1))
                return false;

            healPlayer(
                playerId,
                healAmount
            );

            return true;
        }

        std::string hint(int id) const
        {
            const auto& p = players[id];

            if (crew[id].hp <= 0.f)
                return "DOWNED - WAITING FOR NEXT STAGE";

            if (p.fishing)
                return "FISHING... (MOVE OR E TO CANCEL)";

            if (dist(p.p, FishingPoint) < FishingInteractRadius)
                return "E: start fishing / H: eat fish";

            if (nearHazard(p) >= 0)
                return "Hold E to repair / extinguish";

            int c = nearCannon(p);

            if (c >= 0)
            {
                if (cannons[c].ammo >= 0)
                    return "MOUSE aim / SPACE fire";

                if (p.held >= 0)
                    return "Hold E: load for 1 second";
            }

            if (dist(p.p, AmmoPoint) < 32.f)
                return "1-6 select shell / E take selected shell";

            return "WASD move / E interact / Q drop / H eat fish";
        }
    };
}
