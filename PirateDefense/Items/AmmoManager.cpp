#include "AmmoManager.h"


AmmoManager::AmmoManager()
{
    reset();
}


// 구매하거나 회수한 특수탄을 종류별 재고에 더합니다.
void AmmoManager::addAmmo(CannonballType type, int amount)
{
    if (amount <= 0)
        return;

    switch (type)
    {
    case CannonballType::Spread:
        spreadCount += amount;
        break;

    case CannonballType::Piercing:
        piercingCount += amount;
        break;

    case CannonballType::Explosive:
        explosiveCount += amount;
        break;

    case CannonballType::Fire:
        fireCount += amount;
        break;

    case CannonballType::Heavy:
        heavyCount += amount;
        break;

    case CannonballType::Normal:
        // 일반탄은 무한
        break;
    }
}


// 탄약고에서 가져갈 때 특수탄 1발을 차감합니다. 재고가 없으면 실패합니다.
bool AmmoManager::useAmmo(CannonballType type)
{
    // 일반탄은 무한
    if (type == CannonballType::Normal)
        return true;

    switch (type)
    {
    case CannonballType::Spread:
        if (spreadCount > 0)
        {
            spreadCount--;
            return true;
        }
        break;

    case CannonballType::Piercing:
        if (piercingCount > 0)
        {
            piercingCount--;
            return true;
        }
        break;

    case CannonballType::Explosive:
        if (explosiveCount > 0)
        {
            explosiveCount--;
            return true;
        }
        break;

    case CannonballType::Fire:
        if (fireCount > 0)
        {
            fireCount--;
            return true;
        }
        break;

    case CannonballType::Heavy:
        if (heavyCount > 0)
        {
            heavyCount--;
            return true;
        }
        break;

    case CannonballType::Normal:
        return true;
    }

    return false;
}


int AmmoManager::getAmmoCount(CannonballType type) const
{
    switch (type)
    {
    case CannonballType::Spread:
        return spreadCount;

    case CannonballType::Piercing:
        return piercingCount;

    case CannonballType::Explosive:
        return explosiveCount;

    case CannonballType::Fire:
        return fireCount;

    case CannonballType::Heavy:
        return heavyCount;

    case CannonballType::Normal:
        // -1 = 무한
        return -1;
    }

    return 0;
}


bool AmmoManager::canUse(CannonballType type) const
{
    if (type == CannonballType::Normal)
        return true;

    return getAmmoCount(type) > 0;
}


void AmmoManager::reset()
{
    spreadCount = 0;
    piercingCount = 0;
    explosiveCount = 0;
    fireCount = 0;
    heavyCount = 0;
}
