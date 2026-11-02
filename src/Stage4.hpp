#pragma once

#include "IStage.hpp"

namespace devil
{
    struct Stage4Layout
    {
        static constexpr float floor = 360.f;
        static constexpr float spawnX = 148.f;

        // 낭떠러지
        static constexpr float leftHoleX = 220.f;
        static constexpr float leftHoleWidth = 56.f;

        // 낭떠러지 오른쪽에서 올라왔다 내려가는 벽
        static constexpr float risingBlockGap = 0.f;
        static constexpr float risingBlockWidth = 48.f;
        static constexpr float risingBlockHeight = 110.f;
        static constexpr float risingBlockSpeed = 380.f;

        // 시작할 때 왼쪽에서 오는 작은 블록
        static constexpr float startBlockWidth = 28.f;
        static constexpr float startBlockHeight = 40.f;
        static constexpr float startBlockSpeed = 500.f;

        // 오른쪽에서 날아오는 큰 블록
        static constexpr float blockTriggerX = 620.f;
        static constexpr float blockRestX = 480.f;
        static constexpr float blockWidth = 50.f;
        static constexpr float blockHeight = 110.f;
        static constexpr float blockApproachSpeed = 300.f;
        static constexpr float blockReturnSpeed = 1200.f;

        // 문
        static constexpr float doorX = 820.f;
        static constexpr float doorWidth = 32.f;
        static constexpr float doorHeight = 36.f;

        // 문 위치를 바꾸면 함께 이동하는 숨겨진 낭떠러지.
        static constexpr float doorHoleWidth = 64.f;
        static constexpr float doorHoleGap = 24.f;
        static constexpr float doorHoleX = doorX - doorHoleGap - doorHoleWidth;
        static constexpr float doorHoleDelay = .12f;
        static constexpr float doorFloorFallSpeed = 900.f;

        // 오른쪽 큰 블록 복귀 후 문 왼쪽에서 내려오는 블록
        static constexpr float fallingBlockX = 650.f;
        static constexpr float fallingBlockWidth = 50.f;
        static constexpr float fallingBlockHeight = 100.f;
        static constexpr float fallingBlockStartY = 150.f;
        // 초당 이동 거리: 낙하는 접근 중인 플레이어를 맞힐 수 있도록 조정.
        static constexpr float fallingBlockSpeed = 900.f;
        static constexpr float fallingBlockReturnSpeed = 550.f;
    };

    class Stage4 final : public IStage
    {
    public:
        Stage4();

        std::string_view label() const override
        {
            return "4";
        }

        Vec2 spawnPoint() const override;
        void reset() override;
        void update(PlayerState& player, float dt) override;

        const std::vector<RectF>& solids() const override
        {
            return solids_;
        }

        bool touchesHazard(const RectF& playerBounds) const override;
        bool isAtGoal(const PlayerState& player) const override;
        Vec2 goalPoint() const override;
        void draw(sf::RenderTarget& target) const override;

    private:
        // 시작 블록
        bool startBlockActive_ = true;
        float startBlockX_ =
            GameConfig::left - Stage4Layout::startBlockWidth;

        // 0: 대기, 1: 상승, 2: 하강, 3: 종료
        int risingBlockState_ = 0;
        float risingBlockHeight_ = 0.f;

        // 오른쪽에서 날아오는 큰 블록
        bool blockTriggered_ = false;
        int blockState_ = 0;
        float movingBlockX_ = GameConfig::right;

        // 문 왼쪽에서 내려오는 블록
        bool fallingBlockTriggered_ = false;
        bool fallingBlockLanded_ = false;
        float fallingBlockY_ =
            Stage4Layout::fallingBlockStartY;

        bool doorHoleTriggered_ = false;
        bool doorHoleOpen_ = false;
        float doorHoleTimer_ = 0.f;
        float doorFloorDrop_ = 0.f;

        std::vector<RectF> solids_;

        void rebuildSolids();

        RectF startBlockBounds() const;
        RectF risingBlockBounds() const;
        RectF movingBlockBounds() const;
        RectF fallingBlockBounds() const;
    };
}
