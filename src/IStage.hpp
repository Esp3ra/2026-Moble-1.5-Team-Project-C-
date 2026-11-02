#pragma once

#include "Common.hpp"

#include <string_view>
#include <vector>

namespace sf { class RenderTarget; }

namespace devil
{
// 조원이 만드는 StageX 클래스의 공통 계약입니다.
// 맵 배치와 함정 상태만 StageX가 담당하고, 플레이어 물리는 Game이 담당합니다.
class IStage
{
public:
    virtual ~IStage() = default;

    virtual std::string_view label() const = 0;
    virtual Vec2 spawnPoint() const = 0;
    // 대부분의 방은 기존 천장 높이를 사용하고, 세로형 방만 화면 위쪽까지 확장합니다.
    virtual float ceilingY() const { return GameConfig::ceiling; }
    virtual void reset() = 0;

    // 좌우 이동 뒤, 중력 계산 전에 호출됩니다. 발판/문/함정 상태를 갱신합니다.
    // 움직이는 벽처럼 플레이어를 직접 밀어내는 스테이지도 처리할 수 있습니다.
    virtual void update(PlayerState& player, float dt) = 0;
    virtual const std::vector<RectF>& solids() const = 0;
    virtual bool touchesHazard(const RectF& playerBounds) const = 0;
    // 문에 자동 진입을 시작할 위치와 진입 애니메이션의 도착점을 제공합니다.
    virtual bool isAtGoal(const PlayerState& player) const = 0;
    virtual Vec2 goalPoint() const = 0;

    // 배경, 발판, 함정, 문을 그립니다. 플레이어와 HUD는 공통 렌더러가 그립니다.
    virtual void draw(sf::RenderTarget& target) const = 0;
};
}
