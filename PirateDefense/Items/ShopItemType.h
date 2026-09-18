#pragma once

// 상점에서 판매하는 10종
enum class ShopItemType
{
    // 특수탄
    SpreadAmmo,
    PiercingAmmo,
    ExplosiveAmmo,
    FireAmmo,
    HeavyAmmo,

    // 강화
    NormalDamageUp,
    ShipHpUp,
    ReloadTimeDown,
    RepairTimeDown,
    FishingSpeedUp
};