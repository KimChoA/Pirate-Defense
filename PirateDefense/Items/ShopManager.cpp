#include "ShopManager.h"


ShopManager::ShopManager(
    GoldManager& goldManager,
    AmmoManager& ammoManager,
    UpgradeManager& upgradeManager
)
    : goldManager(goldManager),
    ammoManager(ammoManager),
    upgradeManager(upgradeManager)
{
}


// 현재 가격
int ShopManager::getPrice(ShopItemType type) const
{
    // 테스트 가격이어도 최대 강화 상태는 구매 불가(-1)로 유지합니다.
    if (UseTestPrices && type >= ShopItemType::SpreadAmmo && type <= ShopItemType::FishingSpeedUp)
        return isMaxLevel(type) ? -1 : 1;

    switch (type)
    {
        // ========================
        // 특수탄 - 가격 고정
        // ========================

    case ShopItemType::SpreadAmmo:
        return 100;

    case ShopItemType::PiercingAmmo:
        return 120;

    case ShopItemType::ExplosiveAmmo:
        return 150;

    case ShopItemType::FireAmmo:
        return 130;

    case ShopItemType::HeavyAmmo:
        return 120;


        // ========================
        // 강화 - 레벨마다 +100G
        // ========================

    case ShopItemType::NormalDamageUp:

        if (upgradeManager.getNormalDamageLevel() >= 5)
            return -1;

        return (upgradeManager.getNormalDamageLevel() + 1) * 100;


    case ShopItemType::ShipHpUp:

        if (upgradeManager.getShipHpLevel() >= 5)
            return -1;

        return (upgradeManager.getShipHpLevel() + 1) * 100;


    case ShopItemType::ReloadTimeDown:

        if (upgradeManager.getReloadLevel() >= 5)
            return -1;

        return (upgradeManager.getReloadLevel() + 1) * 100;


    case ShopItemType::RepairTimeDown:

        if (upgradeManager.getRepairLevel() >= 5)
            return -1;

        return (upgradeManager.getRepairLevel() + 1) * 100;


    case ShopItemType::FishingSpeedUp:

        if (upgradeManager.getFishingLevel() >= 5)
            return -1;

        return (upgradeManager.getFishingLevel() + 1) * 100;
    }

    return 0;
}


int ShopManager::getUpgradeLevel(ShopItemType type) const
{
    switch (type)
    {
    case ShopItemType::NormalDamageUp:
        return upgradeManager.getNormalDamageLevel();

    case ShopItemType::ShipHpUp:
        return upgradeManager.getShipHpLevel();

    case ShopItemType::ReloadTimeDown:
        return upgradeManager.getReloadLevel();

    case ShopItemType::RepairTimeDown:
        return upgradeManager.getRepairLevel();

    case ShopItemType::FishingSpeedUp:
        return upgradeManager.getFishingLevel();

    default:
        // 특수탄은 레벨 개념 없음
        return -1;
    }
}


bool ShopManager::isMaxLevel(ShopItemType type) const
{
    int level =
        getUpgradeLevel(type);

    // 특수탄
    if (level == -1)
        return false;

    return level >= 5;
}


// 골드와 강화 한도를 확인한 뒤, 탄약 지급 또는 강화와 결제를 처리합니다.
bool ShopManager::buyItem(ShopItemType type)
{
    // MAX 강화 구매 방지
    if (isMaxLevel(type))
        return false;


    int price =
        getPrice(type);


    if (price < 0)
        return false;


    // 골드 부족
    if (!goldManager.canAfford(price))
        return false;


    switch (type)
    {
        // =========================
        // 특수탄
        // =========================

    case ShopItemType::SpreadAmmo:

        if (!goldManager.spendGold(price))
            return false;

        ammoManager.addAmmo(
            CannonballType::Spread,
            5
        );

        return true;


    case ShopItemType::PiercingAmmo:

        if (!goldManager.spendGold(price))
            return false;

        ammoManager.addAmmo(
            CannonballType::Piercing,
            4
        );

        return true;


    case ShopItemType::ExplosiveAmmo:

        if (!goldManager.spendGold(price))
            return false;

        ammoManager.addAmmo(
            CannonballType::Explosive,
            3
        );

        return true;


    case ShopItemType::FireAmmo:

        if (!goldManager.spendGold(price))
            return false;

        ammoManager.addAmmo(
            CannonballType::Fire,
            3
        );

        return true;


    case ShopItemType::HeavyAmmo:

        if (!goldManager.spendGold(price))
            return false;

        ammoManager.addAmmo(
            CannonballType::Heavy,
            2
        );

        return true;


        // =========================
        // 강화
        // =========================

    case ShopItemType::NormalDamageUp:

        if (!upgradeManager.upgradeNormalDamage())
            return false;

        goldManager.spendGold(price);

        return true;


    case ShopItemType::ShipHpUp:

        if (!upgradeManager.upgradeShipHp())
            return false;

        goldManager.spendGold(price);

        return true;


    case ShopItemType::ReloadTimeDown:

        if (!upgradeManager.upgradeReload())
            return false;

        goldManager.spendGold(price);

        return true;


    case ShopItemType::RepairTimeDown:

        if (!upgradeManager.upgradeRepair())
            return false;

        goldManager.spendGold(price);

        return true;


    case ShopItemType::FishingSpeedUp:

        if (!upgradeManager.upgradeFishing())
            return false;

        goldManager.spendGold(price);

        return true;
    }

    return false;
}


std::wstring ShopManager::getItemName(
    ShopItemType type
) const
{
    switch (type)
    {
    case ShopItemType::SpreadAmmo:
        return L"확산탄 +5";

    case ShopItemType::PiercingAmmo:
        return L"관통탄 +4";

    case ShopItemType::ExplosiveAmmo:
        return L"폭발탄 +3";

    case ShopItemType::FireAmmo:
        return L"화염탄 +3";

    case ShopItemType::HeavyAmmo:
        return L"중포탄 +2";

    case ShopItemType::NormalDamageUp:
        return L"일반탄 데미지";

    case ShopItemType::ShipHpUp:
        return L"배 최대 체력";

    case ShopItemType::ReloadTimeDown:
        return L"장전시간 감소";

    case ShopItemType::RepairTimeDown:
        return L"수리시간 감소";

    case ShopItemType::FishingSpeedUp:
        return L"낚시속도 증가";
    }

    return L"";
}


std::wstring ShopManager::getItemDescription(
    ShopItemType type
) const
{
    switch (type)
    {
    case ShopItemType::SpreadAmmo:
        return L"확산탄 5발 획득";

    case ShopItemType::PiercingAmmo:
        return L"관통탄 4발 획득";

    case ShopItemType::ExplosiveAmmo:
        return L"폭발탄 3발 획득";

    case ShopItemType::FireAmmo:
        return L"화염탄 3발 획득";

    case ShopItemType::HeavyAmmo:
        return L"중포탄 2발 획득";

    case ShopItemType::NormalDamageUp:
        return L"기본 데미지 +10%";

    case ShopItemType::ShipHpUp:
        return L"최대/현재 HP +100";

    case ShopItemType::ReloadTimeDown:
        return L"장전시간 -10%";

    case ShopItemType::RepairTimeDown:
        return L"수리시간 -10%";

    case ShopItemType::FishingSpeedUp:
        return L"낚시속도 +10%";
    }

    return L"";
}
