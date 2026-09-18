#pragma once

#include "CannonballType.h"

// 팀 공용 포탄 수량 관리
class AmmoManager
{
private:
    int spreadCount;
    int piercingCount;
    int explosiveCount;
    int fireCount;
    int heavyCount;

public:
    AmmoManager();

    // 포탄 추가
    void addAmmo(CannonballType type, int amount);

    // 포탄 1발 사용
    bool useAmmo(CannonballType type);

    // 현재 포탄 개수
    int getAmmoCount(CannonballType type) const;

    // 사용 가능한지
    bool canUse(CannonballType type) const;

    // 초기화
    void reset();
};