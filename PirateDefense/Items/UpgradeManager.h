#pragma once

// 팀 공용 강화 관리
class UpgradeManager
{
private:
    int normalDamageLevel;
    int shipHpLevel;
    int reloadLevel;
    int repairLevel;
    int fishingLevel;

public:
    UpgradeManager();

    // 강화
    bool upgradeNormalDamage();
    bool upgradeShipHp();
    bool upgradeReload();
    bool upgradeRepair();
    bool upgradeFishing();

    // 레벨
    int getNormalDamageLevel() const;
    int getShipHpLevel() const;
    int getReloadLevel() const;
    int getRepairLevel() const;
    int getFishingLevel() const;

    // 실제 게임에서 다른 담당자가 사용할 값
    float getNormalDamageMultiplier() const;

    int getShipHpBonus() const;

    float getReloadMultiplier() const;

    float getRepairMultiplier() const;

    float getFishingSpeedMultiplier() const;

    // 초기화
    void reset();
};