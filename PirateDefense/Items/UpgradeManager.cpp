#include "UpgradeManager.h"


UpgradeManager::UpgradeManager()
{
    reset();
}


void UpgradeManager::reset()
{
    normalDamageLevel = 0;
    shipHpLevel = 0;
    reloadLevel = 0;
    repairLevel = 0;
    fishingLevel = 0;
}


// 각 강화는 최대 5단계입니다. 여기서는 레벨만 올리고 결제는 상점에서 합니다.
// 일반탄 데미지 +10%
bool UpgradeManager::upgradeNormalDamage()
{
    if (normalDamageLevel >= 5)
        return false;

    normalDamageLevel++;

    return true;
}


// 배 최대체력 +100
bool UpgradeManager::upgradeShipHp()
{
    if (shipHpLevel >= 5)
        return false;

    shipHpLevel++;

    return true;
}


// 장전시간 -10%
bool UpgradeManager::upgradeReload()
{
    if (reloadLevel >= 5)
        return false;

    reloadLevel++;

    return true;
}


// 수리시간 -10%
bool UpgradeManager::upgradeRepair()
{
    if (repairLevel >= 5)
        return false;

    repairLevel++;

    return true;
}


// 낚시속도 +10%
bool UpgradeManager::upgradeFishing()
{
    if (fishingLevel >= 5)
        return false;

    fishingLevel++;

    return true;
}


int UpgradeManager::getNormalDamageLevel() const
{
    return normalDamageLevel;
}


int UpgradeManager::getShipHpLevel() const
{
    return shipHpLevel;
}


int UpgradeManager::getReloadLevel() const
{
    return reloadLevel;
}


int UpgradeManager::getRepairLevel() const
{
    return repairLevel;
}


int UpgradeManager::getFishingLevel() const
{
    return fishingLevel;
}


// Lv1 = 1.1
// Lv5 = 1.5
float UpgradeManager::getNormalDamageMultiplier() const
{
    return 1.0f +
        normalDamageLevel * 0.1f;
}


// Lv1 = +100
// Lv5 = +500
int UpgradeManager::getShipHpBonus() const
{
    return shipHpLevel * 100;
}


// Lv1 = 0.9
// Lv5 = 0.5
float UpgradeManager::getReloadMultiplier() const
{
    return 1.0f -
        reloadLevel * 0.1f;
}


// 수리와 소화 시간에 함께 적용합니다. 5단계에서는 원래 시간의 절반입니다.
float UpgradeManager::getRepairMultiplier() const
{
    return 1.0f -
        repairLevel * 0.1f;
}


// Lv1 = 1.1
// Lv5 = 1.5
float UpgradeManager::getFishingSpeedMultiplier() const
{
    return 1.0f +
        fishingLevel * 0.1f;
}
