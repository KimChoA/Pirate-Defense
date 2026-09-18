#pragma once
#include "Game.hpp"
#include "SpriteAnimation.hpp"
#include "EndingCutscene.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace dw::cannonVisual
{
    // Complete, independently drawn layers. Coordinates are logical game pixels.
    // The barrel's pivot is (0,0), its bore points along +X, and its lip is x=30.
    struct PixelRect { float x, y, w, h; std::uint32_t rgb; };
    inline constexpr float MuzzleReach = 30.f;
    inline constexpr std::array<PixelRect, 16> Carriage{{
        {-16, 7, 32, 8, 0x19202c},
        {-12,-7, 24,22, 0x241e24},
        {-10,-5, 20,18, 0x70422c},
        { -8,-4, 16, 4, 0xbd8450},
        { -8, 1, 16, 3, 0x9b6137},
        {-13,-2,  4,14, 0xc39251},
        {  9,-2,  4,14, 0xc39251},
        {-16, 4,  8,11, 0x202330},
        {  8, 4,  8,11, 0x202330},
        {-15, 5,  6, 8, 0x815136},
        {  9, 5,  6, 8, 0x815136},
        {-14, 6,  4, 5, 0xbb8b54},
        { 10, 6,  4, 5, 0xbb8b54},
        {-13, 7,  2, 3, 0x303747},
        { 11, 7,  2, 3, 0x303747},
        { -7,10, 14, 2, 0xd6a464}
    }};
    inline constexpr std::array<PixelRect, 19> Barrel{{
        {-10,-5,  5,10, 0x171d2c},
        { -7,-7, 15,14, 0x171d2c},
        {  8,-6, 18,12, 0x171d2c},
        { 25,-8,  5,16, 0x171d2c},
        { -8,-3,  3, 6, 0x65738c},
        { -5,-5, 13,10, 0x53637d},
        {  8,-4, 18, 8, 0x45536d},
        { -4,-5, 11, 3, 0xa3b4c8},
        {  8,-4, 16, 2, 0x8d9fb7},
        { -4, 3, 12, 2, 0x303b53},
        {  8, 2, 17, 2, 0x28334a},
        {  0,-7,  3,14, 0xaf7539},
        {  0,-6,  2, 4, 0xf1cd7b},
        { 19,-6,  3,12, 0xaf7539},
        { 19,-5,  2, 3, 0xf1cd7b},
        { 26,-7,  3,14, 0x8c9cb1},
        { 27,-5,  3,10, 0x121a2a},
        { 27,-4,  1, 8, 0x394861},
        { 26,-7,  3, 2, 0xd2dbe0}
    }};
    inline float recoil(float phase)
    {
        if (phase < 0.f || phase >= 1.f) return 0.f;
        const float t = phase < .14f ? phase / .14f : (phase - .14f) / .86f;
        const float eased = t * t * (3.f - 2.f * t);
        return 5.f * (phase < .14f ? eased : 1.f - eased);
    }
}

namespace dw
{
    inline sf::Color shellColor(int n)
    {
        static const sf::Color c[] = {
            {222, 221, 190},
            {255, 169, 76},
            {198, 161, 236},
            {251, 100, 60},
            {117, 217, 237}
        };

        return n >= 0 && n < 5
            ? c[n]
            : sf::Color(93, 112, 124);
    }

    inline sf::Color playerColor(int n)
    {
        static const sf::Color c[] = {
            {70, 193, 194},
            {247, 151, 99},
            {167, 147, 234},
            {237, 202, 91},
            {218, 119, 170}
        };

        return c[std::clamp(n, 0, 4)];
    }

    struct Screen
    {
        bool menu = true;
        bool help = false;
        bool paused = false;
        bool editing = false;
        bool muted = false;
        bool client = false;
        bool online = false;

        int local = 0;
        sf::Vector2f pointer{-1.f, -1.f};

        // 메인에서 네트워크 닉네임/채팅 상태를 채워 렌더러에 전달
        std::array<std::string, MaxPlayers> playerNames{};
        bool chatVisible = false;
        bool chatActive = false;
        std::string chatInput;
        std::vector<std::string> chatLines;

        std::string endpoint =
            "127.0.0.1:53000";

        std::string status = "";
        std::string address = "";
    };

    // ========================================================================
    // [김초아 담당 범위]
    // 이 Renderer 중 게임 UI/HUD 구성 및 전체 화면 디자인 통합,
    // 낮-저녁-밤 분위기 전환과 야간 시야 제한 효과를 담당
    // ========================================================================
    class Renderer
    {
        sf::Font font;

        std::vector<sf::Vertex>
            rectangles;

        struct TextSlot
        {
            std::string original;
            std::unique_ptr<sf::Text> text;
        };

        std::vector<TextSlot> labels;

        std::size_t labelIndex = 0;
        float gChatWidth = 213.f;
        visual::Animator animator;
        struct SailorVisual { V position; float hp; std::uint64_t key; };
        std::vector<SailorVisual> sailorVisuals;
        std::uint64_t nextSailorKey = 4000000;
        int animationScene = -1;
        float animationAge = -1;
        std::unique_ptr<EndingCutscene> endingCutscene;
        sf::Clock endingClock;
        bool endingVisible = false;

        V drawAnimated(sf::RenderTarget& r, const std::string& key,
            float x, float y, float width, float height, const visual::Pose& pose,
            visual::Rig rig, sf::Color tint = sf::Color::White)
        {
            flush(r);
            const auto& texture = pixelTextures.at(key);
            const auto size = texture.getSize();
            const float scale = std::min(width / size.x, height / size.y);
            const float w = size.x * scale, h = size.y * scale;
            if (pose.flash) tint = sf::Color(255, 145, 145, tint.a);
            if (pose.motion == visual::Motion::Down) tint = {155,155,170,155};
            const int facing = rig == visual::Rig::Boss ? 1 : pose.facing;
            auto point = [&](float u, float v) {
                const auto offset = visual::deform(u, v, pose, rig);
                return V{std::round(x + facing * ((u-.5f)*w + offset.x)),
                         std::round(y + (v-.5f)*h + offset.y)};
            };
            std::vector<sf::Vertex> mesh;
            mesh.reserve(8 * size.y * 6);
            auto vertex = [&](float u, float v) {
                return sf::Vertex{point(u,v), tint, {u*size.x, v*size.y}};
            };
            for (unsigned row = 0; row < size.y; ++row)
                for (int col = 0; col < 8; ++col)
                {
                    const float u = col/8.f, u1 = (col+1)/8.f;
                    const float v = float(row)/size.y, v1 = float(row+1)/size.y;
                    auto a=vertex(u,v), b=vertex(u1,v), c=vertex(u,v1), d=vertex(u1,v1);
                    mesh.insert(mesh.end(), {a,b,c,c,b,d});
                }
            sf::RenderStates state; state.texture = &texture;
            r.draw(mesh.data(), mesh.size(), sf::PrimitiveType::Triangles, state);
            return point(.72f, .76f); // same deformation as the carrying hand
        }


        std::unordered_map<std::string, sf::Texture>
            pixelTextures;

        void loadPixelTexture(
            const std::string& key,
            const std::string& path)
        {
            sf::Texture texture;

            if (!texture.loadFromFile(path))
                throw std::runtime_error("Missing required artwork: " + path);

            texture.setSmooth(false);
            pixelTextures.emplace(key, std::move(texture));
        }

        bool drawPixel(
            sf::RenderTarget& r,
            const std::string& key,
            float centerX,
            float centerY,
            float width,
            float height,
            sf::Color tint = sf::Color::White)
        {
            const auto found = pixelTextures.find(key);

            if (found == pixelTextures.end())
                return false;

            flush(r);

            if (key == "ship/deck")
            {
                const float dx[] = {DeckLeft - 14, DeckLeft, DeckRight, DeckRight + 14};
                const float dy[] = {44, DeckTop, DeckBottom, 338};
                const int sy[] = {0, 40, 230, 292};
                for (int row = 0; row < 3; ++row)
                    for (int col = 0; col < 3; ++col)
                    {
                        const int sx = col == 0 ? 0 : 30;
                        const int sw = col == 0 ? 30 : col == 1 ? 200 : -30;
                        sf::Sprite tile(found->second);
                        tile.setTextureRect(sf::IntRect({sx, sy[row]}, {sw, sy[row+1]-sy[row]}));
                        tile.setPosition({dx[col], dy[row]});
                        tile.setScale({(dx[col+1]-dx[col])/std::abs(sw),
                            (dy[row+1]-dy[row])/(sy[row+1]-sy[row])});
                        r.draw(tile);
                    }
                return true;
            }
            const auto size = found->second.getSize();
            sf::Sprite sprite(found->second);
            sprite.setOrigin({
                static_cast<float>(size.x) * 0.5f,
                static_cast<float>(size.y) * 0.5f
                });
            const float sx = width / static_cast<float>(size.x);
            const float sy = height / static_cast<float>(size.y);
            const bool surface = key.rfind("environment/", 0) == 0 ||
                key == "ship/deck" || key == "ending/treasure_victory";
            const float fit = std::min(sx, sy);
            sprite.setScale(surface ? sf::Vector2f{sx, sy} : sf::Vector2f{fit, fit});
            sprite.setPosition({
                std::floor(centerX),
                std::floor(centerY)
                });
            sprite.setColor(tint);
            r.draw(sprite);
            return true;
        }

        bool drawPixelRect(
            sf::RenderTarget& r,
            const std::string& key,
            float x,
            float y,
            float width,
            float height,
            sf::Color tint = sf::Color::White)
        {
            return drawPixel(
                r,
                key,
                x + width * 0.5f,
                y + height * 0.5f,
                width,
                height,
                tint
            );
        }

        const char* stagePlate(
            bool island,
            int wave) const
        {
            if (island)
            {
                if (wave <= 1) return "environment/island_day";
                if (wave == 2) return "environment/island_sunset";
                return "environment/island_night";
            }

            if (wave <= 1) return "environment/sea_day";
            if (wave == 2) return "environment/sea_sunset";
            return "environment/sea_night";
        }

        void flush(
            sf::RenderTarget& r)
        {
            if (!rectangles.empty())
            {
                r.draw(
                    rectangles.data(),
                    rectangles.size(),
                    sf::PrimitiveType::Triangles
                );

                rectangles.clear();
            }
        }

    public:
        Renderer()
        {
            if (!font.openFromFile(
                "C:/Windows/Fonts/malgun.ttf"))
            {
                throw std::runtime_error(
                    "Korean font unavailable"
                );
            }

            font.setSmooth(true);

            const std::string root =
                "assets/images/pixel/";

            const std::pair<const char*, const char*> assets[] = {
                { "environment/sea_day", "environment/sea_day.png" },
                { "environment/sea_sunset", "environment/sea_sunset.png" },
                { "environment/sea_night", "environment/sea_night.png" },
                { "environment/island_day", "environment/island_day.png" },
                { "environment/island_sunset", "environment/island_sunset.png" },
                { "environment/island_night", "environment/island_night.png" },
                { "ship/deck", "ship/ship_deck.png" },
                { "ship/cannon", "ship/cannon.png" },
                { "ship/broken_deck", "ship/broken_deck.png" },
                { "ship/ammo_crate", "ship/ammo_crate.png" },
                { "ship/repair_station", "ship/repair_station.png" },
                { "ship/fishing_post", "ship/fishing_post.png" },
                { "players/player1", "players/player1.png" },
                { "players/player2", "players/player2.png" },
                { "players/player3", "players/player3.png" },
                { "players/player4", "players/player4.png" },
                { "players/player5", "players/player5.png" },
                { "enemies/ghost_sailor", "enemies/ghost_sailor.png" },
                { "enemies/sea_raider", "enemies/sea_raider.png" },
                { "enemies/armored_corsair", "enemies/armored_corsair.png" },
                { "enemies/goblin_raider", "enemies/goblin_raider.png" },
                { "enemies/red_beast", "enemies/red_beast.png" },
                { "enemies/rock_warrior", "enemies/rock_warrior.png" },
                { "bosses/ghost_ship", "bosses/ghost_ship.png" },
                { "bosses/leviathan", "bosses/leviathan.png" },
                { "bosses/kraken", "bosses/kraken.png" },
                { "bosses/stone_golem", "bosses/stone_golem.png" },
                { "bosses/storm_wyvern", "bosses/storm_wyvern.png" },
                { "bosses/ancient_treant", "bosses/ancient_treant.png" },
                { "ammo/normal", "ammo/normal.png" },
                { "ammo/spread", "ammo/spread.png" },
                { "ammo/pierce", "ammo/pierce.png" },
                { "ammo/blast", "ammo/blast.png" },
                { "ammo/flame", "ammo/flame.png" },
                { "ammo/heavy", "ammo/heavy.png" },
                { "effects/explosion", "effects/explosion.png" },
                { "effects/smoke", "effects/smoke.png" },
                { "effects/fire", "effects/fire.png" },
                { "effects/repair", "effects/repair.png" },
                { "effects/heal", "effects/heal.png" },
                { "effects/healing_fish", "effects/healing_fish.png" },
                { "effects/wave", "effects/wave.png" },
                { "effects/wind", "effects/wind.png" },
                { "effects/treasure_sparkle", "effects/treasure_sparkle.png" },
                { "ui/hud_panel", "ui/hud_panel.png" },
                { "ui/tooltip_panel", "ui/tooltip_panel.png" },
                { "ui/chat_panel", "ui/chat_panel.png" },
                { "ui/shop_header", "ui/shop_header.png" },
                { "ui/vote_card", "ui/vote_card.png" },
                { "ui/help_panel", "ui/help_panel.png" },
                { "ui/gold_coins", "ui/gold_coins.png" },
                { "ending/treasure_closed", "ending/treasure_chest_closed.png" },
                { "ending/treasure_open", "ending/treasure_chest_open.png" },
                { "ending/treasure_victory", "ending/treasure_victory.png" }
            };

            for (const auto& asset : assets)
                loadPixelTexture(asset.first, root + asset.second);
        }

        void box(
            sf::RenderTarget& r,
            float x,
            float y,
            float w,
            float h,
            sf::Color c)
        {
            x = std::floor(x);
            y = std::floor(y);

            const sf::Vertex a{
                {x, y},
                c
            };

            const sf::Vertex b{
                {x + w, y},
                c
            };

            const sf::Vertex d{
                {x, y + h},
                c
            };

            const sf::Vertex e{
                {x + w, y + h},
                c
            };

            rectangles.insert(
                rectangles.end(),
                {
                    a,
                    b,
                    d,
                    d,
                    b,
                    e
                }
            );
        }

        std::string koreanText(
            std::string value)
        {
            static std::unordered_map<
                std::string,
                std::string>
                cache;

            const auto found =
                cache.find(value);

            if (found != cache.end())
            {
                return found->second;
            }

            const std::string
                originalText = value;

            static const char*
                translations[][2] =
            {
                {
                    "PAUSED",
                    "일시정지"
                },

                {
                    "CAPTAIN'S HANDBOOK",
                    "조작 방법"
                },

                {
                    "WASD move  /  E interact  /  Q put down shell",
                    "WASD 이동 / E 상호작용 / Q 포탄 내려놓기"
                },

                {
                    "Use 1-6 to select a shell, then press E at the ammo rack",
                    "1~6으로 탄약 선택 / 탄약고에서 E로 가져오기"
                },

                {
                    "Hold E beside a cannon for 1 second to load",
                    "함포 옆에서 E를 1초 누르면 장전"
                },

                {
                    "After loading, aim with MOUSE and press SPACE to fire.",
                    "장전 후 마우스로 조준하고 Space로 발사"
                },

                {
                    "Hold E at damage sites: repair / extinguish",
                    "파손·화재 위치에서 E를 길게 눌러 수리·소화"
                },

                {
                    "Survive 90 sec each stage; kill the final sea monster.",
                    "각 스테이지 90초 생존 후 마지막 바다 괴물 처치"
                },

                {
                    "Online match keeps running while help is open.",
                    "멀티플레이는 도움말을 열어도 계속 진행됩니다."
                },

                {
                    "Solo play pauses when unfocused or help is open.",
                    "혼자 할 때는 도움말·다른 창 전환 시 일시정지됩니다."
                },

                {
                    "C melee/brace   H eat fish   TAB chat input",
                    "C 근접공격/방어 / H 물고기 사용 / Tab 채팅 입력"
                },

                {
                    "F1 close help   ESC main menu   M mute",
                    "F1 도움말 닫기 / Esc 메인 메뉴 / M 효과음"
                },

                {
                    "CREW ASSEMBLY",
                    "출항 대기실"
                },

                {
                    "Connected: ",
                    "접속 인원: "
                },

                {
                    "Waiting for host to start...",
                    "방장의 시작을 기다리는 중..."
                },

                {
                    "ENTER: depart with current crew",
                    "Enter: 현재 인원으로 시작"
                },

                {
                    "ESC: return to main menu",
                    "Esc: 메인 메뉴로 돌아가기"
                },

                {
                    "STAGE CLEAR - CHOOSE A CREW RELIC",
                    "스테이지 완료! 팀 보상을 선택하세요"
                },

                {
                    "Each player votes. Majority wins; ties favor the left card.",
                    "모두 투표하면 다수결로 결정됩니다. 동률이면 왼쪽 보상!"
                },

                {
                    "1  REINFORCED HULL",
                    "1  강화 선체"
                },

                {
                    "2  REPAIR KIT",
                    "2  수리 도구"
                },

                {
                    "3  WATER SEAL",
                    "3  방수 처리"
                },

                {
                    "+150 max HP / heal 250",
                    "최대 체력 +150 / 회복 250"
                },

                {
                    "+25 repair heal / heal 160",
                    "수리 회복 +25 / 회복 160"
                },

                {
                    "35% less leaks / heal 160",
                    "지속 피해 -35% / 회복 160"
                },

                {
                    "VOTES ",
                    "투표 "
                },

                {
                    "Press 1, 2 or 3. No shop. Cannon damage / load time stay fixed.",
                    "1~3으로 선택하세요. 함포 공격력과 장전 시간은 유지됩니다."
                },

                {
                    "THE SEA IS YOURS",
                    "최종 승리!"
                },

                {
                    "SHIP LOST",
                    "배가 침몰했습니다"
                },

                {
                    "All three stages complete. The monster is defeated.",
                    "모든 스테이지 완료! 바다 괴물을 처치했습니다."
                },

                {
                    "The hull reached zero. Your crew can try again.",
                    "배의 체력이 바닥났습니다. 다시 도전해 보세요."
                },

                {
                    "Enemies sunk: ",
                    "격침한 적: "
                },

                {
                    "Waiting for host to restart",
                    "방장의 재시작을 기다리는 중"
                },

                {
                    "ENTER: restart from stage 1",
                    "Enter: 1스테이지부터 재시작"
                },

                {
                    "ESC: main menu",
                    "Esc: 메인 메뉴"
                },

                {
                    "HOST DISCONNECTED",
                    "방장과 연결이 끊겼습니다"
                },

                {
                    "Room closed. ESC returns to the menu.",
                    "방이 닫혔습니다. Esc를 눌러 메뉴로 돌아가세요."
                },

                {
                    "Host disconnected / room closed",
                    "연결 종료 / 방이 닫혔습니다"
                },

                {
                    "Port is in use or blocked",
                    "포트가 사용 중이거나 차단되어 있습니다"
                },

                {
                    "Enter an IPv4 address, e.g. 192.168.0.10",
                    "IPv4 주소를 입력하세요. 예: 192.168.0.10"
                },

                {
                    "Connection failed. Check IP, port and host.",
                    "접속 실패: 방장의 IP와 포트, 방 생성 여부를 확인하세요"
                },

                {
                    "Connecting...",
                    "접속 중..."
                },

                {
                    "CONNECTED / P",
                    "접속 완료 / P"
                },

                {
                    "HOST TCP :",
                    "방장 포트: "
                },

                {
                    "Invalid port (1-65535)",
                    "포트는 1~65535 사이로 입력하세요"
                },

                {
                    "LAN IP: ",
                    "내부 IP: "
                },

                {
                    "HOST: ",
                    "방장 주소: "
                },

                {
                    "unknown",
                    "확인 불가"
                },

                {
                    "MOUSE aim / SPACE fire",
                    "마우스 조준 / Space 발사"
                },

                {
                    "Hold E to fish",
                    "E 길게 누르기: 낚시"
                },

                {
                    "Press E to start fishing",
                    "E 한 번 누르기: 낚시 시작"
                },

                {
                    "Hold E to repair / extinguish",
                    "E 길게 누르기: 수리·소화"
                },

                {
                    "Hold E: load for 1 second",
                    "E 1초 누르기: 장전"
                },

                {
                    "1-6 select shell / E take selected shell",
                    "1~6 탄약 선택 / E 선택 탄약 가져오기"
                },

                {
                    "WASD move / E interact / Q drop",
                    "WASD 이동 / E 상호작용 / Q 내려놓기"
                },

                {
                    "E hold: repair / extinguish",
                    "E 길게: 수리·소화"
                },

                {
                    "F1 help   M sound   F11 screen",
                    "F1 도움말 / M 효과음 / F11 화면"
                },

                {
                    "! ATTACK !",
                    "! 공격 예고 !"
                },

                {
                    "TENTACLE SLAM",
                    "촉수 강타"
                },

                {
                    "FIRE SURGE",
                    "화염 분출"
                },

                {
                    "THE ABYSS",
                    "심해 괴물"
                },

                {
                    "BOSS FIGHT",
                    "보스 전투"
                },

                {
                    " READY",
                    " 장전"
                },

                {
                    " EMPTY",
                    " 비었음"
                },

                {
                    "AMMO",
                    "탄약고"
                },

                {
                    "FISHING",
                    "낚시"
                },

                {
                    "DECKWATCH",
                    "해상 방어전"
                },

                {
                    "STAGE ",
                    "스테이지 "
                },

                {
                    "SHIP ",
                    "배 체력 "
                },

                {
                    "TIME ",
                    "시간 "
                },

                {
                    "WAVE ",
                    "공세 "
                },

                {
                    "KILLS ",
                    "격침 "
                },

                {
                    "CREW ",
                    "인원 "
                },

                {
                    "SHELL ",
                    "포탄 "
                },

                {
                    "SELECTED: ",
                    "선택: "
                },

                {
                    "TCP CLIENT",
                    "참가자"
                },

                {
                    "TCP HOST",
                    "방장"
                },

                {
                    "SOLO",
                    "혼자"
                },

                {
                    "IRON",
                    "일반탄"
                },

                {
                    "SPREAD",
                    "확산탄"
                },

                {
                    "PIERCE",
                    "관통탄"
                },

                {
                    "BLAST",
                    "폭발탄"
                },

                {
                    "FLAME",
                    "화염탄"
                },

                {
                    "HEAVY",
                    "중포탄"
                },

                {
                    "NONE",
                    "없음"
                },

                // 유동호 병합: 플레이어 상태 / 낚시 안내
                {
                    "DOWNED - WAITING FOR NEXT STAGE",
                    "행동 불능 - 다음 스테이지를 기다리는 중"
                },

                {
                    "FISHING... (MOVE OR E TO CANCEL)",
                    "낚시 중... (이동 또는 E로 취소)"
                },

                {
                    "E: start fishing / H: eat fish",
                    "E: 낚시 시작 / H: 물고기 먹기(회복)"
                },

                {
                    "WASD move / E interact / Q drop / H eat fish",
                    "WASD 이동 / E 상호작용 / Q 내려놓기 / H 회복"
                },

                {
                    "FISH ",
                    "물고기 "
                }
            };

            for (const auto& entry :
                translations)
            {
                const std::string original =
                    entry[0];

                const std::string translated =
                    entry[1];

                std::size_t position = 0;

                while (
                    (
                        position =
                        value.find(
                            original,
                            position
                        )
                        )
                    != std::string::npos
                    )
                {
                    value.replace(
                        position,
                        original.size(),
                        translated
                    );

                    position +=
                        translated.size();
                }
            }

            if (cache.size() < 4096)
            {
                cache.emplace(
                    originalText,
                    value
                );
            }

            return value;
        }

        void text(
            sf::RenderTarget& r,
            const std::string& s,
            float x,
            float y,
            unsigned size = 10,
            sf::Color c =
            { 222, 232, 222 },
            bool centered = false)
        {
            flush(r);

            if (labelIndex ==
                labels.size())
            {
                labels.push_back({
                    "",
                    std::make_unique<
                        sf::Text>(font)
                    });
            }

            auto& slot =
                labels[labelIndex++];

            if (slot.original != s)
            {
                const auto translated =
                    koreanText(s);

                slot.text->setString(
                    sf::String::fromUtf8(
                        translated.begin(),
                        translated.end()
                    )
                );

                slot.original = s;
            }

            auto& t = *slot.text;

            if (
                t.getCharacterSize() !=
                size
                )
            {
                t.setCharacterSize(size);
            }

            if (centered)
            {
                const sf::FloatRect bounds =
                    t.getLocalBounds();

                t.setOrigin({
                    bounds.position.x +
                    bounds.size.x / 2.f,
                    0.f
                    });
            }
            else
            {
                t.setOrigin({ 0.f, 0.f });
            }

            t.setPosition({
                std::floor(x),
                std::floor(y)
                });

            t.setOutlineColor(sf::Color(5, 15, 23, 220));
            t.setOutlineThickness(0.5f);
            t.setFillColor(c);

            r.draw(t);
        }

        // 사용자 입력/채팅처럼 매 프레임 바뀌는 UTF-8 문자열은 번역 캐시를 거치지 않고
        // 그대로 그립니다. 한글 IME 입력 중 불필요한 번역/캐시 작업을 줄여 끊김을 방지
        void rawText(
            sf::RenderTarget& r,
            const std::string& s,
            float x,
            float y,
            unsigned size = 10,
            sf::Color c = { 222, 232, 222 },
            bool centered = false)
        {
            flush(r);

            if (labelIndex == labels.size())
            {
                labels.push_back({
                    "",
                    std::make_unique<sf::Text>(font)
                    });
            }

            auto& slot = labels[labelIndex++];

            // rawText 전용 prefix를 사용해 일반 text() 캐시와 구분
            const std::string cacheKey = std::string("\x1FRAW:") + s;
            if (slot.original != cacheKey)
            {
                slot.text->setString(
                    sf::String::fromUtf8(
                        s.begin(),
                        s.end()
                    )
                );
                slot.original = cacheKey;
            }

            auto& t = *slot.text;

            if (t.getCharacterSize() != size)
                t.setCharacterSize(size);

            if (centered)
            {
                const sf::FloatRect bounds = t.getLocalBounds();
                t.setOrigin({
                    bounds.position.x + bounds.size.x / 2.f,
                    0.f
                    });
            }
            else
            {
                t.setOrigin({ 0.f, 0.f });
            }

            t.setString(sf::String::fromUtf8(s.begin(), s.end()));
            const float availableWidth = (gChatWidth > 0.f ? gChatWidth : 213.f);
            sf::String visible = t.getString();
            while (t.getLocalBounds().size.x > availableWidth && visible.getSize() > 1)
            {
                visible.erase(0, 1);
                t.setString(visible);
            }
            t.setOutlineColor(sf::Color(5, 15, 23, 220));
            t.setOutlineThickness(0.4f);
            t.setFillColor(c);
            t.setPosition({ std::floor(x), std::floor(y) });
            r.draw(t);
        }

        void panel(sf::RenderTarget& r, float x, float y, float w, float h)
        {
            box(r, x + 2, y + 2, w, h, {4, 12, 20, 170});
            box(r, x, y, w, h, {174, 128, 66});
            box(r, x + 1, y + 1, w - 2, h - 2, {57, 40, 31});
            box(r, x + 3, y + 3, w - 6, h - 6, {20, 34, 43});
            box(r, x + 4, y + 4, w - 8, 1, {86, 91, 77});
        }

        void bar(
            sf::RenderTarget& r,
            float x,
            float y,
            float w,
            float ratio,
            sf::Color c)
        {
            box(
                r,
                x - 1,
                y - 1,
                w + 2,
                7,
                { 10, 22, 33 }
            );

            box(
                r,
                x,
                y,
                w,
                5,
                { 52, 60, 60 }
            );

            box(
                r,
                x,
                y,
                w *
                std::clamp(
                    ratio,
                    0.f,
                    1.f
                ),
                5,
                c
            );
        }

        void ball(
            sf::RenderTarget& r,
            V p,
            int type)
        {
            static const char* keys[] = {
                "ammo/normal",
                "ammo/spread",
                "ammo/pierce",
                "ammo/blast",
                "ammo/flame",
                "ammo/heavy"
            };

            const int clamped =
                std::clamp(type, 0, 5);

            if (drawPixel(
                r,
                keys[clamped],
                p.x,
                p.y,
                type == Flame || type == Heavy ? 10.f : 9.f,
                type == Flame || type == Heavy ? 10.f : 9.f
            ))
            {
                return;
            }

        }

        void fishIcon(
            sf::RenderTarget& r,
            float x,
            float y,
            float s,
            sf::Color body,
            sf::Color fin,
            sf::Color eye = sf::Color(247, 250, 252))
        {
            box(r, x + 0.f * s, y + 2.f * s, 1.0f * s, 1.0f * s, body);
            box(r, x + 1.f * s, y + 1.f * s, 1.0f * s, 1.0f * s, body);
            box(r, x + 1.f * s, y + 3.f * s, 1.0f * s, 1.0f * s, body);
            box(r, x + 2.f * s, y + 1.f * s, 4.0f * s, 3.0f * s, body);
            box(r, x + 6.f * s, y + 2.f * s, 1.0f * s, 1.0f * s, body);
            box(r, x + 4.f * s, y + 0.f * s, 1.0f * s, 1.0f * s, fin);
            box(r, x + 4.f * s, y + 4.f * s, 1.0f * s, 1.0f * s, fin);
            box(r, x + 5.5f * s, y + 1.6f * s, 0.8f * s, 0.8f * s, eye);
        }

        const char* bossDisplayName(const Game& g) const
        {
            if (g.theme == Theme::Island)
            {
                if (g.boss.type == BossType::GhostShip) return "STONE GOLEM";
                if (g.boss.type == BossType::Leviathan) return "STORM WYVERN";
                if (g.boss.type == BossType::Kraken) return "ANCIENT TREANT";
                return "ISLAND BOSS";
            }

            if (g.boss.type == BossType::GhostShip) return "GHOST SHIP";
            if (g.boss.type == BossType::Leviathan) return "LEVIATHAN";
            if (g.boss.type == BossType::Kraken) return "KRAKEN";
            return "BOSS";
        }

        // ====================================================================
        // [김초아 담당] 밤 시야 제한 효과
        // 플레이어 위치를 중심으로 가시 반경을 유지하고 거리에 따라 암부 알파를
        // 증가시켜 저녁 -> 밤으로 갈수록 시야가 좁아지는 효과를 구현합니다.
        // ====================================================================
        void drawNightVision(
            sf::RenderTarget& r,
            const Game& g,
            const Screen& ui)
        {
            if (g.wave <= 1)
                return;

            const int localId =
                std::clamp(ui.local, 0, MaxPlayers - 1);

            if (!g.players[localId].active)
                return;

            const V center = g.players[localId].p;

            // 난이도 완화: 밤에도 주변 상황을 읽을 수 있도록
            // 시야 반경과 가장자리 페이드를 넉넉하게 유지한다.
            float radius = 195.f;
            float fade = 90.f;
            float maxAlpha = 155.f;

            if (g.wave == 2)
            {
                const float eveningProgress =
                    std::clamp(
                        1.f - g.time / WaveSeconds,
                        0.f,
                        1.f
                    );

                // 저녁 초반은 거의 전체가 보이고, 후반에도
                // 전투가 답답하지 않도록 255 -> 195 정도만 줄인다.
                radius =
                    255.f -
                    60.f * eveningProgress;

                maxAlpha =
                    45.f +
                    110.f * eveningProgress;

                fade =
                    100.f -
                    10.f * eveningProgress;
            }
            else
            {
                // 밤 / 보스전: 이전보다 훨씬 넓고 부드러운 시야
                radius = 180.f;
                fade = 90.f;
                maxAlpha = 185.f;
            }

            constexpr float Cell = 10.f;
            constexpr float WorldTop = 43.f;
            constexpr float WorldBottom = 338.f;

            for (float y = WorldTop; y < WorldBottom; y += Cell)
            {
                for (float x = 0.f; x < 640.f; x += Cell)
                {
                    const float cx = x + Cell * 0.5f;
                    const float cy = y + Cell * 0.5f;
                    const float dx = cx - center.x;
                    const float dy = cy - center.y;
                    const float d = std::sqrt(dx * dx + dy * dy);

                    if (d <= radius)
                        continue;

                    const float darkness =
                        std::clamp(
                            (d - radius) / fade,
                            0.f,
                            1.f
                        );

                    const std::uint8_t alpha =
                        static_cast<std::uint8_t>(
                            std::clamp(
                                maxAlpha * darkness,
                                0.f,
                                245.f
                            )
                            );

                    if (alpha == 0)
                        continue;

                    box(
                        r,
                        x,
                        y,
                        Cell,
                        std::min(Cell, WorldBottom - y),
                        sf::Color(3, 7, 18, alpha)
                    );
                }
            }
        }

        void drawMenuAccents(sf::RenderTarget& r)
        {
            rectangles.clear();
            labelIndex = 0;

            drawPixel(r, "players/player1", 348, 585, 84, 104);
            drawPixel(r, "players/player3", 925, 585, 84, 104);
            panel(r, 420, 632, 440, 40);
            text(r, "바다에서 보물섬까지 · 최대 5명 협동 항해", 640, 641, 17,
                {249,218,155}, true);
            flush(r);
        }

        void draw(
            sf::RenderTarget& r,
            const Game& g,
            const Screen& ui)
        {
            rectangles.clear();
            labelIndex = 0;

            if (!ui.menu && g.phase == Phase::Won)
            {
                if (!endingCutscene) endingCutscene = std::make_unique<EndingCutscene>();
                if (!endingVisible) { (void)endingClock.restart(); endingVisible = true; }
                endingCutscene->draw(r, endingClock.getElapsedTime().asSeconds(), ui.client);
                return;
            }
            endingVisible = false;
            const int scene = int(g.theme)*100 + g.stage*10 + int(g.phase);
            if (scene != animationScene || g.age < animationAge - .001f)
            {
                sailorVisuals.clear(); nextSailorKey = 4000000;
                animationScene = scene;
            }
            animationAge = g.age;
            animator.begin(g.age, scene, ui.paused || ui.menu || g.phase != Phase::Play);
            std::vector<std::uint64_t> sailorKeys;
            std::vector<SailorVisual> nextSailors;
            std::vector<bool> matched(sailorVisuals.size(), false);
            for (const auto& sailor : g.sailors)
            {
                int best = -1; float nearest = 48.f;
                for (int j = 0; j < int(sailorVisuals.size()); ++j)
                {
                    if (matched[j] || sailor.hp > sailorVisuals[j].hp + .1f) continue;
                    const float d = dist(sailor.p, sailorVisuals[j].position);
                    if (d < nearest) { nearest=d; best=j; }
                }
                const auto key = best < 0 ? nextSailorKey++ : sailorVisuals[best].key;
                if (best >= 0) matched[best] = true;
                sailorKeys.push_back(key); nextSailors.push_back({sailor.p,sailor.hp,key});
            }
            sailorVisuals = std::move(nextSailors);

            const bool islandTheme = g.theme == Theme::Island;

            sf::Color seaColor;
            sf::Color waveColor;

            if (g.wave == 1)
            {
                seaColor = islandTheme
                    ? sf::Color(48, 132, 151)
                    : sf::Color(35, 125, 160);
                waveColor = islandTheme
                    ? sf::Color(123, 205, 210)
                    : sf::Color(95, 190, 210);
            }
            else if (g.wave == 2)
            {
                seaColor = islandTheme
                    ? sf::Color(89, 83, 111)
                    : sf::Color(85, 72, 115);
                waveColor = islandTheme
                    ? sf::Color(171, 145, 157)
                    : sf::Color(145, 115, 155);
            }
            else
            {
                seaColor = islandTheme
                    ? sf::Color(19, 34, 50)
                    : sf::Color(10, 25, 50);
                waveColor = islandTheme
                    ? sf::Color(58, 83, 94)
                    : sf::Color(35, 65, 90);
            }

            r.clear(seaColor);

            drawPixelRect(
                r,
                stagePlate(islandTheme, g.wave),
                0.f,
                43.f,
                640.f,
                295.f
            );

            drawPixelRect(
                r,
                "ship/deck",
                DeckLeft - 10.f,
                44.f,
                DeckRight - DeckLeft + 20.f,
                292.f
            );

            drawPixel(
                r,
                "ship/fishing_post",
                FishingPoint.x,
                FishingPoint.y - 2.f,
                36.f,
                40.f
            );

            text(
                r,
                "FISHING",
                26.f,
                208.f,
                8,
                { 249, 220, 151 }
            );

            // 낚시터 장식 물고기: 왼쪽 바다에 서로 겹치지 않게 분산 배치
            // 각 물고기의 움직임 위상을 다르게 해서 한 덩어리처럼 보이지 않게.
            const float fishBobA = std::sin(g.age * 2.2f) * 1.4f;
            const float fishBobB = std::sin(g.age * 1.8f + 1.1f) * 1.3f;
            const float fishBobC = std::sin(g.age * 2.5f + 2.0f) * 1.1f;
            const float fishSwimA = std::sin(g.age * 1.25f) * 2.8f;
            const float fishSwimB = std::sin(g.age * 1.05f + 1.7f) * 2.2f;

            fishIcon(
                r, 6.f + fishSwimA, 128.f + fishBobA, 1.5f,
                sf::Color(104, 205, 224), sf::Color(134, 233, 245)
            );

            fishIcon(
                r, 27.f - fishSwimB, 146.f + fishBobB, 1.3f,
                sf::Color(244, 185, 96), sf::Color(251, 216, 144)
            );

            fishIcon(
                r, 5.f + fishSwimB * 0.5f, 169.f + fishBobC, 1.6f,
                sf::Color(173, 160, 236), sf::Color(200, 191, 246)
            );

            fishIcon(
                r, 18.f - fishSwimA * 0.35f, 191.f - fishBobA, 1.25f,
                sf::Color(112, 224, 181), sf::Color(157, 240, 204)
            );

            fishIcon(
                r, 4.f + fishSwimA * 0.6f, 216.f + fishBobB, 1.45f,
                sf::Color(238, 132, 156), sf::Color(250, 172, 190)
            );

            fishIcon(
                r, 28.f - fishSwimB * 0.45f, 238.f - fishBobC, 1.25f,
                sf::Color(111, 186, 238), sf::Color(157, 215, 249)
            );

            fishIcon(
                r, 10.f + fishSwimA * 0.35f, 261.f + fishBobC, 1.35f,
                sf::Color(245, 202, 105), sf::Color(255, 225, 146)
            );

            drawPixel(
                r,
                "ship/ammo_crate",
                AmmoPoint.x,
                AmmoPoint.y - 2.f,
                48.f,
                38.f
            );

            text(
                r,
                "AMMO",
                AmmoPoint.x - 14.f,
                AmmoPoint.y + 14.f,
                9,
                { 249, 220, 151 }
            );

            for (const auto& e :
                g.enemies)
            {
                V p = e.p;

                const char* enemyKey = nullptr;

                if (islandTheme)
                {
                    enemyKey = e.type == 0
                        ? "enemies/goblin_raider"
                        : e.type == 1
                        ? "enemies/red_beast"
                        : "enemies/rock_warrior";
                }
                else
                {
                    enemyKey = e.type == 0
                        ? "enemies/ghost_sailor"
                        : e.type == 1
                        ? "enemies/sea_raider"
                        : "enemies/armored_corsair";
                }

                const float enemyWidth =
                    e.type == 2 ? 52.f : e.type == 1 ? 44.f : 38.f;

                visual::Sample enemySample;
                enemySample.x=e.p.x; enemySample.y=e.p.y; enemySample.hp=e.hp;
                enemySample.cooldown=e.cooldown; enemySample.initialFacing=-1;
                enemySample.rate=e.freeze > 0 ? .4f : 1.f;
                const auto enemyPose=animator.observe(1000000 + e.id, enemySample);
                const auto enemyRig = !islandTheme ? visual::Rig::Floating :
                    e.type == 1 ? visual::Rig::Beast : visual::Rig::Humanoid;
                drawAnimated(r, enemyKey, p.x, p.y - 4.f, enemyWidth,
                    e.type == 2 ? 48.f : 42.f, enemyPose, enemyRig);

                if (e.freeze > 0.f)
                {
                    float w =
                        e.type == 2
                        ? 42.f
                        : e.type == 1
                        ? 24.f
                        : 30.f;

                    box(
                        r,
                        p.x - w / 2.f,
                        p.y + 8.f,
                        w,
                        3.f,
                        { 117, 222, 242 }
                    );
                }

                if (e.burn > 0.f)
                {
                    drawPixel(r, "effects/fire", p.x + 7.f, p.y - 5.f, 16, 22);
                }

                float hpWidth =
                    e.type == 2
                    ? 40.f
                    : e.type == 1
                    ? 24.f
                    : 30.f;

                bar(
                    r,
                    p.x - hpWidth / 2.f,
                    p.y + 22.f,
                    hpWidth,
                    e.hp / e.maxHp,
                    e.type == 2
                    ? sf::Color(
                        155,
                        165,
                        170
                    )
                    : e.type == 1
                    ? sf::Color(
                        235,
                        90,
                        75
                    )
                    : sf::Color(
                        210,
                        90,
                        70
                    )
                );
            }

            for (const auto& s :
                g.sailors)
            {
                if (s.hp <= 0.f)
                    continue;

                float drawX = s.p.x;
                float drawY = s.p.y;

                if (s.boarding > 0.f)
                {
                    float t =
                        std::clamp(
                            s.boarding /
                            1.2f,
                            0.f,
                            1.f
                        );

                    drawX +=
                        35.f * t;
                }

                visual::Sample sailorSample;
                sailorSample.x=s.p.x; sailorSample.y=s.p.y; sailorSample.hp=s.hp;
                sailorSample.cooldown=s.cooldown; sailorSample.windup=s.windup > 0;
                sailorSample.locked=s.boarding > 0; sailorSample.initialFacing=-1;
                const auto sailorIndex=std::size_t(&s - g.sailors.data());
                const auto sailorPose=animator.observe(sailorKeys[sailorIndex], sailorSample);
                drawAnimated(r, islandTheme ? "enemies/goblin_raider" : "enemies/ghost_sailor",
                    drawX, drawY-3.f, 34,38,sailorPose,visual::Rig::Humanoid);

                // 유령 선원이 C 근접공격으로 처치 가능한 대상임을
                // 바로 알 수 있도록 머리 위 HP바를 선명하게 표시
                const float hpRatio =
                    std::clamp(
                        s.hp / 60.f,
                        0.f,
                        1.f
                    );

                box(
                    r,
                    drawX - 13.f,
                    drawY - 22.f,
                    26.f,
                    5.f,
                    sf::Color(18, 24, 29, 230)
                );

                const sf::Color ghostHpColor =
                    hpRatio > 0.55f
                    ? sf::Color(88, 224, 135)
                    : hpRatio > 0.25f
                    ? sf::Color(244, 197, 82)
                    : sf::Color(238, 91, 82);

                box(
                    r,
                    drawX - 12.f,
                    drawY - 21.f,
                    24.f * hpRatio,
                    3.f,
                    ghostHpColor
                );

                if (s.windup > 0.f)
                {
                    box(
                        r,
                        s.strike.x - 13.f,
                        s.strike.y - 2.f,
                        26.f,
                        4.f,
                        sf::Color(
                            255,
                            70,
                            70,
                            180
                        )
                    );

                    box(
                        r,
                        s.strike.x - 2.f,
                        s.strike.y - 13.f,
                        4.f,
                        26.f,
                        sf::Color(
                            255,
                            70,
                            70,
                            180
                        )
                    );

                    if (
                        static_cast<int>(
                            g.age * 10.f
                            )
                        % 2 == 0
                        )
                    {
                        box(
                            r,
                            drawX - 9.f,
                            drawY - 15.f,
                            18.f,
                            25.f,
                            sf::Color(
                                255,
                                60,
                                60,
                                55
                            )
                        );
                    }
                }
            }

            if (g.greatWave.active)
            {
                if (g.greatWave.warning > 0.f)
                {
                    if (islandTheme)
                    {
                        // 섬 테마: 큰 파도 판정을 강풍 경고로 표현
                        box(r, DeckRight - 18.f, DeckTop, 18.f,
                            DeckBottom - DeckTop,
                            sf::Color(218, 188, 121, 80));

                        if (static_cast<int>(g.age * 8.f) % 2 == 0)
                        {
                            box(r, DeckRight - 30.f, DeckTop, 30.f,
                                DeckBottom - DeckTop,
                                sf::Color(235, 218, 166, 65));
                        }

                        text(r, "STRONG WIND!",
                            DeckRight - 112.f, DeckTop + 18.f, 10,
                            sf::Color(250, 222, 151));
                        text(r, "HOLD C",
                            DeckRight - 91.f, DeckTop + 32.f, 9,
                            sf::Color(248, 239, 205));
                    }
                    else
                    {
                        box(r, DeckRight - 18.f, DeckTop, 18.f,
                            DeckBottom - DeckTop,
                            sf::Color(80, 180, 255, 90));

                        if (static_cast<int>(g.age * 8.f) % 2 == 0)
                        {
                            box(r, DeckRight - 28.f, DeckTop, 28.f,
                                DeckBottom - DeckTop,
                                sf::Color(120, 210, 255, 75));
                        }

                        text(r, "BIG WAVE!",
                            DeckRight - 100.f, DeckTop + 18.f, 10,
                            sf::Color(190, 235, 255));
                        text(r, "HOLD C",
                            DeckRight - 91.f, DeckTop + 32.f, 9,
                            sf::Color(230, 245, 255));
                    }
                }

                if (g.greatWave.impacting)
                {
                    const float x = g.greatWave.front;

                    for (float y = DeckTop + 26.f; y < DeckBottom; y += 58.f)
                    {
                        drawPixel(
                            r,
                            islandTheme ? "effects/wind" : "effects/wave",
                            x,
                            y,
                            islandTheme ? 50.f : 54.f,
                            islandTheme ? 36.f : 42.f,
                            sf::Color(255, 255, 255, 220)
                        );
                    }
                }
            }

            for (const auto& t :
                g.tentacles)
            {
                if (t.hp <= 0.f)
                    continue;

                if (islandTheme)
                {
                    // 섬 테마에서는 크라켄 촉수를 고대 나무의 뿌리로 표시
                    if (t.warning > 0.f)
                    {
                        box(r, t.p.x - 18.f, t.p.y - 4.f, 36.f, 8.f,
                            sf::Color(147, 104, 58, 90));
                        box(r, t.p.x - 4.f, t.p.y - 18.f, 8.f, 36.f,
                            sf::Color(147, 104, 58, 90));
                        if (static_cast<int>(g.age * 10.f) % 2 == 0)
                        {
                            box(r, t.p.x - 13.f, t.p.y - 13.f, 26.f, 26.f,
                                sf::Color(105, 147, 67, 60));
                        }
                        continue;
                    }

                    for (int j = 0; j < 8; ++j)
                    {
                        const V a = t.segmentPoint(j, g.age);
                        const V b = t.segmentPoint(j + 1, g.age);
                        const V middle{ (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
                        const float width = 11.f - j * 0.7f;
                        box(r, middle.x - width * 0.5f, middle.y - 5.f,
                            width, 10.f, sf::Color(109, 73, 41, 240));
                        box(r, middle.x - width * 0.25f, middle.y - 3.f,
                            width * 0.5f, 5.f, sf::Color(126, 102, 55, 220));
                        if (j % 2 == 0)
                            box(r, middle.x + 2.f, middle.y - 6.f, 5.f, 3.f,
                                sf::Color(69, 118, 57, 220));
                    }

                    box(r, t.p.x - 10.f, t.p.y - 5.f, 20.f, 10.f,
                        sf::Color(78, 55, 35, 235));

                    const float rootHp = std::clamp(t.hp / 90.f, 0.f, 1.f);
                    box(r, t.p.x - 15.f, t.p.y + 11.f, 30.f, 4.f,
                        sf::Color(35, 30, 27, 220));
                    box(r, t.p.x - 15.f, t.p.y + 11.f, 30.f * rootHp, 4.f,
                        sf::Color(112, 175, 76, 230));

                    if (t.slamTimer <= 0.75f && t.slamAnimation <= 0.f)
                    {
                        const V hit = t.slamPoint();
                        box(r, hit.x - 18.f, hit.y - 3.f, 36.f, 6.f,
                            sf::Color(235, 111, 64, 145));
                        box(r, hit.x - 3.f, hit.y - 18.f, 6.f, 36.f,
                            sf::Color(235, 111, 64, 145));
                    }
                    continue;
                }

                if (t.warning > 0.f)
                {
                    box(
                        r,
                        t.p.x - 18.f,
                        t.p.y - 4.f,
                        36.f,
                        8.f,
                        sf::Color(
                            180,
                            70,
                            210,
                            80
                        )
                    );

                    box(
                        r,
                        t.p.x - 4.f,
                        t.p.y - 18.f,
                        8.f,
                        36.f,
                        sf::Color(
                            180,
                            70,
                            210,
                            80
                        )
                    );

                    if (
                        static_cast<int>(
                            g.age * 10.f
                            )
                        % 2 == 0
                        )
                    {
                        box(
                            r,
                            t.p.x - 13.f,
                            t.p.y - 13.f,
                            26.f,
                            26.f,
                            sf::Color(
                                220,
                                90,
                                230,
                                55
                            )
                        );
                    }

                    continue;
                }

                for (
                    int j = 0;
                    j < 8;
                    ++j
                    )
                {
                    V a =
                        t.segmentPoint(
                            j,
                            g.age
                        );

                    V b =
                        t.segmentPoint(
                            j + 1,
                            g.age
                        );

                    V middle{
                        (
                            a.x +
                            b.x
                        ) * 0.5f,

                        (
                            a.y +
                            b.y
                        ) * 0.5f
                    };

                    float width =
                        11.f -
                        j * 0.7f;

                    box(
                        r,
                        middle.x -
                        width * 0.5f,
                        middle.y - 5.f,
                        width,
                        10.f,
                        sf::Color(
                            112,
                            62,
                            145,
                            235
                        )
                    );

                    box(
                        r,
                        middle.x -
                        width * 0.25f,
                        middle.y - 3.f,
                        width * 0.5f,
                        5.f,
                        sf::Color(
                            166,
                            91,
                            183,
                            220
                        )
                    );
                }

                box(
                    r,
                    t.p.x - 10.f,
                    t.p.y - 5.f,
                    20.f,
                    10.f,
                    sf::Color(
                        72,
                        48,
                        100,
                        230
                    )
                );

                float tentacleHp =
                    std::clamp(
                        t.hp / 90.f,
                        0.f,
                        1.f
                    );

                box(
                    r,
                    t.p.x - 15.f,
                    t.p.y + 11.f,
                    30.f,
                    4.f,
                    sf::Color(
                        35,
                        30,
                        45,
                        220
                    )
                );

                box(
                    r,
                    t.p.x - 15.f,
                    t.p.y + 11.f,
                    30.f *
                    tentacleHp,
                    4.f,
                    sf::Color(
                        195,
                        90,
                        200,
                        230
                    )
                );

                if (
                    t.slamTimer <=
                    0.75f &&
                    t.slamAnimation <=
                    0.f
                    )
                {
                    V hit =
                        t.slamPoint();

                    box(
                        r,
                        hit.x - 18.f,
                        hit.y - 3.f,
                        36.f,
                        6.f,
                        sf::Color(
                            255,
                            70,
                            90,
                            150
                        )
                    );

                    box(
                        r,
                        hit.x - 3.f,
                        hit.y - 18.f,
                        6.f,
                        36.f,
                        sf::Color(
                            255,
                            70,
                            90,
                            150
                        )
                    );

                    if (
                        static_cast<int>(
                            g.age * 12.f
                            )
                        % 2 == 0
                        )
                    {
                        box(
                            r,
                            t.p.x - 12.f,
                            t.p.y - 55.f,
                            24.f,
                            60.f,
                            sf::Color(
                                255,
                                80,
                                110,
                                45
                            )
                        );
                    }
                }
            }

            if (g.boss.active)
            {
                V p = g.boss.p;

                const char* bossKey = nullptr;

                if (islandTheme)
                {
                    bossKey = g.boss.type == BossType::GhostShip
                        ? "bosses/stone_golem"
                        : g.boss.type == BossType::Leviathan
                        ? "bosses/storm_wyvern"
                        : "bosses/ancient_treant";
                }
                else
                {
                    bossKey = g.boss.type == BossType::GhostShip
                        ? "bosses/ghost_ship"
                        : g.boss.type == BossType::Leviathan
                        ? "bosses/leviathan"
                        : "bosses/kraken";
                }

                visual::Sample bossSample;
                bossSample.x=p.x; bossSample.y=p.y; bossSample.hp=g.boss.hp;
                bossSample.cooldown=g.boss.cooldown; bossSample.windup=g.boss.warning > 0;
                const auto bossPose=animator.observe(3000000 + int(g.boss.type), bossSample);
                drawAnimated(r,bossKey,p.x,p.y-8.f,
                    g.boss.type == BossType::GhostShip ? 122.f : 108.f,96,
                    bossPose,visual::Rig::Boss);

                if (g.boss.warning > 0.f)
                {
                    const char* cue = nullptr;
                    if (islandTheme)
                    {
                        cue = g.boss.type == BossType::GhostShip ? "ROCK BARRAGE!" :
                            g.boss.type == BossType::Leviathan ? "GALE VOLLEY!" : "ROOT BURST!";
                    }
                    else
                    {
                        cue = g.boss.type == BossType::GhostShip ? "BROADSIDE!" :
                            g.boss.type == BossType::Leviathan ? "WATER VOLLEY!" : "INK VOLLEY!";
                    }
                    text(r, cue, std::clamp(p.x - 50.f, 344.f, 530.f), p.y - 65.f, 10, { 255, 172, 90 });
                }
            }

            for (
                int i = 0;
                i <
                static_cast<int>(
                    g.cannons.size()
                    );
                ++i
                )
            {
                const auto& c =
                    g.cannons[i];

                const float x = c.p.x;
                const float y = c.p.y;
                const float aimAngle = std::atan2(
                    std::sin(c.angle), c.side * std::cos(c.angle));
                const V aimDirection{
                    std::cos(aimAngle), std::sin(aimAngle)
                };

                float firePhase = -1.f;
                for (const auto& effect : g.effects)
                {
                    if (effect.kind == 0 && effect.maxLife > 0.f &&
                        dist(effect.p, c.p) < 2.f)
                    {
                        const float phase = std::clamp(
                            1.f - effect.life / effect.maxLife, 0.f, 1.f);
                        if (firePhase < 0.f || phase < firePhase) firePhase = phase;
                    }
                }
                const float recoil = cannonVisual::recoil(firePhase);

                auto part = [&](const cannonVisual::PixelRect& q, bool rotating) {
                    const sf::Color color(
                        static_cast<std::uint8_t>((q.rgb >> 16) & 255),
                        static_cast<std::uint8_t>((q.rgb >> 8) & 255),
                        static_cast<std::uint8_t>(q.rgb & 255));
                    auto point = [&](float px, float py) -> V {
                        if (!rotating) return {std::round(x + px), std::round(y + py)};
                        px -= recoil;
                        return {std::round(x + aimDirection.x * px - aimDirection.y * py),
                                std::round(y + aimDirection.y * px + aimDirection.x * py)};
                    };
                    const V a = point(q.x, q.y), b = point(q.x + q.w, q.y);
                    const V c1 = point(q.x + q.w, q.y + q.h), d = point(q.x, q.y + q.h);
                    for (V p : {a,b,c1,a,c1,d}) rectangles.push_back({p,color,{0,0}});
                };
                for (const auto& q : cannonVisual::Carriage) part(q, false);
                for (const auto& q : cannonVisual::Barrel) part(q, true);
                box(r, x - 3.f, y - 3.f, 6.f, 6.f, {39,34,36});
                box(r, x - 2.f, y - 2.f, 4.f, 4.f, {219,170,91});
                box(r, x - 1.f, y - 2.f, 2.f, 1.f, {255,226,154});

                if (firePhase >= 0.f && firePhase < 0.22f)
                {
                    const V muzzle = c.p + aimDirection * (cannonVisual::MuzzleReach - recoil);
                    const std::uint8_t alpha = static_cast<std::uint8_t>(
                        255.f * (1.f - firePhase / 0.22f));
                    box(r, muzzle.x - 2.f, muzzle.y - 2.f, 4.f, 4.f,
                        sf::Color(255, 239, 166, alpha));
                    box(r, muzzle.x - 5.f, muzzle.y - 1.f, 10.f, 2.f,
                        sf::Color(247, 170, 54, alpha));
                    box(r, muzzle.x - 1.f, muzzle.y - 5.f, 2.f, 10.f,
                        sf::Color(247, 170, 54, alpha));
                }

                if (c.ammo >= 0)
                {
                    static const char* ammoKeys[] = {
                        "ammo/normal", "ammo/spread", "ammo/pierce",
                        "ammo/blast", "ammo/flame", "ammo/heavy"
                    };

                    drawPixel(
                        r,
                        ammoKeys[std::clamp(c.ammo, 0, 5)],
                        x - 8.f,
                        y - 8.f,
                        8.f,
                        8.f
                    );
                }

                text(
                    r,

                    std::to_string(
                        i + 1
                    )
                    +
                    (
                        c.ammo >= 0
                        ? " READY"
                        : ""
                        ),

                    x - 22.f,
                    y + 15.f,
                    8,

                    c.ammo >= 0
                    ? sf::Color(
                        246,
                        205,
                        92
                    )
                    : sf::Color(
                        165,
                        151,
                        126
                    )
                );

                if (c.ammo < 0)
                {
                    const sf::Color slotEdge(132, 116, 91, 210);
                    const sf::Color slotInside(34, 31, 29, 185);
                    box(r, x + 8.f, y + 16.f, 9.f, 6.f, slotEdge);
                    box(r, x + 9.f, y + 17.f, 7.f, 4.f, slotInside);
                }

                if (
                    c.progress >
                    0.f
                    )
                {
                    bar(
                        r,
                        x - 15.f,
                        y - 18.f,
                        30.f,
                        c.progress,
                        { 244, 212, 111 }
                    );
                }

                int localId =
                    std::clamp(
                        ui.local,
                        0,
                        MaxPlayers - 1
                    );

                if (
                    g.players[
                        localId
                    ].active
                    &&
                            g.nearCannon(
                                g.players[
                                    localId
                                ]
                            )
                            == i
                            &&
                            c.ammo >= 0
                            )
                {
                    for (
                        int k = 0;
                        k < 8;
                        ++k
                        )
                    {
                        box(
                            r,

                            x +
                            c.side *
                            std::cos(
                                c.angle
                            )
                            *
                            (
                                25.f +
                                k * 10.f
                                ),

                            y +
                            std::sin(
                                c.angle
                            )
                            *
                            (
                                25.f +
                                k * 10.f
                                ),

                            2.f,
                            2.f,
                            { 232, 218, 135 }
                        );
                    }
                }
            }

            for (const auto& h :
                g.hazards)
            {
                auto p = h.p;

                drawPixel(
                    r,
                    h.fire
                    ? "effects/fire"
                    : "ship/broken_deck",
                    p.x,
                    p.y - 3.f,
                    h.fire ? 26.f : 40.f,
                    h.fire ? 32.f : 40.f
                );

                if (h.progress > 0.f)
                {
                    drawPixel(
                        r,
                        "effects/repair",
                        p.x,
                        p.y - 11.f,
                        24.f,
                        24.f,
                        sf::Color(255, 255, 255, 210)
                    );
                }

                if (h.progress > 0.f)
                {
                    bar(
                        r,
                        p.x - 12.f,
                        p.y - 18.f,
                        24.f,
                        h.progress /
                        (
                            h.fire
                            ? 1.5f
                            : 2.f
                            ),
                        { 147, 219, 161 }
                    );
                }
            }

            for (const auto& d :
                g.drops)
            {
                ball(
                    r,
                    d.p,
                    d.type
                );
            }

            for (const auto& s :
                g.shots)
            {
                static const char* shotKeys[] = {"ammo/normal", "ammo/spread", "ammo/pierce",
                    "ammo/blast", "ammo/flame", "ammo/heavy"};
                const char* shotKey = shotKeys[std::clamp(s.type, 0, 5)];
                if (s.hostile)
                {
                    const sf::Color fill = s.black
                        ? sf::Color(7, 8, 11)
                        : sf::Color(252, 252, 248);
                    const sf::Color outline = s.black
                        ? sf::Color(250, 250, 246)
                        : sf::Color(6, 7, 10);

                    drawPixel(r, shotKey, s.p.x - 1.f, s.p.y, 10.f, 10.f, outline);
                    drawPixel(r, shotKey, s.p.x + 1.f, s.p.y, 10.f, 10.f, outline);
                    drawPixel(r, shotKey, s.p.x, s.p.y - 1.f, 10.f, 10.f, outline);
                    drawPixel(r, shotKey, s.p.x, s.p.y + 1.f, 10.f, 10.f, outline);
                    drawPixel(r, shotKey, s.p.x, s.p.y, 8.f, 8.f, fill);
                }
                else
                {
                    drawPixel(r, shotKey, s.p.x, s.p.y, 10.f, 10.f,
                        sf::Color::White);
                }
            }

            std::array<int, MaxPlayers> playerOrder{0,1,2,3,4};
            std::stable_sort(playerOrder.begin(), playerOrder.end(), [&](int a, int b) {
                return g.players[a].p.y < g.players[b].p.y;
            });
            for (int i : playerOrder)
            {
                const auto& p =
                    g.players[i];

                if (!p.active)
                    continue;

                const auto& c =
                    g.crew[i];

                visual::Sample playerSample;
                playerSample.x=p.p.x; playerSample.y=p.p.y; playerSample.hp=c.hp;
                playerSample.attacking=c.swing > 0;
                playerSample.attack=1.f-c.swing/.18f;
                playerSample.locked=p.fishing || c.bracing || c.revive > 0;
                const auto playerPose=animator.observe(100 + i,playerSample);
                V v = p.p;
                const float nameY = v.y < 96.f ? v.y + 14.f : v.y - 39.f;
                const float healthY = v.y < 96.f ? v.y + 25.f : v.y - 31.f;

                if (c.hp <= 0.f)
                {
                    drawAnimated(r, "players/player" + std::to_string(i + 1),
                        v.x, v.y - 9.f, 32, 38, playerPose, visual::Rig::Humanoid);
                    text(
                        r,
                        "DOWN",
                        v.x - 12.f,
                        v.y - 14.f,
                        8,
                        { 255, 75, 75 }
                    );

                    const std::string downName =
                        !ui.playerNames[i].empty()
                        ? ui.playerNames[i]
                        : "플레이어";

                    text(
                        r,
                        downName,
                        v.x,
                        nameY,
                        8,
                        { 150, 150, 150 },
                        true
                    );

                    continue;
                }

                if (
                    p.fishing &&
                    p.fishProgress > 0.f
                    )
                {
                    bar(
                        r,
                        v.x - 14.f,
                        v.y - 44.f,
                        28.f,
                        p.fishProgress,
                        { 115, 215, 235 }
                    );
                }

                if (p.fishCatchFx > 0.f)
                {
                    const float rise = (1.f - p.fishCatchFx) * 28.f;
                    const std::uint8_t alpha = static_cast<std::uint8_t>(70.f + 185.f * p.fishCatchFx);
                    const sf::Color body(110, 218, 238, alpha);
                    const sf::Color fin(170, 242, 250, alpha);
                    const sf::Color txt(240, 252, 255, alpha);

                    drawPixel(
                        r,
                        "effects/healing_fish",
                        v.x,
                        v.y - 51.f - rise,
                        30.f,
                        24.f,
                        sf::Color(255, 255, 255, alpha)
                    );

                    text(
                        r,
                        "+" + std::to_string(std::max(1, p.fishCatchAmount)) + " FISH",
                        v.x - 18.f,
                        v.y - 66.f - rise,
                        8,
                        txt
                    );
                }

                static const char* playerKeys[] = {
                    "players/player1",
                    "players/player2",
                    "players/player3",
                    "players/player4",
                    "players/player5"
                };

                const bool blink = c.invulnerable > 0 && c.revive <= 0 &&
                    int(g.age * 12.f) % 2 == 0 && playerPose.motion != visual::Motion::Hurt;
                const V carryingHand = drawAnimated(r,playerKeys[i],v.x,v.y-9.f,32,38,
                    playerPose,visual::Rig::Humanoid,
                    sf::Color(255,255,255,blink ? 150 : 255));

                const std::string playerName =
                    !ui.playerNames[i].empty()
                    ? ui.playerNames[i]
                    : "플레이어";

                text(
                    r,
                    playerName,
                    v.x,
                    nameY,
                    8,
                    sf::Color(249, 235, 205),
                    true
                );

                if (p.held >= 0)
                {
                    ball(
                        r,
                        {
                            carryingHand.x,
                            carryingHand.y
                        },
                        p.held
                    );
                }

                if (i == ui.local)
                {
                    box(
                        r,
                        v.x - 5.f,
                        v.y + 12.f,
                        10.f,
                        1.f,
                        { 245, 226, 171 }
                    );
                }

                float hpRatio =
                    c.maxHp > 0.f
                    ? std::clamp(
                        c.hp / c.maxHp,
                        0.f,
                        1.f
                    )
                    : 0.f;

                box(
                    r,
                    v.x - 13.f,
                    healthY,
                    26.f,
                    4.f,
                    sf::Color(
                        35,
                        35,
                        35,
                        220
                    )
                );

                box(
                    r,
                    v.x - 13.f,
                    healthY,
                    26.f * hpRatio,
                    4.f,
                    hpRatio <= 0.30f
                    ? sf::Color(
                        230,
                        80,
                        70,
                        230
                    )
                    : sf::Color(
                        90,
                        220,
                        110,
                        230
                    )
                );

                if (c.revive > 0.f)
                {
                    box(
                        r,
                        v.x - 12.f,
                        v.y - 16.f,
                        24.f,
                        30.f,
                        sf::Color(
                            20,
                            20,
                            25,
                            150
                        )
                    );

                    text(
                        r,

                        "REVIVE " +
                        std::to_string(
                            static_cast<int>(
                                std::ceil(
                                    c.revive
                                )
                                )
                        ),

                        v.x - 21.f,
                        v.y - 40.f,
                        7
                    );
                }

                if (c.bracing)
                {
                    box(
                        r,
                        v.x - 14.f,
                        v.y - 18.f,
                        3.f,
                        35.f,
                        sf::Color(
                            100,
                            210,
                            255,
                            180
                        )
                    );

                    box(
                        r,
                        v.x + 11.f,
                        v.y - 18.f,
                        3.f,
                        35.f,
                        sf::Color(
                            100,
                            210,
                            255,
                            180
                        )
                    );
                }
            }

            for (const auto& e :
                g.effects)
            {
                float f =
                    1.f -
                    e.life /
                    e.maxLife;

                const std::uint8_t effectAlpha =
                    static_cast<std::uint8_t>(
                        255.f * std::clamp(1.f - f * 0.75f, 0.f, 1.f)
                    );

                V effectPosition = e.p;
                if (e.kind == 0)
                {
                    for (const auto& cannon : g.cannons)
                    {
                        if (dist(e.p, cannon.p) < 2.f)
                        {
                            const float angle = std::atan2(
                                std::sin(cannon.angle),
                                cannon.side * std::cos(cannon.angle));
                            effectPosition = cannon.p +
                                V{std::cos(angle), std::sin(angle)} *
                                (cannonVisual::MuzzleReach - cannonVisual::recoil(f));
                            break;
                        }
                    }
                }

                const bool loadSpark = e.kind == 4;
                drawPixel(
                    r,
                    loadSpark
                    ? "effects/treasure_sparkle"
                    : e.kind == 0
                    ? "effects/smoke"
                    : e.kind == 2
                    ? "effects/heal"
                    : "effects/explosion",
                    effectPosition.x,
                    effectPosition.y,
                    loadSpark ? 12.f + f * 4.f : 24.f + f * 20.f,
                    loadSpark ? 12.f + f * 4.f : 24.f + f * 20.f,
                    loadSpark
                    ? sf::Color(255, 207, 92, effectAlpha)
                    : sf::Color(255, 255, 255, effectAlpha)
                );
            }

            if (g.wave == 2)
            {
                const float eveningProgress =
                    std::clamp(
                        1.f - g.time / WaveSeconds,
                        0.f,
                        1.f
                    );

                const std::uint8_t tintAlpha =
                    static_cast<std::uint8_t>(
                        18.f +
                        42.f * eveningProgress
                        );

                box(
                    r,
                    0.f,
                    43.f,
                    640.f,
                    295.f,
                    sf::Color(
                        55,
                        20,
                        70,
                        tintAlpha
                    )
                );
            }
            else if (g.wave >= 3)
            {
                box(
                    r,
                    0.f,
                    43.f,
                    640.f,
                    295.f,
                    sf::Color(
                        5,
                        10,
                        35,
                        68
                    )
                );
            }

            // 저녁부터 플레이어 주변 시야가 점점 좁아지고
            // 밤에는 로컬 플레이어 주변만 밝게 보인다.
            drawNightVision(r, g, ui);

            box(
                r,
                0.f,
                0.f,
                640.f,
                43.f,
                { 13, 27, 39 }
            );

            box(
                r,
                0.f,
                42.f,
                640.f,
                1.f,
                { 70, 103, 111 }
            );

            text(
                r,
                "DECKWATCH",
                12.f,
                4.f,
                15,
                { 248, 218, 155 }
            );

            text(
                r,

                std::string(g.theme == Theme::Island ? "섬 " : "바다 ") +
                "STAGE 0" +
                std::to_string(g.stage) +
                " / 03",

                13.f,
                25.f,
                9,
                { 132, 180, 189 }
            );

            text(
                r,

                "SHIP " +
                std::to_string(
                    static_cast<int>(
                        g.hp
                        )
                )
                +
                " / " +
                std::to_string(
                    static_cast<int>(
                        g.maxHp
                        )
                ),

                181.f,
                5.f,
                10
            );

            text(
                r,

                "FISH " +
                std::to_string(
                    g.fishCount
                ),

                122.f,
                25.f,
                9,
                g.fishCount > 0
                ? sf::Color(
                    120,
                    220,
                    240
                )
                : sf::Color(
                    140,
                    150,
                    155
                )
            );

            bar(
                r,
                182.f,
                25.f,
                150.f,
                g.hp / g.maxHp,
                g.hp < 250.f
                ? sf::Color(
                    234,
                    103,
                    85
                )
                : sf::Color(
                    105,
                    199,
                    138
                )
            );

            int seconds =
                static_cast<int>(
                    std::ceil(
                        g.time
                    )
                    );

            text(
                r,

                g.boss.active
                ? "BOSS FIGHT"
                :
                "TIME " +
                std::to_string(
                    seconds / 60
                )
                +
                ":"
                +
                (
                    seconds % 60 < 10
                    ? "0"
                    : ""
                    )
                +
                std::to_string(
                    seconds % 60
                ),

                354.f,
                5.f,
                12,
                { 249, 218, 151 }
            );

            std::string timeName;

            if (g.wave == 1)
            {
                timeName = "DAY";
            }
            else if (g.wave == 2)
            {
                timeName = "EVENING";
            }
            else if (g.wave == 3)
            {
                timeName = "NIGHT";
            }
            else
            {
                timeName = "DAY";
            }

            text(
                r,

                "WAVE " +
                std::to_string(
                    g.wave
                )
                +
                "/3  " +
                timeName +
                "  KILLS " +
                std::to_string(
                    g.kills
                ),

                354.f,
                25.f,
                9
            );

            int id =
                std::clamp(
                    ui.local,
                    0,
                    4
                );

            const auto& p =
                g.players[id];

            text(
                r,

                (
                    !ui.playerNames[id].empty()
                    ? ui.playerNames[id]
                    : "플레이어"
                    )
                +
                "  CREW " +
                std::to_string(
                    g.count()
                )
                +
                "/5",

                500.f,
                4.f,
                9,
                playerColor(id)
            );

            text(
                r,

                "GOLD " +
                std::to_string(
                    g.gold.getGold()
                )
                +
                "G",

                500.f,
                16.f,
                9,
                sf::Color(
                    249,
                    218,
                    151
                )
            );

            text(
                r,
                "소지 " + std::string(shellName(p.held)),
                500.f,
                28.f,
                8
            );

            if (g.boss.active)
            {
                panel(
                    r,
                    344.f,
                    46.f,
                    288.f,
                    28.f
                );

                text(
                    r,
                    bossDisplayName(g),
                    352.f,
                    49.f,
                    9,
                    { 210, 160, 232 }
                );

                bar(
                    r,
                    352.f,
                    64.f,
                    272.f,
                    g.boss.hp /
                    g.boss.maxHp,
                    { 186, 100, 181 }
                );
            }

            if (g.phase == Phase::Play)
            {
                panel(r, 344.f, 292.f, 288.f, 42.f);
                for (int n = 0; n < ShellCount; ++n)
                {
                    const float x = 351.f + n * 46.f;
                    if (p.selected == n) box(r, x - 2, 298.f, 43.f, 30.f, {53, 76, 78});
                    ball(r, {x + 6.f, 306.f}, n);
                    text(r, std::to_string(n + 1), x + 17.f, 300.f, 8, {249,218,155});
                    text(r, n == Normal ? "∞" : std::to_string(g.ammoCount(n)), x + 1.f, 316.f, 8);
                }
            }

            box(
                r,
                0.f,
                338.f,
                640.f,
                22.f,
                { 13, 27, 39 }
            );

            text(
                r,
                g.hint(id),
                12.f,
                344.f,
                10
            );

            text(
                r,

                ui.online
                ? (
                    ui.client
                    ? "TCP CLIENT"
                    : "TCP HOST"
                    )
                : "SOLO",

                555.f,
                344.f,
                9,
                { 151, 188, 197 }
            );

            if (
                g.phase ==
                Phase::Lobby &&
                !ui.menu
                )
            {
                panel(
                    r,
                    112.f,
                    90.f,
                    416.f,
                    174.f
                );

                text(
                    r,
                    "CREW ASSEMBLY",
                    143.f,
                    108.f,
                    23,
                    { 249, 218, 155 }
                );

                text(
                    r,

                    "Connected: " +
                    std::to_string(
                        g.count()
                    )
                    +
                    " / 5",

                    144.f,
                    146.f,
                    12
                );

                text(
                    r,
                    ui.status,
                    144.f,
                    168.f,
                    10
                );

                text(
                    r,
                    ui.address,
                    144.f,
                    186.f,
                    10,
                    { 142, 192, 201 }
                );

                text(
                    r,

                    ui.client
                    ? "Waiting for host to start..."
                    : "ENTER: depart with current crew",

                    144.f,
                    219.f,
                    12,
                    { 248, 218, 155 }
                );

                text(
                    r,
                    "ESC: return to main menu",
                    144.f,
                    241.f,
                    10
                );
            }

            if (
                g.phase ==
                Phase::Shop &&
                !ui.menu
                )
            {
                auto shopGold =
                    g.gold;

                auto shopAmmo =
                    g.ammo;

                auto shopUpgrades =
                    g.upgrades;

                ShopManager shop(
                    shopGold,
                    shopAmmo,
                    shopUpgrades
                );

                box(
                    r,
                    0.f,
                    0.f,
                    640.f,
                    360.f,
                    sf::Color(
                        6,
                        16,
                        25,
                        255
                    )
                );

                panel(
                    r,
                    22.f,
                    52.f,
                    596.f,
                    286.f
                );

                drawPixel(
                    r,
                    "ui/gold_coins",
                    205.f,
                    75.f,
                    24.f,
                    20.f
                );

                text(
                    r,
                    "선상 상점",
                    34.f,
                    62.f,
                    20,
                    sf::Color(
                        249,
                        218,
                        155
                    )
                );

                text(
                    r,

                    "팀 골드 " +
                    std::to_string(
                        g.gold.getGold()
                    )
                    +
                    "G",

                    220.f,
                    68.f,
                    13,
                    sf::Color(
                        249,
                        218,
                        155
                    )
                );

                text(
                    r,

                    "물고기 " +
                    std::to_string(
                        g.fishCount
                    ),

                    365.f,
                    68.f,
                    11,
                    sf::Color(
                        180,
                        225,
                        245
                    )
                );

                text(
                    r,

                    std::to_string(
                        static_cast<int>(
                            std::ceil(
                                g.shopRemaining
                            )
                            )
                    )
                    +
                    "초 남음",

                    505.f,
                    68.f,
                    13,
                    sf::Color(
                        249,
                        218,
                        155
                    )
                );

                text(
                    r,
                    "특수탄 구매 / 팀 강화",
                    34.f,
                    91.f,
                    10,
                    sf::Color(
                        150,
                        185,
                        193
                    )
                );

                const char*
                    itemNames[
                        ShopItemCount] =
                        {
                            "확산탄 +5발",
                            "관통탄 +4발",
                            "폭발탄 +3발",
                            "화염탄 +3발",
                            "중포탄 +2발",
                            "일반탄 공격력",
                            "배 최대 체력",
                            "장전 속도",
                            "수리 속도",
                            "낚시 속도"
                        };

                        for (
                            int i = 0;
                            i < ShopItemCount;
                            ++i
                            )
                        {
                            int col =
                                i % 5;

                            int row =
                                i / 5;

                            float x =
                                34.f +
                                col * 116.f;

                            float y =
                                108.f +
                                row * 74.f;

                            const auto item =
                                ShopItems[i];

                            int price =
                                shop.getPrice(
                                    item
                                );

                            bool maxLevel =
                                shop.isMaxLevel(
                                    item
                                );

                            int level =
                                shop.getUpgradeLevel(
                                    item
                                );

                            box(
                                r,
                                x,
                                y,
                                108.f,
                                64.f,
                                sf::Color(
                                    28,
                                    47,
                                    60
                                )
                            );

                            const bool hovered = ui.pointer.x >= x && ui.pointer.x < x + 108 &&
                                ui.pointer.y >= y && ui.pointer.y < y + 64;
                            if (hovered) box(r, x, y, 108, 64, {49, 69, 76});
                            if (i < 5)
                            {
                                static const char* shopAmmoKeys[] = {
                                    "ammo/spread", "ammo/pierce", "ammo/blast",
                                    "ammo/flame", "ammo/heavy"
                                };

                                drawPixel(
                                    r,
                                    shopAmmoKeys[i],
                                    x + 92.f,
                                    y + 47.f,
                                    20.f,
                                    20.f
                                );
                            }

                            const bool selected = g.purchaseVoteActive && g.purchaseVoteItem == i;
                            const bool available = !maxLevel && g.gold.getGold() >= price;
                            box(r, x, y, 108.f, 2.f, selected ? sf::Color(249,218,155) :
                                available ? sf::Color(100,159,139) : sf::Color(91,98,103));
                            text(r, maxLevel ? "최대 강화" : !available ? "골드 부족" :
                                i < 5 ? "특수 탄약 보충" : "팀 전체 적용", x + 5.f, y + 26.f, 8,
                                available ? sf::Color(167,197,190) : sf::Color(159,153,148));
                            text(
                                r,
                                itemNames[i],
                                x + 5.f,
                                y + 7.f,
                                10,
                                sf::Color(
                                    245,
                                    218,
                                    155
                                )
                            );

                            if (maxLevel)
                            {
                                text(
                                    r,
                                    "MAX",
                                    x + 5.f,
                                    y + 43.f,
                                    10,
                                    sf::Color(
                                        150,
                                        185,
                                        193
                                    )
                                );
                            }
                            else
                            {
                                text(
                                    r,

                                    std::to_string(
                                        price
                                    )
                                    +
                                    "G",

                                    x + 5.f,
                                    y + 43.f,
                                    10,
                                    sf::Color(
                                        245,
                                        218,
                                        155
                                    )
                                );
                            }

                            if (level >= 0)
                            {
                                text(
                                    r,

                                    "Lv." +
                                    std::to_string(
                                        level
                                    )
                                    +
                                    "/5",

                                    x + 58.f,
                                    y + 43.f,
                                    9,
                                    sf::Color(
                                        150,
                                        185,
                                        193
                                    )
                                );
                            }
                        }

                        if (
                            g.purchaseVoteActive &&
                            g.purchaseVoteItem >=
                            0 &&
                            g.purchaseVoteItem <
                            ShopItemCount
                            )
                        {
                            const char*
                                voteItemNames[
                                    ShopItemCount] =
                                    {
                                        "확산탄",
                                        "관통탄",
                                        "폭발탄",
                                        "화염탄",
                                        "중포탄",
                                        "일반탄 공격력",
                                        "배 최대 체력",
                                        "장전 속도",
                                        "수리 속도",
                                        "낚시 속도"
                                    };

                                    float yesWeight =
                                        g.purchaseVoteWeight(
                                            1
                                        );

                                    float noWeight =
                                        g.purchaseVoteWeight(
                                            0
                                        );

                                    auto voteWeightText = [](float value)
                                        {
                                            const int whole = static_cast<int>(value);
                                            return value - static_cast<float>(whole) > 0.25f
                                                ? std::to_string(whole) + ".5"
                                                : std::to_string(whole);
                                        };

                                    box(
                                        r,
                                        34.f,
                                        252.f,
                                        572.f,
                                        34.f,
                                        sf::Color(
                                            42,
                                            34,
                                            38
                                        )
                                    );

                                    text(
                                        r,
                                        std::string("구매 투표: ") +
                                        voteItemNames[g.purchaseVoteItem],
                                        42.f,
                                        258.f,
                                        10,
                                        sf::Color(249, 218, 155)
                                    );

                                    // 마우스로 직접 투표: 방장 1.5표 / 참가자 1표, 자동 찬성 없음
                                    box(r, 330.f, 255.f, 88.f, 26.f, sf::Color(44, 92, 62));
                                    box(r, 426.f, 255.f, 88.f, 26.f, sf::Color(102, 54, 54));

                                    text(
                                        r,
                                        "찬성 " + voteWeightText(yesWeight),
                                        374.f,
                                        261.f,
                                        10,
                                        sf::Color::White,
                                        true
                                    );

                                    text(
                                        r,
                                        "반대 " + voteWeightText(noWeight),
                                        470.f,
                                        261.f,
                                        10,
                                        sf::Color::White,
                                        true
                                    );

                                    text(
                                        r,
                                        std::to_string(
                                            static_cast<int>(
                                                std::ceil(g.purchaseVoteRemaining)
                                                )
                                        ) + "초",
                                        565.f,
                                        260.f,
                                        9,
                                        sf::Color(249, 218, 155)
                                    );
                        }

                        int localId =
                            ui.local;

                        if (
                            localId >= 0 &&
                            localId <
                            MaxPlayers &&
                            g.shopResultTime[
                                localId
                            ] > 0.f
                            )
                        {
                            std::string
                                resultMessage;

                            switch (
                                g.shopResult[
                                    localId
                                ]
                                )
                            {
                            case 1:
                                resultMessage =
                                    "구매 완료";
                                break;

                            case 2:
                                resultMessage =
                                    "골드 부족";
                                break;

                            case 3:
                                resultMessage =
                                    "이미 최대 강화";
                                break;

                            case 5:
                                resultMessage =
                                    "구매 투표 부결";
                                break;

                            default:
                                break;
                            }

                            if (
                                !resultMessage.empty()
                                )
                            {
                                text(
                                    r,
                                    resultMessage,
                                    430.f,
                                    91.f,
                                    10,
                                    sf::Color(
                                        249,
                                        218,
                                        155
                                    )
                                );
                            }
                        }

                        int readyCount = 0;

                        for (
                            int i = 0;
                            i < MaxPlayers;
                            ++i
                            )
                        {
                            if (
                                g.players[
                                    i
                                ].active &&
                                g.shopReady[
                                    i
                                ]
                                        )
                            {
                                ++readyCount;
                            }
                        }

                        text(
                            r,
                            "준비 " + std::to_string(readyCount) +
                            "/" + std::to_string(g.count()),
                            430.f,
                            291.f,
                            10,
                            sf::Color(245, 218, 155)
                        );

                        box(
                            r,
                            500.f,
                            286.f,
                            106.f,
                            28.f,
                            sf::Color(45, 82, 94)
                        );

                        text(
                            r,
                            "출항 준비",
                            553.f,
                            293.f,
                            10,
                            sf::Color::White,
                            true
                        );

                        text(
                            r,
                            "Enter: 출항 준비",
                            430.f,
                            319.f,
                            8,
                            sf::Color(150, 185, 193)
                        );
            }

            if (
                g.phase ==
                Phase::Reward &&
                !ui.menu
                )
            {
                panel(
                    r,
                    35.f,
                    90.f,
                    570.f,
                    188.f
                );

                text(
                    r,
                    "STAGE CLEAR - CHOOSE A CREW RELIC",
                    55.f,
                    104.f,
                    18,
                    { 249, 218, 155 }
                );

                text(
                    r,
                    "Each player votes. Majority wins; ties favor the left card.",
                    55.f,
                    131.f,
                    9,
                    { 151, 190, 197 }
                );

                const char*
                    names[] =
                {
                    "1  REINFORCED HULL",
                    "2  REPAIR KIT",
                    "3  WATER SEAL"
                };

                const char*
                    desc[] =
                {
                    "+150 max HP / heal 250",
                    "+25 repair heal / heal 160",
                    "35% less leaks / heal 160"
                };

                for (
                    int k = 0;
                    k < 3;
                    ++k
                    )
                {
                    box(
                        r,
                        50.f +
                        k * 180.f,
                        156.f,
                        170.f,
                        74.f,

                        p.vote == k
                        ? sf::Color(
                            50,
                            85,
                            80
                        )
                        : sf::Color(
                            28,
                            47,
                            60
                        )
                    );

                    text(
                        r,
                        names[k],
                        57.f +
                        k * 180.f,
                        168.f,
                        11,
                        { 246, 216, 148 }
                    );

                    text(
                        r,
                        desc[k],
                        57.f +
                        k * 180.f,
                        195.f,
                        9
                    );

                    int votes = 0;

                    for (
                        const auto& x :
                        g.players
                        )
                    {
                        if (
                            x.active &&
                            x.vote == k
                            )
                        {
                            ++votes;
                        }
                    }

                    text(
                        r,

                        "VOTES " +
                        std::to_string(
                            votes
                        ),

                        57.f +
                        k * 180.f,
                        213.f,
                        9
                    );
                }

                text(
                    r,
                    "Press 1, 2 or 3. No shop. Cannon damage / load time stay fixed.",
                    55.f,
                    247.f,
                    9
                );
            }

            if (!ui.menu && (g.phase == Phase::Won || g.phase == Phase::Lost))
            {
                if (g.phase == Phase::Won)
                {
                    drawPixelRect(r, "ending/treasure_victory", 0, 0, 640, 360);
                    panel(r, 96, 12, 448, 47);
                    text(r, "최종 승리 · 보물섬에 도착했습니다", 320, 20, 18, {249,218,155}, true);
                    text(r, "바다 3개 + 섬 3개 스테이지 완료", 320, 44, 9, {216,226,211}, true);
                    panel(r, 96, 298, 448, 50);
                    text(r, "함께 항해해 주셔서 감사합니다  ·  격퇴한 적 " + std::to_string(g.kills),
                        320, 305, 10, {249,218,155}, true);
                    text(r, ui.client ? "방장의 재시작을 기다리는 중  /  Esc 메인 메뉴" :
                        "Enter 처음부터 다시 항해  /  Esc 메인 메뉴", 320, 328, 10, sf::Color::White, true);
                }
                else
                {
                    box(r, 0, 0, 640, 360, {6,16,25,235});
                    panel(r, 115, 90, 410, 183);
                    text(r, "항해 종료", 320, 111, 27, {249,218,155}, true);
                    text(r, "배가 침몰했습니다. 선원들과 다시 도전하세요.", 320, 162, 11, sf::Color::White, true);
                    text(r, ui.client ? "방장의 재시작을 기다리는 중" : "Enter 다시 시작", 320, 210, 12, sf::Color::White, true);
                    text(r, "Esc 메인 메뉴", 320, 244, 10, sf::Color::White, true);
                }
            }

            if (ui.menu)
            {
                panel(
                    r,
                    90.f,
                    60.f,
                    460.f,
                    250.f
                );

                text(
                    r,
                    "해상 방어전",
                    123.f,
                    78.f,
                    25,
                    { 249, 218, 155 }
                );

                text(
                    r,
                    "총 3개 스테이지 / 하나의 배 / 최대 5명 협동",
                    123.f,
                    115.f,
                    12,
                    { 151, 193, 202 }
                );

                text(
                    r,
                    "1   혼자 시작하기",
                    123.f,
                    143.f,
                    16
                );

                text(
                    r,
                    "2   방 만들기",
                    123.f,
                    167.f,
                    16
                );

                text(
                    r,
                    "3   다른 사람의 방에 접속",
                    123.f,
                    191.f,
                    16
                );

                box(
                    r,
                    121.f,
                    221.f,
                    396.f,
                    22.f,

                    ui.editing
                    ? sf::Color(
                        60,
                        80,
                        80
                    )
                    : sf::Color(
                        28,
                        45,
                        58
                    )
                );

                const std::string
                    addressText =
                    std::string(
                        ui.editing
                        ? "> "
                        : "접속 주소: "
                    )
                    +
                    ui.endpoint
                    +
                    (
                        ui.editing
                        ? "_"
                        : ""
                        );

                text(
                    r,
                    addressText,
                    128.f,
                    225.f,
                    12,
                    { 242, 216, 168 }
                );

                text(
                    r,
                    "Tab 주소 수정 / Enter 입력 완료 / F1 조작법",
                    123.f,
                    250.f,
                    11
                );

                text(
                    r,
                    ui.status,
                    123.f,
                    273.f,
                    10,
                    { 239, 164, 137 }
                );

                text(
                    r,
                    "F1 조작법 / Esc·F10 메인 메뉴",
                    123.f,
                    292.f,
                    11,
                    { 151, 190, 197 }
                );
            }

            if (
                ui.help ||
                ui.paused
                )
            {
                box(r, 0, 0, 640, 360, {6,16,25,255});
                panel(
                    r,
                    25.f,
                    18.f,
                    590.f,
                    318.f
                );

                text(
                    r,
                    ui.paused ? "일시정지" : "최종 조작법",
                    45.f,
                    30.f,
                    20,
                    { 249, 218, 155 }
                );

                text(r, "WASD 이동  /  Q 포탄 내려놓기", 45.f, 65.f, 10);
                text(r, "1~6 탄약 선택: 1일반  2확산  3관통  4폭발  5화염  6중포", 45.f, 87.f, 9);
                text(r, "탄약고: 선택 후 E로 포탄 획득", 45.f, 109.f, 10);
                text(r, "대포: E 길게 장전 → 마우스 조준 → Space 발사", 45.f, 131.f, 10);
                text(r, "파손·화재: 해당 위치에서 E 길게 수리·소화", 45.f, 153.f, 10);
                text(r, "낚시터: E 시작/취소  /  H 물고기 사용(체력 회복)", 45.f, 175.f, 10);
                text(r, "C 근접 공격  /  큰 파도·강풍은 C 길게 눌러 방어", 45.f, 197.f, 10);
                text(r, "상점: 마우스 구매·찬반 투표  /  Enter 출항 준비", 45.f, 219.f, 10);

                if (ui.online)
                    text(r, "채팅: Tab 입력 시작 → Enter 전송 → Tab 입력 종료", 45.f, 241.f, 10, { 151, 205, 213 });
                else
                    text(r, "싱글 플레이는 도움말을 열면 일시정지됩니다.", 45.f, 241.f, 10, { 151, 205, 213 });

                text(r, "F1 도움말 열기/닫기", 45.f, 268.f, 10, { 248, 218, 155 });
                text(r, "Esc 또는 F10: 메인 메뉴로 복귀", 45.f, 290.f, 10, { 248, 218, 155 });
                text(r, "진행: 바다 Stage 1~3 → 섬 Stage 1~3", 45.f, 314.f, 9, { 151, 190, 197 });
            }

            // 멀티 채팅: 항상 왼쪽 아래에 표시하고, Tab은 입력 활성화만 전환합니다.
            // 하단 조작 바(y=338~360)와 겹치지 않도록 입력창을 y=322에 고정합니다.
            if (!ui.menu && ui.online && ui.chatVisible && !ui.help && !ui.paused &&
                (g.phase == Phase::Play || g.phase == Phase::Shop))
            {
                const bool shopChat = g.phase == Phase::Shop;
                const float chatX = shopChat ? 34.f : 8.f;
                const float chatY = shopChat ? 288.f : 250.f;
                const float chatW = shopChat ? 380.f : 225.f;
                gChatWidth = chatW - 14.f;
                panel(r, chatX, chatY, chatW, shopChat ? 44.f : 84.f);
                text(
                    r,
                    ui.chatActive ? "채팅 입력 중  [Enter 전송 / Tab 종료]"
                    : "채팅  [Tab 입력]",
                    chatX + 6.f,
                    chatY + 4.f,
                    8,
                    ui.chatActive
                    ? sf::Color(249, 218, 155)
                    : sf::Color(170, 205, 213)
                );

                const std::size_t maxLines = shopChat ? 1 : 4;
                const std::size_t begin =
                    ui.chatLines.size() > maxLines
                    ? ui.chatLines.size() - maxLines
                    : 0;

                float y = chatY + 17.f;
                for (std::size_t i = begin; i < ui.chatLines.size(); ++i)
                {
                    rawText(
                        r,
                        ui.chatLines[i],
                        chatX + 5.f,
                        y + 1.f,
                        9,
                        sf::Color(0, 0, 0, 220)
                    );

                    rawText(
                        r,
                        ui.chatLines[i],
                        chatX + 4.f,
                        y,
                        9,
                        sf::Color(245, 245, 240)
                    );

                    y += 12.f;
                }

                const std::string inputLine =
                    ui.chatActive
                    ? "> " + ui.chatInput + "_"
                    : "> " + ui.chatInput;

                rawText(
                    r,
                    inputLine,
                    chatX + 5.f,
                    chatY + (shopChat ? 30.f : 73.f),
                    9,
                    sf::Color(0, 0, 0, 230)
                );

                rawText(
                    r,
                    inputLine,
                    chatX + 4.f,
                    chatY + (shopChat ? 29.f : 72.f),
                    9,
                    ui.chatActive
                    ? sf::Color(249, 218, 155)
                    : sf::Color(170, 190, 198)
                );
            }

            if (
                !ui.menu &&
                ui.client &&
                ui.status.find(
                    "disconnected"
                )
                != std::string::npos
                )
            {
                panel(
                    r,
                    80.f,
                    132.f,
                    480.f,
                    90.f
                );

                text(
                    r,
                    "HOST DISCONNECTED",
                    100.f,
                    148.f,
                    20,
                    { 249, 168, 135 }
                );

                text(
                    r,
                    "Room closed. ESC returns to the menu.",
                    100.f,
                    185.f,
                    12
                );
            }

            flush(r);
        }
    };

    class Audio
    {
        std::array<
            sf::SoundBuffer,
            3>
            buffers;

        std::array<
            std::unique_ptr<
            sf::Sound>,
            3>
            sounds;

        std::uint32_t fired = 0;
        std::uint32_t hit = 0;

        Phase last =
            Phase::Lobby;

    public:
        bool muted = false;

        Audio()
        {
            for (
                int k = 0;
                k < 3;
                ++k
                )
            {
                int n =
                    k == 2
                    ? 11025
                    : 4410;

                std::vector<
                    std::int16_t>
                    samples(n);

                std::uint32_t
                    seed = 827;

                for (
                    int i = 0;
                    i < n;
                    ++i
                    )
                {
                    float t =
                        i / 22050.f;

                    float envelope =
                        1.f -
                        static_cast<float>(
                            i
                            )
                        / n;

                    seed =
                        seed *
                        1664525 +
                        1013904223;

                    float noise =
                        static_cast<float>(
                            (
                                seed >> 16
                                )
                            &
                            65535
                            )
                        / 32768.f
                        - 1.f;

                    float tone =
                        std::sin(
                            2.f *
                            Pi *
                            (
                                k == 0
                                ? 100.f
                                : k == 1
                                ? 70.f
                                : 440.f
                                )
                            *
                            t
                        );

                    samples[i] =
                        static_cast<
                        std::int16_t>(
                            (
                                k < 2
                                ?
                                0.65f *
                                noise +
                                0.35f *
                                tone
                                :
                                tone
                                )
                            *
                            envelope
                            *
                            7000.f
                            );
                }

                if (
                    buffers[k]
                    .loadFromSamples(
                        samples.data(),
                        samples.size(),
                        1,
                        22050,
                        {
                            sf::SoundChannel::Mono
                        }
                    )
                    )
                {
                    sounds[k] =
                        std::make_unique<
                        sf::Sound>(
                            buffers[k]
                        );

                    sounds[k]
                        ->setVolume(
                            20.f
                        );
                }
            }
        }

        void update(
            const Game& g)
        {
            if (!muted)
            {
                if (
                    g.shotsFired >
                    fired &&
                    sounds[0]
                    )
                {
                    sounds[0]->play();
                }

                if (
                    g.impacts >
                    hit &&
                    sounds[1]
                    )
                {
                    sounds[1]->play();
                }

                if (
                    g.phase != last &&
                    (
                        g.phase ==
                        Phase::Reward ||
                        g.phase ==
                        Phase::Won
                        )
                    &&
                    sounds[2]
                    )
                {
                    sounds[2]->play();
                }
            }

            fired =
                g.shotsFired;

            hit =
                g.impacts;

            last =
                g.phase;
        }
    };
}
