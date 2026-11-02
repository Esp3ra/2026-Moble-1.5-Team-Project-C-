#include "Stage4.hpp"
#include "RenderCommon.hpp"

#include <algorithm>

namespace devil
{
    Stage4::Stage4()
    {
        reset();
    }

    Vec2 Stage4::spawnPoint() const
    {
        return {
            Stage4Layout::spawnX,
            Stage4Layout::floor - GameConfig::playerHeight
        };
    }

    void Stage4::rebuildSolids()
    {
        const float depth =
            GameConfig::height - Stage4Layout::floor + 40.f;

        solids_.clear();

        // 낭떠러지 왼쪽 바닥
        solids_.push_back({
            GameConfig::left,
            Stage4Layout::floor,
            Stage4Layout::leftHoleX - GameConfig::left,
            depth
            });

        // 문 앞 함정이 열리면 실제 충돌 바닥도 두 조각으로 나눔.
        const float rightFloorX =
            Stage4Layout::leftHoleX + Stage4Layout::leftHoleWidth;
        if (doorHoleOpen_)
        {
            solids_.push_back({rightFloorX, Stage4Layout::floor,
                Stage4Layout::doorHoleX - rightFloorX, depth});
            const float holeEnd = Stage4Layout::doorHoleX + Stage4Layout::doorHoleWidth;
            solids_.push_back({holeEnd, Stage4Layout::floor,
                GameConfig::right - holeEnd, depth});
            if (doorFloorDrop_ < depth)
                solids_.push_back({Stage4Layout::doorHoleX,
                    Stage4Layout::floor + doorFloorDrop_,
                    Stage4Layout::doorHoleWidth, depth});
        }
        else
        {
            solids_.push_back({rightFloorX, Stage4Layout::floor,
                GameConfig::right - rightFloorX, depth});
        }

        // 올라오는 벽의 충돌 영역
        if (risingBlockHeight_ > 0.f)
            solids_.push_back(risingBlockBounds());

        // 낙하 블록은 착지 후 바로 올라가므로 고정 벽으로 등록하지 않음
    }

    void Stage4::reset()
    {
        startBlockActive_ = true;
        startBlockX_ =
            GameConfig::left - Stage4Layout::startBlockWidth;

        risingBlockState_ = 0;
        risingBlockHeight_ = 0.f;

        blockTriggered_ = false;
        blockState_ = 0;
        movingBlockX_ = GameConfig::right;

        fallingBlockTriggered_ = false;
        fallingBlockLanded_ = false;
        fallingBlockY_ = Stage4Layout::fallingBlockStartY;

        doorHoleTriggered_ = false;
        doorHoleOpen_ = false;
        doorHoleTimer_ = 0.f;
        doorFloorDrop_ = 0.f;

        rebuildSolids();
    }

    RectF Stage4::startBlockBounds() const
    {
        return {
            startBlockX_,
            Stage4Layout::floor - Stage4Layout::startBlockHeight,
            Stage4Layout::startBlockWidth,
            Stage4Layout::startBlockHeight
        };
    }

    RectF Stage4::risingBlockBounds() const
    {
        return {
            Stage4Layout::leftHoleX + Stage4Layout::leftHoleWidth
                + Stage4Layout::risingBlockGap,
            Stage4Layout::floor - risingBlockHeight_,
            Stage4Layout::risingBlockWidth,
            risingBlockHeight_
        };
    }

    RectF Stage4::movingBlockBounds() const
    {
        return {
            movingBlockX_,
            Stage4Layout::floor - Stage4Layout::blockHeight,
            Stage4Layout::blockWidth,
            Stage4Layout::blockHeight
        };
    }

    RectF Stage4::fallingBlockBounds() const
    {
        return {
            Stage4Layout::fallingBlockX,
            fallingBlockY_,
            Stage4Layout::fallingBlockWidth,
            Stage4Layout::fallingBlockHeight
        };
    }

    void Stage4::update(PlayerState& player, float dt)
    {
        // 바닥을 실제로 밟았을 때만 발동. 점프로 넘으면 발동하지 않음.
        if (!doorHoleTriggered_ && player.grounded &&
            player.feet() >= Stage4Layout::floor - 1.f &&
            player.feet() <= Stage4Layout::floor + 1.f &&
            player.centerX() >= Stage4Layout::doorHoleX &&
            player.centerX() < Stage4Layout::doorHoleX + Stage4Layout::doorHoleWidth)
        {
            doorHoleTriggered_ = true;
            doorHoleTimer_ = Stage4Layout::doorHoleDelay;
        }
        else if (doorHoleTriggered_ && !doorHoleOpen_)
        {
            doorHoleTimer_ = std::max(0.f, doorHoleTimer_ - dt);
            if (doorHoleTimer_ <= 0.f)
            {
                doorHoleOpen_ = true;
                rebuildSolids();
            }
        }

        if (doorHoleOpen_)
        {
            const float depth = GameConfig::height - Stage4Layout::floor + 40.f;
            doorFloorDrop_ = std::min(depth,
                doorFloorDrop_ + Stage4Layout::doorFloorFallSpeed * dt);
            rebuildSolids();
        }

        // 왼쪽 작은 블록: 플레이어를 오른쪽으로 밀기
        if (startBlockActive_)
        {
            startBlockX_ += Stage4Layout::startBlockSpeed * dt;

            const RectF block = startBlockBounds();

            if (block.intersects(player.bounds()))
                player.x = block.right();

            if (startBlockX_ >= Stage4Layout::leftHoleX)
                startBlockActive_ = false;
        }

        // 낭떠러지에 가까워지면 옆의 벽 상승
        if (risingBlockState_ == 0 &&
            player.x >= Stage4Layout::leftHoleX - GameConfig::playerWidth)
        {
            risingBlockState_ = 1;
        }

        // 벽 상승: 기존 속도 유지
        if (risingBlockState_ == 1)
        {
            risingBlockHeight_ = std::min(
                Stage4Layout::risingBlockHeight,
                risingBlockHeight_ + Stage4Layout::risingBlockSpeed * dt
            );

            if (risingBlockHeight_ >= Stage4Layout::risingBlockHeight)
                risingBlockState_ = 2;
        }
        // 벽 하강: 초당 60px로 천천히 내려옴
        else if (risingBlockState_ == 2)
        {
            risingBlockHeight_ = std::max(
                0.f,
                risingBlockHeight_ - 60.f * dt
            );

            if (risingBlockHeight_ <= 0.f)
                risingBlockState_ = 3;
        }

        // 벽 높이에 맞춰 충돌 영역 갱신
        if (risingBlockHeight_ > 0.f || risingBlockState_ == 3)
        {
            rebuildSolids();

            if (risingBlockHeight_ > 0.f)
            {
                const RectF wall = risingBlockBounds();

                if (wall.intersects(player.bounds()))
                    player.x = wall.x - GameConfig::playerWidth;
            }
        }

        // 오른쪽 큰 벽 작동
        if (!blockTriggered_ && player.x >= Stage4Layout::blockTriggerX)
        {
            blockTriggered_ = true;
            blockState_ = 1;
            movingBlockX_ = GameConfig::right;
        }

        // 왼쪽으로 오는 동안 닿으면 사망
        if (blockState_ == 1)
        {
            movingBlockX_ -= Stage4Layout::blockApproachSpeed * dt;

            if (movingBlockX_ <= Stage4Layout::blockRestX)
            {
                movingBlockX_ = Stage4Layout::blockRestX;
                blockState_ = 2;
            }
        }
        // 오른쪽으로 복귀
        else if (blockState_ == 2)
        {
            movingBlockX_ += Stage4Layout::blockReturnSpeed * dt;

            if (movingBlockX_ >= GameConfig::right)
            {
                movingBlockX_ = GameConfig::right;
                blockState_ = 0;
            }

            // 빠른 복귀 중에는 플레이어를 밀어 다음 함정 너머로 보내지 않음.
        }

        // 오른쪽 벽이 작동한 뒤 완전히 복귀했는지 확인
        const bool rightBlockReturned =
            blockTriggered_ && blockState_ == 0;

        // 오른쪽 벽 복귀 완료 + 플레이어 접근 시 낙하
        if (!fallingBlockTriggered_ &&
            !fallingBlockLanded_ &&
            rightBlockReturned &&
            player.x >= Stage4Layout::fallingBlockX - 30.f)
        {
            fallingBlockTriggered_ = true;
        }

        if (fallingBlockTriggered_)
        {
            const float bottomY =
                Stage4Layout::floor - Stage4Layout::fallingBlockHeight;

            // 착지하는 프레임까지 낙하 사망 판정 유지
            // 다음 프레임부터 대기 없이 상승
            if (!fallingBlockLanded_ && fallingBlockY_ >= bottomY)
                fallingBlockLanded_ = true;

            if (!fallingBlockLanded_)
            {
                // 접근 중인 플레이어와 충돌할 시간을 확보하며 내려옴
                fallingBlockY_ = std::min(
                    bottomY,
                    fallingBlockY_ + Stage4Layout::fallingBlockSpeed * dt
                );
            }
            else
            {
                // 바닥에 닿으면 빠르게 다시 올라감
                fallingBlockY_ = std::max(
                    Stage4Layout::fallingBlockStartY,
                    fallingBlockY_ - Stage4Layout::fallingBlockReturnSpeed * dt
                );

                if (fallingBlockY_ <= Stage4Layout::fallingBlockStartY)
                    fallingBlockTriggered_ = false;
            }
        }
    }

    bool Stage4::touchesHazard(const RectF& playerBounds) const
    {
        // 오른쪽에서 왼쪽으로 오는 큰 벽
        const bool hitByRightBlock =
            blockState_ == 1 &&
            playerBounds.intersects(movingBlockBounds());

        // 낙하 중인 블록에 닿으면 사망
        // 다시 올라가는 동안에는 사망 판정 없음
        const bool hitByFallingBlock =
            fallingBlockTriggered_ &&
            !fallingBlockLanded_ &&
            playerBounds.intersects(fallingBlockBounds());

        return hitByRightBlock || hitByFallingBlock;
    }

    bool Stage4::isAtGoal(const PlayerState& player) const
    {
        const RectF door{
            Stage4Layout::doorX,
            Stage4Layout::floor - Stage4Layout::doorHeight,
            Stage4Layout::doorWidth,
            Stage4Layout::doorHeight
        };

        return player.bounds().intersects(door);
    }

    Vec2 Stage4::goalPoint() const
    {
        return {
            Stage4Layout::doorX +
                (Stage4Layout::doorWidth - GameConfig::playerWidth) * .5f,
            Stage4Layout::floor - GameConfig::playerHeight
        };
    }

    void Stage4::draw(sf::RenderTarget& target) const
    {
        using namespace render;

        // 배경
        box(
            target,
            GameConfig::left,
            GameConfig::ceiling,
            GameConfig::right - GameConfig::left,
            Stage4Layout::floor - GameConfig::ceiling,
            air
        );

        // 낭떠러지
        box(
            target,
            Stage4Layout::leftHoleX,
            Stage4Layout::floor,
            Stage4Layout::leftHoleWidth,
            GameConfig::height - Stage4Layout::floor + 40.f,
            air
        );

        // 바닥의 윗면과 충돌 영역이 같은 속도로 아래로 내려감.
        if (doorHoleOpen_)
        {
            box(target, Stage4Layout::doorHoleX, Stage4Layout::floor,
                Stage4Layout::doorHoleWidth,
                doorFloorDrop_, air);
        }
        else if (doorHoleTriggered_)
        {
            // 무너지기 직전 짧게 보이는 금.
            box(target, Stage4Layout::doorHoleX + 4.f, Stage4Layout::floor,
                Stage4Layout::doorHoleWidth - 8.f, 2.f, ink);
        }

        // 문
        drawDoor(
            target,
            Stage4Layout::doorX,
            Stage4Layout::floor,
            Stage4Layout::doorWidth,
            Stage4Layout::doorHeight
        );

        // 내려왔다 다시 올라가는 블록
        if (fallingBlockTriggered_)
        {
            const RectF block = fallingBlockBounds();

            box(target, block.x, block.y,
                block.width, block.height, earth);
        }

        // 시작 시 왼쪽에서 오는 작은 블록
        if (startBlockActive_)
        {
            const RectF block = startBlockBounds();

            box(target, block.x, block.y,
                block.width, block.height, earth);
        }

        // 낭떠러지 옆에서 올라왔다 천천히 내려가는 벽
        if (risingBlockHeight_ > 0.f)
        {
            const RectF block = risingBlockBounds();

            box(target, block.x, block.y,
                block.width, block.height, earth);
        }

        // 오른쪽에서 오는 큰 벽
        if (blockState_ != 0)
        {
            const RectF block = movingBlockBounds();

            box(target, block.x, block.y,
                block.width, block.height, earth);
        }
    }
}
