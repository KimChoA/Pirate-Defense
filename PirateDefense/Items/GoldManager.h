#pragma once

// 팀 공용 골드 관리
class GoldManager
{
private:
    int teamGold;

public:
    GoldManager();

    // 골드 획득
    void addGold(int amount);

    // 골드 사용
    bool spendGold(int amount);

    // 구매할 수 있는지
    bool canAfford(int price) const;

    // 현재 골드
    int getGold() const;

    // 0G 초기화
    void reset();
};