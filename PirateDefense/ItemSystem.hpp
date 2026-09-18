#pragma once
#include "Items/ShopManager.h"
#include <array>

namespace dw
{
    // 게임의 포탄 번호를 아이템 시스템의 종류로 연결(일반·확산·관통·폭발·화염·중포).
    inline CannonballType itemAmmo(int shell)
    {
        constexpr CannonballType types[] = { CannonballType::Normal, CannonballType::Spread, CannonballType::Piercing,
            CannonballType::Explosive, CannonballType::Fire, CannonballType::Heavy };
        return shell >= 0 && shell < 6 ? types[shell] : CannonballType::Normal;
    }
    struct ShellStats { float speed, damage; };
    inline ShellStats shellStats(int shell)
    {
        // 포탄 속도는 기존 값 260을 유지하고, 종류별 피해량만 다르게 설정
        constexpr ShellStats stats[] = { {260,30}, {260,12}, {260,25}, {260,40}, {260,20}, {260,70} };
        return stats[shell >= 0 && shell < 6 ? shell : 0];
    }
    // 물고기 1마리를 사용했을 때의 개인 HP 회복량
    constexpr float FishHealAmount = 20.f;
    constexpr int ShopItemCount = 10;
    // 상점 카드 순서: 특수탄 5종 다음에 강화 5종을 표시
    inline constexpr std::array<ShopItemType, ShopItemCount> ShopItems{
        ShopItemType::SpreadAmmo, ShopItemType::PiercingAmmo, ShopItemType::ExplosiveAmmo,
        ShopItemType::FireAmmo, ShopItemType::HeavyAmmo,
        ShopItemType::NormalDamageUp, ShopItemType::ShipHpUp, ShopItemType::ReloadTimeDown,
        ShopItemType::RepairTimeDown, ShopItemType::FishingSpeedUp
    };
}
