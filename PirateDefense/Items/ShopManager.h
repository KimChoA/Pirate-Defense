#pragma once

#include <string>

#include "GoldManager.h"
#include "AmmoManager.h"
#include "UpgradeManager.h"
#include "ShopItemType.h"


class ShopManager
{
private:
    GoldManager& goldManager;
    AmmoManager& ammoManager;
    UpgradeManager& upgradeManager;

public:
  
    static constexpr bool UseTestPrices = false;

    ShopManager(
        GoldManager& goldManager,
        AmmoManager& ammoManager,
        UpgradeManager& upgradeManager
    );

    // 아이템 구매
    bool buyItem(ShopItemType type);

    // 현재 가격
    int getPrice(ShopItemType type) const;

    // 강화 MAX 여부
    bool isMaxLevel(ShopItemType type) const;

    // 현재 강화 레벨
    int getUpgradeLevel(ShopItemType type) const;

    // 상품 이름
    std::wstring getItemName(ShopItemType type) const;

    // 상품 설명
    std::wstring getItemDescription(ShopItemType type) const;
};
