// ============================================================================
// [김초아 담당] TCP/IP 네트워크 연동 구현
// 방 생성/검색/접속부터 로비 동기화, 실시간 채팅, 게임 입력 및 상태 동기화까지
// 멀티플레이 통신 흐름을 담당합니다.
// ============================================================================
#include "NetworkManager.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace
{
    void writeVector(
        sf::Packet& packet,
        const dw::V& v)
    {
        packet << v.x << v.y;
    }

    bool readVector(
        sf::Packet& packet,
        dw::V& v)
    {
        return static_cast<bool>(
            packet >> v.x >> v.y
            );
    }

    void writeInput(
        sf::Packet& packet,
        const dw::Input& input)
    {
        packet
            << input.x
            << input.y;

        writeVector(
            packet,
            input.aim
        );

        packet
            // 기존 입력
            << static_cast<std::int32_t>(
                input.hold ? 1 : 0)
            << static_cast<std::int32_t>(
                input.tap ? 1 : 0)
            << static_cast<std::int32_t>(
                input.drop ? 1 : 0)
            << static_cast<std::int32_t>(
                input.fire ? 1 : 0)
            //수정
            << static_cast<std::int32_t>(
                input.useFish ? 1 : 0)
            //수정 끝
            << static_cast<std::int32_t>(
                input.melee ? 1 : 0)
            << static_cast<std::int32_t>(
                input.brace ? 1 : 0)

            << static_cast<std::int32_t>(
                input.select)
            << static_cast<std::int32_t>(
                input.vote)

            // 상점 입력
            << static_cast<std::int32_t>(
                input.buy)

            << static_cast<std::int32_t>(
                input.sell)

            << static_cast<std::int32_t>(
                input.ready ? 1 : 0)

            << static_cast<std::int32_t>(
                input.shopVisit)

            << static_cast<std::int32_t>(
                input.purchaseVote)

            << static_cast<std::int32_t>(
                input.purchaseVoteId);
    }

    bool readInput(
        sf::Packet& packet,
        dw::Input& input)
    {
        std::int32_t hold = 0;
        std::int32_t tap = 0;
        std::int32_t drop = 0;
        std::int32_t fire = 0;
        //수정
        std::int32_t useFish = 0;
        //수정 끝

        std::int32_t melee = 0;
        std::int32_t brace = 0;

        std::int32_t select = -1;
        std::int32_t vote = -1;

        // 상점
        std::int32_t buy = -1;
        std::int32_t sell = 0;
        std::int32_t ready = 0;
        std::int32_t shopVisit = 0;
        std::int32_t purchaseVote = -1;
        std::int32_t purchaseVoteId = 0;


        if (!(packet >>
            input.x >>
            input.y))
        {
            return false;
        }


        if (!readVector(
            packet,
            input.aim))
        {
            return false;
        }


        if (!(packet >>
            hold >>
            tap >>
            drop >>
            fire >>
            //수정
            useFish >>
            //수정 끝
            melee >>
            brace >>
            select >>
            vote >>

            buy >>
            sell >>
            ready >>
            shopVisit >>
            purchaseVote >>
            purchaseVoteId))
        {
            return false;
        }


        // 기존 입력
        input.hold =
            hold != 0;

        input.tap =
            tap != 0;

        input.drop =
            drop != 0;

        input.fire =
            fire != 0;

        //수정
        input.useFish =
            useFish != 0;
        //수정 끝

        input.melee =
            melee != 0;

        input.brace =
            brace != 0;

        input.select =
            static_cast<int>(select);

        input.vote =
            static_cast<int>(vote);


        // 상점 입력
        input.buy =
            static_cast<int>(buy);

        input.sell =
            static_cast<int>(sell);

        input.ready =
            ready != 0;

        input.shopVisit =
            static_cast<int>(shopVisit);

        input.purchaseVote =
            static_cast<int>(
                purchaseVote
                );

        input.purchaseVoteId =
            static_cast<int>(
                purchaseVoteId
                );


        return true;
    }

    void writePlayer(
        sf::Packet& packet,
        const dw::Player& value)
    {
        packet
            << static_cast<std::int32_t>(
                value.active ? 1 : 0);

        writeVector(packet, value.p);

        packet
            << static_cast<std::int32_t>(value.held)
            << static_cast<std::int32_t>(value.selected)
            << static_cast<std::int32_t>(value.mount)
            << static_cast<std::int32_t>(value.vote)
            //수정
            << static_cast<std::int32_t>(value.fishing ? 1 : 0)
            << value.fishProgress
            << value.fishCatchFx
            << static_cast<std::int32_t>(value.fishCatchAmount);
        //수정 끝
    }

    bool readPlayer(
        sf::Packet& packet,
        dw::Player& value)
    {
        std::int32_t active = 0;
        std::int32_t held = -1;
        std::int32_t selected = 0;
        std::int32_t mount = -1;
        std::int32_t vote = -1;
        //수정
        std::int32_t fishing = 0;
        std::int32_t fishCatchAmount = 0;
        //수정 끝

        if (!(packet >> active))
            return false;

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >>
            held >>
            selected >>
            mount >>
            vote >>
            //수정
            fishing >>
            value.fishProgress >>
            value.fishCatchFx >>
            fishCatchAmount))
            //수정 끝
        {
            return false;
        }

        value.active = active != 0;
        value.held = static_cast<int>(held);
        value.selected =
            static_cast<int>(selected);
        value.mount = static_cast<int>(mount);
        value.vote = static_cast<int>(vote);
        //수정
        value.fishing = fishing != 0;
        value.fishCatchAmount = static_cast<int>(fishCatchAmount);
        //수정 끝

        return true;
    }

    void writeCannon(
        sf::Packet& packet,
        const dw::Cannon& value)
    {
        writeVector(packet, value.p);

        packet
            << static_cast<std::int32_t>(value.side)
            << static_cast<std::int32_t>(value.ammo)
            << static_cast<std::int32_t>(value.owner)
            << static_cast<std::int32_t>(value.loader)
            << value.progress
            << value.angle;
    }

    bool readCannon(
        sf::Packet& packet,
        dw::Cannon& value)
    {
        std::int32_t side = 0;
        std::int32_t ammo = -1;
        std::int32_t owner = -1;
        std::int32_t loader = -1;

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >>
            side >>
            ammo >>
            owner >>
            loader >>
            value.progress >>
            value.angle))
        {
            return false;
        }

        value.side = static_cast<int>(side);
        value.ammo = static_cast<int>(ammo);
        value.owner = static_cast<int>(owner);
        value.loader = static_cast<int>(loader);

        return true;
    }

    void writeHazard(
        sf::Packet& packet,
        const dw::Hazard& value)
    {
        writeVector(packet, value.p);

        packet
            << static_cast<std::int32_t>(
                value.fire ? 1 : 0)
            << value.progress
            << value.age;
    }

    bool readHazard(
        sf::Packet& packet,
        dw::Hazard& value)
    {
        std::int32_t fire = 0;

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >>
            fire >>
            value.progress >>
            value.age))
        {
            return false;
        }

        value.fire = fire != 0;
        return true;
    }

    void writeDrop(
        sf::Packet& packet,
        const dw::Drop& value)
    {
        writeVector(packet, value.p);
        packet <<
            static_cast<std::int32_t>(
                value.type);
    }

    bool readDrop(
        sf::Packet& packet,
        dw::Drop& value)
    {
        std::int32_t type = 0;

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >> type))
            return false;

        value.type = static_cast<int>(type);
        return true;
    }

    void writeEnemy(
        sf::Packet& packet,
        const dw::Enemy& value)
    {
        packet
            << static_cast<std::int32_t>(value.id)
            << static_cast<std::int32_t>(value.type)
            << static_cast<std::int32_t>(value.side);

        writeVector(packet, value.p);

        packet
            << value.hp
            << value.maxHp
            << value.cooldown
            << value.burn
            << value.freeze
            << value.age
            << value.defense;
    }

    bool readEnemy(
        sf::Packet& packet,
        dw::Enemy& value)
    {
        std::int32_t id = 0;
        std::int32_t type = 0;
        std::int32_t side = 0;

        if (!(packet >>
            id >>
            type >>
            side))
        {
            return false;
        }

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >>
            value.hp >>
            value.maxHp >>
            value.cooldown >>
            value.burn >>
            value.freeze >>
            value.age >>
            value.defense))
        {
            return false;
        }

        value.id = static_cast<int>(id);
        value.type = static_cast<int>(type);
        value.side = static_cast<int>(side);

        return true;
    }

    void writeShot(
        sf::Packet& packet,
        const dw::Shot& value)
    {
        writeVector(packet, value.p);
        writeVector(packet, value.v);

        packet
            << static_cast<std::int32_t>(
                value.type)
            << static_cast<std::int32_t>(
                value.hostile ? 1 : 0)
            << static_cast<std::int32_t>(
                value.black ? 1 : 0)
            << value.damage
            << value.life

            // ★ readShot()에서 읽고 있으므로 반드시 전송
            << static_cast<std::uint32_t>(
                value.hit.size()
                );

        for (int id : value.hit)
        {
            packet <<
                static_cast<std::int32_t>(id);
        }
    }

    bool readShot(
        sf::Packet& packet,
        dw::Shot& value)
    {
        std::int32_t type = 0;
        std::int32_t hostile = 0;
        std::int32_t black = 0;
        std::uint32_t hitCount = 0;

        if (!readVector(packet, value.p))
            return false;

        if (!readVector(packet, value.v))
            return false;

        if (!(packet >>
            type >>
            hostile >>
            black >>
            value.damage >>
            value.life >>
            hitCount))
        {
            return false;
        }

        if (hitCount > 512)
            return false;

        value.type = static_cast<int>(type);
        value.hostile = hostile != 0;
        value.black = black != 0;

        value.hit.clear();
        value.hit.reserve(hitCount);

        for (std::uint32_t i = 0;
            i < hitCount;
            ++i)
        {
            std::int32_t id = 0;

            if (!(packet >> id))
                return false;

            value.hit.push_back(
                static_cast<int>(id));
        }

        return true;
    }

    void writeEffect(
        sf::Packet& packet,
        const dw::Effect& value)
    {
        writeVector(packet, value.p);

        packet
            << value.life
            << value.maxLife
            << static_cast<std::int32_t>(
                value.kind);
    }

    bool readEffect(
        sf::Packet& packet,
        dw::Effect& value)
    {
        std::int32_t kind = 0;

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >>
            value.life >>
            value.maxLife >>
            kind))
        {
            return false;
        }

        value.kind = static_cast<int>(kind);
        return true;
    }
    void writeBoss(
        sf::Packet& packet,
        const dw::Boss& value)
    {
        packet <<
            static_cast<std::int32_t>(
                value.active ? 1 : 0);

        writeVector(packet, value.p);

        packet
            << value.hp
            << value.maxHp
            << value.cooldown
            << value.warning
            << value.burn
            << value.freeze
            << static_cast<std::int32_t>(
                value.pattern)
            << static_cast<std::int32_t>(
                value.side)

            // 추가
            << static_cast<std::int32_t>(
                value.type)
            << value.specialCooldown;
    }

    bool readBoss(
        sf::Packet& packet,
        dw::Boss& value)
    {
        std::int32_t active = 0;
        std::int32_t pattern = 0;
        std::int32_t side = -1;
        std::int32_t bossType = 0;

        if (!(packet >> active))
            return false;

        if (!readVector(packet, value.p))
            return false;

        if (!(packet >>
            value.hp >>
            value.maxHp >>
            value.cooldown >>
            value.warning >>
            value.burn >>
            value.freeze >>
            pattern >>
            side >>
            bossType >>
            value.specialCooldown))
        {
            return false;
        }

        value.active =
            active != 0;

        value.pattern =
            static_cast<int>(pattern);

        value.side =
            static_cast<int>(side);

        value.type =
            static_cast<dw::BossType>(
                bossType
                );

        return true;
    }

    // =============================================================
    // 플레이어 개인 전투 상태
    // =============================================================
    void writeCrew(
        sf::Packet& packet,
        const dw::CrewCombat& value)
    {
        packet
            << value.hp
            << value.invulnerable
            << value.revive
            << value.attackCooldown
            << value.swing
            << static_cast<std::int32_t>(
                value.bracing ? 1 : 0
                );
    }

    bool readCrew(
        sf::Packet& packet,
        dw::CrewCombat& value)
    {
        std::int32_t bracing = 0;

        if (!(packet >>
            value.hp >>
            value.invulnerable >>
            value.revive >>
            value.attackCooldown >>
            value.swing >>
            bracing))
        {
            return false;
        }

        value.bracing =
            bracing != 0;

        return true;
    }


    // =============================================================
    // 유령 선원
    // =============================================================
    void writeGhostSailor(
        sf::Packet& packet,
        const dw::GhostSailor& value)
    {
        writeVector(packet, value.p);
        writeVector(packet, value.strike);

        packet
            << value.hp
            << value.boarding
            << value.cooldown
            << value.windup;
    }

    bool readGhostSailor(
        sf::Packet& packet,
        dw::GhostSailor& value)
    {
        if (!readVector(
            packet,
            value.p))
        {
            return false;
        }

        if (!readVector(
            packet,
            value.strike))
        {
            return false;
        }

        return static_cast<bool>(
            packet >>
            value.hp >>
            value.boarding >>
            value.cooldown >>
            value.windup
            );
    }


    // 크라켄 촉수
    void writeTentacle(
        sf::Packet& packet,
        const dw::DeckTentacle& value)
    {
        writeVector(packet, value.p);

        packet
            << value.hp
            << value.warning
            << value.slamTimer
            << value.flash
            << value.slamAnimation;
    }

    bool readTentacle(
        sf::Packet& packet,
        dw::DeckTentacle& value)
    {
        if (!readVector(
            packet,
            value.p))
        {
            return false;
        }

        return static_cast<bool>(
            packet >>
            value.hp >>
            value.warning >>
            value.slamTimer >>
            value.flash >>
            value.slamAnimation
            );
    }


    // 레비아탄 큰 파도
    void writeGreatWave(
        sf::Packet& packet,
        const dw::GreatWave& value)
    {
        packet
            << static_cast<std::int32_t>(
                value.active ? 1 : 0)
            << static_cast<std::int32_t>(
                value.impacting ? 1 : 0)
            << value.warning
            << value.front
            << value.hitPlayers;
    }

    bool readGreatWave(
        sf::Packet& packet,
        dw::GreatWave& value)
    {
        std::int32_t active = 0;
        std::int32_t impacting = 0;

        if (!(packet >>
            active >>
            impacting >>
            value.warning >>
            value.front >>
            value.hitPlayers))
        {
            return false;
        }

        value.active =
            active != 0;

        value.impacting =
            impacting != 0;

        return true;
    }

    // 골드 / 탄약 / 강화 / 상점 상태 전송
    void writeShopState(
        sf::Packet& packet,
        const dw::Game& game)
    {
        // 팀 골드
        packet <<
            static_cast<std::int32_t>(
                game.gold.getGold()
                );
        packet <<
            static_cast<std::int32_t>(
                game.fishCount
                );

        // 특수탄 5종
        // Normal은 무한탄이므로 전송하지 않음
        for (int i = 1;
            i < dw::ShellCount;
            ++i)
        {
            packet <<
                static_cast<std::int32_t>(
                    game.ammo.getAmmoCount(
                        dw::itemAmmo(i)
                    )
                    );
        }


        // 강화 레벨 5종
        packet
            << static_cast<std::int32_t>(
                game.upgrades.getNormalDamageLevel()
                )
            << static_cast<std::int32_t>(
                game.upgrades.getShipHpLevel()
                )
            << static_cast<std::int32_t>(
                game.upgrades.getReloadLevel()
                )
            << static_cast<std::int32_t>(
                game.upgrades.getRepairLevel()
                )
            << static_cast<std::int32_t>(
                game.upgrades.getFishingLevel()
                );


        // 상점 기본 상태
        packet
            << static_cast<std::int32_t>(
                game.shopVisit
                )
            << game.shopRemaining;


        // 준비 상태
        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            packet <<
                static_cast<std::int32_t>(
                    game.shopReady[i] ? 1 : 0
                    );
        }


        // 구매 결과
        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            packet
                << static_cast<std::int32_t>(
                    game.shopResult[i]
                    )
                << game.shopResultTime[i];
        }


        // 구매 투표 상태
        packet
            << static_cast<std::int32_t>(
                game.purchaseVoteActive ? 1 : 0
                )
            << static_cast<std::int32_t>(
                game.purchaseVoteItem
                )
            << static_cast<std::int32_t>(
                game.purchaseVoteProposer
                )
            << static_cast<std::int32_t>(
                game.purchaseVoteId
                )
            << game.purchaseVoteRemaining;


        // 각 플레이어 찬반표
        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            packet <<
                static_cast<std::int32_t>(
                    game.purchaseBallots[i]
                    );
        }
    }


    // =============================================================
    // 골드 / 탄약 / 강화 / 상점 상태 수신
    // =============================================================
    bool readShopState(
        sf::Packet& packet,
        dw::Game& game)
    {
        std::int32_t gold = 0;
        std::int32_t fishCount = 0;
        std::array<std::int32_t, 5>
            ammoCounts{};

        std::int32_t normalDamageLevel = 0;
        std::int32_t shipHpLevel = 0;
        std::int32_t reloadLevel = 0;
        std::int32_t repairLevel = 0;
        std::int32_t fishingLevel = 0;

        std::int32_t shopVisit = 0;

        std::array<std::int32_t,
            dw::MaxPlayers>
            ready{};

        std::array<std::int32_t,
            dw::MaxPlayers>
            results{};

        std::array<float,
            dw::MaxPlayers>
            resultTimes{};

        std::int32_t voteActive = 0;
        std::int32_t voteItem = -1;
        std::int32_t voteProposer = -1;
        std::int32_t voteId = 0;

        std::array<std::int32_t,
            dw::MaxPlayers>
            ballots{};


        // 골드
        if (!(packet >>
            gold >>
            fishCount))
        {
            return false;
        }

        // 특수탄 5종
        for (int i = 0; i < 5; ++i)
        {
            if (!(packet >> ammoCounts[i]))
                return false;
        }


        // 강화
        if (!(packet >>
            normalDamageLevel >>
            shipHpLevel >>
            reloadLevel >>
            repairLevel >>
            fishingLevel))
        {
            return false;
        }


        // 상점
        if (!(packet >>
            shopVisit >>
            game.shopRemaining))
        {
            return false;
        }


        // 준비 상태
        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            if (!(packet >> ready[i]))
                return false;
        }


        // 결과 상태
        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            if (!(packet >>
                results[i] >>
                resultTimes[i]))
            {
                return false;
            }
        }


        // 투표 상태
        if (!(packet >>
            voteActive >>
            voteItem >>
            voteProposer >>
            voteId >>
            game.purchaseVoteRemaining))
        {
            return false;
        }


        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            if (!(packet >> ballots[i]))
                return false;
        }


        // =============================
        // 실제 Game에 적용
        // =============================

        game.gold.reset();

        if (gold > 0)
        {
            game.gold.addGold(
                static_cast<int>(gold)
            );
        }

        game.fishCount =
            static_cast<int>(fishCount);

        game.ammo.reset();

        // i=0은 Spread부터
        for (int i = 0; i < 5; ++i)
        {
            if (ammoCounts[i] > 0)
            {
                game.ammo.addAmmo(
                    dw::itemAmmo(i + 1),
                    static_cast<int>(
                        ammoCounts[i]
                        )
                );
            }
        }


        game.upgrades.reset();

        for (int i = 0;
            i < normalDamageLevel;
            ++i)
        {
            game.upgrades
                .upgradeNormalDamage();
        }

        for (int i = 0;
            i < shipHpLevel;
            ++i)
        {
            game.upgrades
                .upgradeShipHp();
        }

        for (int i = 0;
            i < reloadLevel;
            ++i)
        {
            game.upgrades
                .upgradeReload();
        }

        for (int i = 0;
            i < repairLevel;
            ++i)
        {
            game.upgrades
                .upgradeRepair();
        }

        for (int i = 0;
            i < fishingLevel;
            ++i)
        {
            game.upgrades
                .upgradeFishing();
        }


        game.shopVisit =
            static_cast<int>(
                shopVisit
                );


        for (int i = 0;
            i < dw::MaxPlayers;
            ++i)
        {
            game.shopReady[i] =
                ready[i] != 0;

            game.shopResult[i] =
                static_cast<int>(
                    results[i]
                    );

            game.shopResultTime[i] =
                resultTimes[i];

            game.purchaseBallots[i] =
                static_cast<int>(
                    ballots[i]
                    );
        }


        game.purchaseVoteActive =
            voteActive != 0;

        game.purchaseVoteItem =
            static_cast<int>(
                voteItem
                );

        game.purchaseVoteProposer =
            static_cast<int>(
                voteProposer
                );

        game.purchaseVoteId =
            static_cast<int>(
                voteId
                );


        return true;
    }

    void writeGame(
        sf::Packet& packet,
        const dw::Game& game)
    {
        packet
            << static_cast<std::int32_t>(
                game.phase)
            << static_cast<std::int32_t>(
                game.theme)
            << static_cast<std::int32_t>(
                game.stage)
            << static_cast<std::int32_t>(
                game.wave)
            << static_cast<std::int32_t>(
                game.kills)
            << static_cast<std::int32_t>(
                game.nextId)
            << static_cast<std::int32_t>(
                game.reward)
            << game.hp
            << game.maxHp
            << game.time
            << game.age
            << game.spawn
            << game.repairBonus
            << game.leakScale
            << game.rng
            << game.shotsFired
            << game.impacts;

        writeShopState(packet, game);

        for (const auto& player :
            game.players)
        {
            writePlayer(
                packet,
                player
            );
        }

        for (const auto& cannon :
            game.cannons)
        {
            writeCannon(
                packet,
                cannon
            );
        }

        packet <<
            static_cast<std::uint32_t>(
                game.enemies.size());

        for (const auto& value :
            game.enemies)
        {
            writeEnemy(packet, value);
        }

        packet <<
            static_cast<std::uint32_t>(
                game.shots.size());

        for (const auto& value :
            game.shots)
        {
            writeShot(packet, value);
        }

        packet <<
            static_cast<std::uint32_t>(
                game.hazards.size());

        for (const auto& value :
            game.hazards)
        {
            writeHazard(packet, value);
        }

        packet <<
            static_cast<std::uint32_t>(
                game.drops.size());

        for (const auto& value :
            game.drops)
        {
            writeDrop(packet, value);
        }

        packet <<
            static_cast<std::uint32_t>(
                game.effects.size());

        for (const auto& value :
            game.effects)
        {
            writeEffect(packet, value);
        }

        writeBoss(
            packet,
            game.boss
        );

        // 플레이어 개인 전투 상태
        for (const auto& value :
            game.crew)
        {
            writeCrew(
                packet,
                value
            );
        }


        // 유령 선원
        packet <<
            static_cast<std::uint32_t>(
                game.sailors.size()
                );

        for (const auto& value :
            game.sailors)
        {
            writeGhostSailor(
                packet,
                value
            );
        }


        // 크라켄 촉수
        packet <<
            static_cast<std::uint32_t>(
                game.tentacles.size()
                );

        for (const auto& value :
            game.tentacles)
        {
            writeTentacle(
                packet,
                value
            );
        }


        // 레비아탄 큰 파도
        writeGreatWave(
            packet,
            game.greatWave
        );


        // 특수 몬스터 생성 타이머
        packet <<
            game.deckSpawn;
    }

    bool readGame(
        sf::Packet& packet,
        dw::Game& game)
    {
        std::int32_t phase = 0;
        std::int32_t theme = static_cast<std::int32_t>(dw::Theme::Sea);
        std::int32_t stage = 1;
        std::int32_t wave = 0;
        std::int32_t kills = 0;
        std::int32_t nextId = 1;
        std::int32_t reward = 0;

        if (!(packet >>
            phase >>
            theme >>
            stage >>
            wave >>
            kills >>
            nextId >>
            reward >>
            game.hp >>
            game.maxHp >>
            game.time >>
            game.age >>
            game.spawn >>
            game.repairBonus >>
            game.leakScale >>
            game.rng >>
            game.shotsFired >>
            game.impacts))
        {
            return false;
        }

        game.phase =
            static_cast<dw::Phase>(phase);
        game.theme =
            static_cast<dw::Theme>(theme);
        game.stage =
            static_cast<int>(stage);
        game.wave =
            static_cast<int>(wave);
        game.kills =
            static_cast<int>(kills);
        game.nextId =
            static_cast<int>(nextId);
        game.reward =
            static_cast<int>(reward);

        if (!readShopState(
            packet,
            game))
        {
            return false;
        }

        for (auto& player :
            game.players)
        {
            if (!readPlayer(
                packet,
                player))
            {
                return false;
            }
        }

        for (auto& cannon :
            game.cannons)
        {
            if (!readCannon(
                packet,
                cannon))
            {
                return false;
            }
        }

        std::uint32_t count = 0;

        if (!(packet >> count) ||
            count > 512)
        {
            return false;
        }

        game.enemies.clear();
        game.enemies.resize(count);

        for (auto& value :
            game.enemies)
        {
            if (!readEnemy(
                packet,
                value))
            {
                return false;
            }
        }

        if (!(packet >> count) ||
            count > 2048)
        {
            return false;
        }

        game.shots.clear();
        game.shots.resize(count);

        for (auto& value :
            game.shots)
        {
            if (!readShot(
                packet,
                value))
            {
                return false;
            }
        }

        if (!(packet >> count) ||
            count > 64)
        {
            return false;
        }

        game.hazards.clear();
        game.hazards.resize(count);

        for (auto& value :
            game.hazards)
        {
            if (!readHazard(
                packet,
                value))
            {
                return false;
            }
        }

        if (!(packet >> count) ||
            count > 128)
        {
            return false;
        }

        game.drops.clear();
        game.drops.resize(count);

        for (auto& value :
            game.drops)
        {
            if (!readDrop(
                packet,
                value))
            {
                return false;
            }
        }

        if (!(packet >> count) ||
            count > 512)
        {
            return false;
        }

        game.effects.clear();
        game.effects.resize(count);

        for (auto& value :
            game.effects)
        {
            if (!readEffect(
                packet,
                value))
            {
                return false;
            }
        }

        // 보스
        if (!readBoss(
            packet,
            game.boss))
        {
            return false;
        }


        // 플레이어 개인 전투 상태
        for (auto& value :
            game.crew)
        {
            if (!readCrew(
                packet,
                value))
            {
                return false;
            }
        }


        // 유령 선원
        std::uint32_t sailorCount = 0;

        if (!(packet >> sailorCount) ||
            sailorCount > 32)
        {
            return false;
        }

        game.sailors.clear();
        game.sailors.resize(
            sailorCount
        );

        for (auto& value :
            game.sailors)
        {
            if (!readGhostSailor(
                packet,
                value))
            {
                return false;
            }
        }


        // 크라켄 촉수
        std::uint32_t tentacleCount = 0;

        if (!(packet >> tentacleCount) ||
            tentacleCount > 32)
        {
            return false;
        }

        game.tentacles.clear();
        game.tentacles.resize(
            tentacleCount
        );

        for (auto& value :
            game.tentacles)
        {
            if (!readTentacle(
                packet,
                value))
            {
                return false;
            }
        }


        // 레비아탄 큰 파도
        if (!readGreatWave(
            packet,
            game.greatWave))
        {
            return false;
        }


        // 생성 타이머
        if (!(packet >>
            game.deckSpawn))
        {
            return false;
        }


        return true;
    }
}

NetworkManager::NetworkManager()
{
}

// 서버 시작
bool NetworkManager::startServer(
    unsigned short port)
{
    if (serverRunning)
    {
        return true;
    }

    listener.close();

    if (listener.listen(port) !=
        sf::Socket::Status::Done)
    {
        serverRunning = false;
        return false;
    }

    listener.setBlocking(false);

    serverPort = port;
    serverRunning = true;
    gameStarted = false;
    hostClosed = false;

    remoteGameInputs = {};
    receivedGameStateReady = false;
    localPlayerId = 0;
    chatMessages.clear();

    return true;
}

// 방 코드 저장
void NetworkManager::setRoomCode(
    const std::string& roomCode)
{
    serverRoomCode = roomCode;
}

// 방 이름 저장
void NetworkManager::setRoomName(
    const sf::String& roomName)
{
    serverRoomName = roomName;
}

// 최대 인원 저장
void NetworkManager::setMaxPlayers(
    int count)
{
    maxPlayers = count;
}

// 같은 LAN에서 TCP로 방 코드 검색
std::optional<NetworkManager::RoomServerInfo>
NetworkManager::findRoomServer(
    const std::string& roomCode)
{
    auto localAddress =
        sf::IpAddress::getLocalAddress(
            sf::IpAddress::Type::IpV4
        );

    if (!localAddress.has_value())
    {
        return std::nullopt;
    }

    std::string localIp =
        localAddress->toString();

    std::size_t lastDot =
        localIp.rfind('.');

    if (lastDot ==
        std::string::npos)
    {
        return std::nullopt;
    }

    std::string subnet =
        localIp.substr(
            0,
            lastDot + 1
        );

    std::atomic<bool> found(false);
    std::mutex resultMutex;

    std::optional<RoomServerInfo>
        result;

    constexpr int threadCount = 8;

    auto scanWorker =
        [&](int workerIndex)
        {
            for (int host =
                workerIndex + 1;
                host <= 254;
                host += threadCount)
            {
                if (found.load())
                {
                    return;
                }

                std::string ipText =
                    subnet +
                    std::to_string(host);

                auto candidate =
                    sf::IpAddress::fromString(
                        ipText
                    );

                if (!candidate.has_value())
                {
                    continue;
                }

                sf::TcpSocket probeSocket;

                probeSocket.setBlocking(true);

                sf::Socket::Status connectStatus =
                    probeSocket.connect(
                        *candidate,
                        54000,
                        sf::milliseconds(35)
                    );

                if (connectStatus !=
                    sf::Socket::Status::Done)
                {
                    probeSocket.disconnect();
                    continue;
                }

                sf::Packet request;

                request
                    << std::string("FIND_ROOM")
                    << roomCode;

                if (probeSocket.send(request) !=
                    sf::Socket::Status::Done)
                {
                    probeSocket.disconnect();
                    continue;
                }

                probeSocket.setBlocking(false);

                sf::Clock waitClock;

                while (
                    waitClock.getElapsedTime() <
                    sf::milliseconds(350))
                {
                    if (found.load())
                    {
                        probeSocket.disconnect();
                        return;
                    }

                    sf::Packet response;

                    sf::Socket::Status receiveStatus =
                        probeSocket.receive(
                            response
                        );

                    if (receiveStatus ==
                        sf::Socket::Status::Done)
                    {
                        std::string packetType;
                        std::string foundCode;
                        std::uint32_t foundPort = 0;

                        if (response >>
                            packetType >>
                            foundCode >>
                            foundPort)
                        {
                            if (packetType ==
                                "ROOM_MATCH" &&
                                foundCode ==
                                roomCode)
                            {
                                {
                                    std::lock_guard<
                                        std::mutex>
                                        lock(
                                            resultMutex
                                        );

                                    if (!found.load())
                                    {
                                        result =
                                            RoomServerInfo{
                                                *candidate,
                                                static_cast<
                                                    unsigned short>(
                                                    foundPort
                                                )
                                        };

                                        found.store(true);
                                    }
                                }

                                probeSocket.disconnect();
                                return;
                            }
                        }

                        break;
                    }

                    if (receiveStatus ==
                        sf::Socket::Status::Disconnected ||
                        receiveStatus ==
                        sf::Socket::Status::Error)
                    {
                        break;
                    }

                    sf::sleep(
                        sf::milliseconds(5)
                    );
                }

                probeSocket.disconnect();
            }
        };

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (int i = 0;
        i < threadCount;
        ++i)
    {
        workers.emplace_back(
            scanWorker,
            i
        );
    }

    for (auto& worker :
        workers)
    {
        worker.join();
    }

    return result;
}

// 서버 업데이트
void NetworkManager::updateServer()
{
    if (!serverRunning)
    {
        return;
    }

    // 새 연결 한 프레임에 여러 개 처리
    for (int acceptCount = 0;
        acceptCount < 8;
        ++acceptCount)
    {
        auto newClient =
            std::make_unique<
            sf::TcpSocket>();

        newClient->setBlocking(
            false
        );

        sf::Socket::Status acceptStatus =
            listener.accept(
                *newClient
            );

        if (acceptStatus !=
            sf::Socket::Status::Done)
        {
            break;
        }

        clients.push_back(
            std::move(newClient)
        );

        playerNicknames.push_back(
            sf::String()
        );

        clientPlayerIds.push_back(0);
    }

    std::size_t i = 0;

    while (i < clients.size())
    {
        if (!clients[i])
        {
            clients.erase(
                clients.begin() + i
            );

            playerNicknames.erase(
                playerNicknames.begin() + i
            );

            clientPlayerIds.erase(
                clientPlayerIds.begin() + i
            );

            continue;
        }

        sf::Packet packet;

        sf::Socket::Status status =
            clients[i]->receive(
                packet
            );

        if (status ==
            sf::Socket::Status::Disconnected ||
            status ==
            sf::Socket::Status::Error)
        {
            int oldPlayerId =
                clientPlayerIds[i];

            if (oldPlayerId > 0 &&
                oldPlayerId < dw::MaxPlayers)
            {
                remoteGameInputs[
                    oldPlayerId] = {};
            }

            clients[i]->disconnect();

            clients.erase(
                clients.begin() + i
            );

            playerNicknames.erase(
                playerNicknames.begin() + i
            );

            clientPlayerIds.erase(
                clientPlayerIds.begin() + i
            );

            continue;
        }

        if (status !=
            sf::Socket::Status::Done)
        {
            ++i;
            continue;
        }

        std::string packetType;

        if (!(packet >> packetType))
        {
            ++i;
            continue;
        }

        // 방 코드 검색용 TCP 연결
        if (packetType ==
            "FIND_ROOM")
        {
            std::string requestedCode;

            if (packet >>
                requestedCode)
            {
                sf::Packet response;

                if (requestedCode ==
                    serverRoomCode &&
                    !gameStarted)
                {
                    response
                        << std::string(
                            "ROOM_MATCH"
                        )
                        << serverRoomCode
                        << static_cast<
                        std::uint32_t>(
                            serverPort
                            );
                }
                else
                {
                    response
                        << std::string(
                            "ROOM_NO_MATCH"
                        )
                        << requestedCode
                        << static_cast<
                        std::uint32_t>(
                            serverPort
                            );
                }

                clients[i]->send(
                    response
                );
            }

            clients[i]->disconnect();

            clients.erase(
                clients.begin() + i
            );

            playerNicknames.erase(
                playerNicknames.begin() + i
            );

            clientPlayerIds.erase(
                clientPlayerIds.begin() + i
            );

            continue;
        }

        if (packetType == "JOIN")
        {
            std::string receivedCode;
            std::string utf8Nickname;

            if (!(packet >>
                receivedCode >>
                utf8Nickname))
            {
                ++i;
                continue;
            }

            if (receivedCode !=
                serverRoomCode)
            {
                sf::Packet response;
                response <<
                    std::string("FAIL");

                clients[i]->send(response);
                clients[i]->disconnect();

                clients.erase(
                    clients.begin() + i
                );

                playerNicknames.erase(
                    playerNicknames.begin() + i
                );

                clientPlayerIds.erase(
                    clientPlayerIds.begin() + i
                );

                continue;
            }

            int joinedPlayers = 1;

            for (const auto& name :
                playerNicknames)
            {
                if (!name.isEmpty())
                {
                    ++joinedPlayers;
                }
            }

            if (joinedPlayers >=
                maxPlayers)
            {
                sf::Packet response;
                response <<
                    std::string("FULL");

                clients[i]->send(response);
                clients[i]->disconnect();

                clients.erase(
                    clients.begin() + i
                );

                playerNicknames.erase(
                    playerNicknames.begin() + i
                );

                clientPlayerIds.erase(
                    clientPlayerIds.begin() + i
                );

                continue;
            }

            sf::String nickname =
                sf::String::fromUtf8(
                    utf8Nickname.begin(),
                    utf8Nickname.end()
                );

            playerNicknames[i] =
                nickname;

            sf::Packet response;
            response <<
                std::string("OK");

            clients[i]->send(response);
        }
        else if (packetType ==
            "GAME_INPUT")
        {
            int playerId =
                clientPlayerIds[i];

            if (playerId > 0 &&
                playerId <
                dw::MaxPlayers)
            {
                dw::Input input;

                if (readInput(
                    packet,
                    input))
                {
                    dw::mergeInput(
                        remoteGameInputs[playerId],
                        input
                    );
                }
            }
        }
        else if (packetType ==
            "CHAT_SEND")
        {
            std::string utf8Message;

            if (packet >> utf8Message &&
                i < playerNicknames.size() &&
                !playerNicknames[i].isEmpty() &&
                !utf8Message.empty())
            {
                sf::String message =
                    sf::String::fromUtf8(
                        utf8Message.begin(),
                        utf8Message.end()
                    );

                if (message.getSize() > 120)
                {
                    message.erase(120,
                        message.getSize() - 120);
                }

                const sf::String sender =
                    playerNicknames[i];

                chatMessages.push_back({
                    sender, message
                    });

                if (chatMessages.size() > 40)
                {
                    chatMessages.erase(
                        chatMessages.begin()
                    );
                }

                sf::U8String senderUtf8 =
                    sender.toUtf8();
                sf::U8String messageUtf8 =
                    message.toUtf8();

                std::string senderText(
                    senderUtf8.begin(),
                    senderUtf8.end()
                );
                std::string messageText(
                    messageUtf8.begin(),
                    messageUtf8.end()
                );

                sf::Packet chatPacket;
                chatPacket
                    << std::string("CHAT_MESSAGE")
                    << senderText
                    << messageText;

                for (std::size_t c = 0;
                    c < clients.size(); ++c)
                {
                    if (!clients[c] ||
                        c >= playerNicknames.size() ||
                        playerNicknames[c].isEmpty())
                    {
                        continue;
                    }

                    clients[c]->send(chatPacket);
                }
            }
        }

        ++i;
    }
}

// 서버 종료
void NetworkManager::stopServer()
{
    // 방장이 방/게임을 나가면 참가자에게 종료 신호를 먼저 보냄
    sf::Packet closePacket;
    closePacket << std::string("HOST_CLOSED");

    for (std::size_t i = 0;
        i < clients.size();
        ++i)
    {
        if (!clients[i])
        {
            continue;
        }

        // 방 검색용 임시 연결이 아니라 실제 입장한 참가자에게만 전송
        if (i < playerNicknames.size() &&
            !playerNicknames[i].isEmpty())
        {
            clients[i]->send(closePacket);
        }
    }

    listener.close();

    for (auto& client :
        clients)
    {
        if (client)
        {
            client->disconnect();
        }
    }

    clients.clear();
    playerNicknames.clear();
    clientPlayerIds.clear();

    serverRoomCode.clear();
    serverRoomName.clear();

    serverRunning = false;
    gameStarted = false;
    hostClosed = false;

    remoteGameInputs = {};
    receivedGameStateReady = false;
    localPlayerId = 0;
    chatMessages.clear();
}

// 서버 접속
bool NetworkManager::connectToServer(
    const sf::IpAddress& ip,
    unsigned short port)
{
    serverSocket.disconnect();
    serverSocket.setBlocking(true);

    connected = false;
    joinAccepted = false;
    joinRejected = false;
    roomFull = false;
    kicked = false;
    gameStarted = false;
    hostClosed = false;

    syncedPlayerNicknames.clear();
    syncedRoomName.clear();
    syncedMaxPlayers = 2;

    remoteGameInputs = {};
    receivedGameStateReady = false;
    localPlayerId = 0;
    chatMessages.clear();

    sf::Socket::Status status =
        serverSocket.connect(
            ip,
            port,
            sf::seconds(3.f)
        );

    if (status !=
        sf::Socket::Status::Done)
    {
        serverSocket.disconnect();
        serverSocket.setBlocking(true);

        return false;
    }

    serverSocket.setBlocking(false);
    connected = true;

    return true;
}

// 방 코드와 닉네임 전송
bool NetworkManager::sendJoinRequest(
    const sf::String& nickname,
    const std::string& roomCode)
{
    if (!connected)
    {
        return false;
    }

    sf::U8String utf8 =
        nickname.toUtf8();

    std::string utf8Nickname(
        utf8.begin(),
        utf8.end()
    );

    sf::Packet packet;

    packet
        << std::string("JOIN")
        << roomCode
        << utf8Nickname;

    sf::Socket::Status status =
        serverSocket.send(packet);

    return status ==
        sf::Socket::Status::Done;
}

// 참가자 서버 응답 확인
void NetworkManager::updateClient()
{
    if (!connected)
    {
        return;
    }

    // 한 프레임에 여러 패킷 처리
    for (int receiveCount = 0;
        receiveCount < 16;
        ++receiveCount)
    {
        sf::Packet packet;

        sf::Socket::Status status =
            serverSocket.receive(
                packet
            );

        if (status ==
            sf::Socket::Status::Disconnected ||
            status ==
            sf::Socket::Status::Error)
        {
            disconnect();
            hostClosed = true;
            return;
        }

        if (status !=
            sf::Socket::Status::Done)
        {
            return;
        }

        std::string packetType;

        if (!(packet >> packetType))
        {
            continue;
        }

        if (packetType == "OK")
        {
            joinAccepted = true;
            joinRejected = false;
            roomFull = false;
        }
        else if (packetType == "FAIL")
        {
            joinAccepted = false;
            joinRejected = true;
            roomFull = false;
        }
        else if (packetType == "FULL")
        {
            joinAccepted = false;
            joinRejected = false;
            roomFull = true;
        }
        else if (packetType == "KICK")
        {
            joinAccepted = false;
            joinRejected = false;
            roomFull = false;
            kicked = true;
        }
        else if (packetType ==
            "GAME_START")
        {
            std::uint32_t receivedId = 0;

            if (packet >> receivedId)
            {
                localPlayerId =
                    static_cast<int>(
                        receivedId
                        );
            }
            else
            {
                // 구형 GAME_START와의 안전 호환
                localPlayerId = 1;
            }

            joinAccepted = false;
            gameStarted = true;
        }
        else if (packetType ==
            "PLAYER_LIST")
        {
            std::string utf8RoomName;
            std::uint32_t
                receivedMaxPlayers = 0;
            std::uint32_t count = 0;

            if (!(packet >>
                utf8RoomName >>
                receivedMaxPlayers >>
                count))
            {
                continue;
            }

            syncedRoomName =
                sf::String::fromUtf8(
                    utf8RoomName.begin(),
                    utf8RoomName.end()
                );

            syncedMaxPlayers =
                static_cast<int>(
                    receivedMaxPlayers
                    );

            syncedPlayerNicknames.clear();

            for (std::uint32_t i = 0;
                i < count;
                ++i)
            {
                std::string utf8Name;

                if (!(packet >>
                    utf8Name))
                {
                    break;
                }

                sf::String name =
                    sf::String::fromUtf8(
                        utf8Name.begin(),
                        utf8Name.end()
                    );

                syncedPlayerNicknames
                    .push_back(name);
            }
        }
        else if (packetType ==
            "CHAT_MESSAGE")
        {
            std::string utf8Sender;
            std::string utf8Message;

            if (packet >>
                utf8Sender >>
                utf8Message)
            {
                sf::String sender =
                    sf::String::fromUtf8(
                        utf8Sender.begin(),
                        utf8Sender.end()
                    );

                sf::String message =
                    sf::String::fromUtf8(
                        utf8Message.begin(),
                        utf8Message.end()
                    );

                chatMessages.push_back({
                    sender, message
                    });

                if (chatMessages.size() > 40)
                {
                    chatMessages.erase(
                        chatMessages.begin()
                    );
                }
            }
        }
        else if (packetType ==
            "HOST_CLOSED")
        {
            disconnect();
            hostClosed = true;
            return;
        }
        else if (packetType ==
            "GAME_STATE")
        {
            dw::Game temp;

            if (readGame(
                packet,
                temp))
            {
                receivedGameState =
                    std::move(temp);

                receivedGameStateReady =
                    true;
            }
        }
    }
}

// 참가자 연결 종료
void NetworkManager::disconnect()
{
    serverSocket.disconnect();
    serverSocket.setBlocking(true);

    hostClosed = false;
    connected = false;
    joinAccepted = false;
    joinRejected = false;
    roomFull = false;
    kicked = false;
    gameStarted = false;

    syncedPlayerNicknames.clear();
    syncedRoomName.clear();
    syncedMaxPlayers = 2;

    remoteGameInputs = {};
    receivedGameStateReady = false;
    localPlayerId = 0;
    chatMessages.clear();
}

// 참가자 내보내기
bool NetworkManager::kickPlayer(
    std::size_t participantIndex)
{
    if (!serverRunning)
    {
        return false;
    }

    std::size_t visibleIndex = 0;

    for (std::size_t i = 0;
        i < playerNicknames.size();
        ++i)
    {
        if (playerNicknames[i]
            .isEmpty())
        {
            continue;
        }

        if (visibleIndex ==
            participantIndex)
        {
            if (!clients[i])
            {
                return false;
            }

            sf::Packet packet;
            packet <<
                std::string("KICK");

            clients[i]->send(packet);
            clients[i]->disconnect();

            int oldPlayerId =
                clientPlayerIds[i];

            if (oldPlayerId > 0 &&
                oldPlayerId <
                dw::MaxPlayers)
            {
                remoteGameInputs[
                    oldPlayerId] = {};
            }

            clients.erase(
                clients.begin() + i
            );

            playerNicknames.erase(
                playerNicknames.begin() + i
            );

            clientPlayerIds.erase(
                clientPlayerIds.begin() + i
            );

            return true;
        }

        ++visibleIndex;
    }

    return false;
}

// 게임 시작 전송
bool NetworkManager::startGame()
{
    if (!serverRunning)
    {
        return false;
    }

    int currentPlayers = 1;

    for (const auto& name :
        playerNicknames)
    {
        if (!name.isEmpty())
        {
            ++currentPlayers;
        }
    }

    if (currentPlayers < maxPlayers)
    {
        return false;
    }

    bool success = true;
    int nextPlayerId = 1;

    remoteGameInputs = {};

    for (std::size_t i = 0;
        i < clients.size();
        ++i)
    {
        if (!clients[i])
        {
            continue;
        }

        if (i >= playerNicknames.size() ||
            playerNicknames[i].isEmpty())
        {
            continue;
        }

        if (nextPlayerId >= dw::MaxPlayers)
        {
            break;
        }

        clientPlayerIds[i] =
            nextPlayerId;

        sf::Packet packet;

        packet
            << std::string("GAME_START")
            << static_cast<std::uint32_t>(
                nextPlayerId
                );

        sf::Socket::Status status =
            clients[i]->send(packet);

        if (status !=
            sf::Socket::Status::Done)
        {
            success = false;
        }

        ++nextPlayerId;
    }

    if (success)
    {
        gameStarted = true;
    }

    return success;
}

bool NetworkManager::isServerRunning() const
{
    return serverRunning;
}

bool NetworkManager::isConnected() const
{
    return connected;
}

bool NetworkManager::isJoinAccepted() const
{
    return joinAccepted;
}

bool NetworkManager::isJoinRejected() const
{
    return joinRejected;
}

bool NetworkManager::isRoomFull() const
{
    return roomFull;
}

bool NetworkManager::isKicked() const
{
    return kicked;
}

bool NetworkManager::isGameStarted() const
{
    return gameStarted;
}

bool NetworkManager::isHostClosed() const
{
    return hostClosed;
}

int NetworkManager::getMaxPlayers() const
{
    return maxPlayers;
}

int NetworkManager::getSyncedMaxPlayers() const
{
    return syncedMaxPlayers;
}

std::size_t
NetworkManager::getClientCount() const
{
    return clients.size();
}

const std::vector<sf::String>&
NetworkManager::getPlayerNicknames() const
{
    return playerNicknames;
}

const std::vector<sf::String>&
NetworkManager::getSyncedPlayerNicknames() const
{
    return syncedPlayerNicknames;
}

const sf::String&
NetworkManager::getRoomName() const
{
    return serverRoomName;
}

const sf::String&
NetworkManager::getSyncedRoomName() const
{
    return syncedRoomName;
}

// ============================================================================
// [김초아 담당] 실시간 채팅 송수신
// 방장은 CHAT_MESSAGE를 참가자에게 브로드캐스트하고, 참가자는 CHAT_SEND로 전송합니다.
// ============================================================================
bool NetworkManager::sendChatMessage(
    const sf::String& sender,
    const sf::String& message)
{
    if (message.isEmpty())
    {
        return false;
    }

    sf::String safeMessage = message;
    if (safeMessage.getSize() > 120)
    {
        safeMessage.erase(
            120,
            safeMessage.getSize() - 120
        );
    }

    sf::U8String messageUtf8 =
        safeMessage.toUtf8();
    std::string messageText(
        messageUtf8.begin(),
        messageUtf8.end()
    );

    if (serverRunning)
    {
        sf::String safeSender = sender;
        if (safeSender.isEmpty())
        {
            safeSender = U"HOST";
        }

        chatMessages.push_back({
            safeSender, safeMessage
            });

        if (chatMessages.size() > 40)
        {
            chatMessages.erase(
                chatMessages.begin()
            );
        }

        sf::U8String senderUtf8 =
            safeSender.toUtf8();
        std::string senderText(
            senderUtf8.begin(),
            senderUtf8.end()
        );

        sf::Packet packet;
        packet
            << std::string("CHAT_MESSAGE")
            << senderText
            << messageText;

        for (std::size_t i = 0;
            i < clients.size(); ++i)
        {
            if (!clients[i] ||
                i >= playerNicknames.size() ||
                playerNicknames[i].isEmpty())
            {
                continue;
            }

            clients[i]->send(packet);
        }

        return true;
    }

    if (connected)
    {
        sf::Packet packet;
        packet
            << std::string("CHAT_SEND")
            << messageText;

        return serverSocket.send(packet) ==
            sf::Socket::Status::Done;
    }

    return false;
}

const std::vector<NetworkManager::ChatMessage>&
NetworkManager::getChatMessages() const
{
    return chatMessages;
}

void NetworkManager::clearChatMessages()
{
    chatMessages.clear();
}

// 플레이어 목록 전송
void NetworkManager::broadcastPlayerList(
    const sf::String& hostNickname)
{
    if (!serverRunning ||
        gameStarted)
    {
        return;
    }

    std::vector<sf::String> names;
    names.push_back(hostNickname);

    for (const auto& name :
        playerNicknames)
    {
        if (!name.isEmpty())
        {
            names.push_back(name);
        }
    }

    sf::U8String roomUtf8 =
        serverRoomName.toUtf8();

    std::string utf8RoomName(
        roomUtf8.begin(),
        roomUtf8.end()
    );

    sf::Packet packet;

    packet
        << std::string("PLAYER_LIST")
        << utf8RoomName
        << static_cast<std::uint32_t>(
            maxPlayers
            )
        << static_cast<std::uint32_t>(
            names.size()
            );

    for (const auto& name :
        names)
    {
        sf::U8String utf8 =
            name.toUtf8();

        std::string utf8Name(
            utf8.begin(),
            utf8.end()
        );

        packet << utf8Name;
    }

    for (std::size_t i = 0;
        i < clients.size();
        ++i)
    {
        if (!clients[i])
        {
            continue;
        }

        if (i >=
            playerNicknames.size() ||
            playerNicknames[i].isEmpty())
        {
            continue;
        }

        clients[i]->send(packet);
    }
}

// =============================================================
// 실제 멀티 게임 통신
// =============================================================

// ============================================================================
// [김초아 담당] 게임 네트워크 동기화
// 참가자 입력 -> 방장, 권위 서버 상태 -> 참가자 흐름을 연결합니다.
// ============================================================================
bool NetworkManager::sendGameInput(
    const dw::Input& input)
{
    if (!connected)
    {
        return false;
    }

    sf::Packet packet;

    packet <<
        std::string("GAME_INPUT");

    writeInput(
        packet,
        input
    );

    return serverSocket.send(
        packet
    ) == sf::Socket::Status::Done;
}

void NetworkManager::copyGameInputs(
    std::array<dw::Input,
    dw::MaxPlayers>& inputs)
{
    for (int i = 1;
        i < dw::MaxPlayers;
        ++i)
    {
        // 이전 프레임에 아직 처리되지 않은
        // tap/drop/fire/melee 등의 1회성 입력 보존
        dw::mergeInput(
            inputs[i],
            remoteGameInputs[i]
        );

        // 서버 수신 버퍼에서는
        // 1회성 입력만 제거
        remoteGameInputs[i]
            .edgesOff();
    }
}

void NetworkManager::broadcastGameState(
    const dw::Game& game)
{
    if (!serverRunning ||
        !gameStarted)
    {
        return;
    }

    sf::Packet packet;

    packet <<
        std::string("GAME_STATE");

    writeGame(
        packet,
        game
    );

    for (std::size_t i = 0;
        i < clients.size();
        ++i)
    {
        if (!clients[i])
        {
            continue;
        }

        if (i >=
            playerNicknames.size() ||
            playerNicknames[i].isEmpty())
        {
            continue;
        }

        clients[i]->send(
            packet
        );
    }
}

bool NetworkManager::consumeGameState(
    dw::Game& game)
{
    if (!receivedGameStateReady)
    {
        return false;
    }

    game =
        receivedGameState;

    receivedGameStateReady =
        false;

    return true;
}

int NetworkManager::getLocalPlayerId() const
{
    return localPlayerId;
}

void NetworkManager::clearGameStarted()
{
    if (connected &&
        !serverRunning)
    {
        gameStarted = false;
    }
}
