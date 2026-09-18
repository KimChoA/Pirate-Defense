#include "GoldManager.h"


GoldManager::GoldManager()
{
    teamGold = 0;
}


// 처치·수리·소화 보상과 물고기 판매금을 팀 공용 골드에 더합니다.
void GoldManager::addGold(int amount)
{
    if (amount <= 0)
        return;

    teamGold += amount;
}


// 잔액이 충분할 때만 차감하고, 구매 성공 여부를 돌려줍니다.
bool GoldManager::spendGold(int amount)
{
    if (amount <= 0)
        return false;

    if (teamGold < amount)
        return false;

    teamGold -= amount;

    return true;
}


bool GoldManager::canAfford(int price) const
{
    return teamGold >= price;
}


int GoldManager::getGold() const
{
    return teamGold;
}


void GoldManager::reset()
{
    teamGold = 0;
}
