#include "Stage5.hpp"

#include "RenderCommon.hpp"
#include <algorithm>

namespace devil
{
    // Stage5 객체가 만들어질 때 실행되는 생성자
    Stage5::Stage5()
    {
        // 스테이지를 처음 상태로 초기화
        reset();
    }

    // 플레이어가 처음 시작할 위치를 반환
    Vec2 Stage5::spawnPoint() const
    {
        return {
            Stage5Layout::spawnX,   // 플레이어의 시작 X 위치

            // 바닥 바로 위에 플레이어가 서도록 Y 위치 계산
            Stage5Layout::floor - GameConfig::playerHeight
        };
    }

    // 현재 Stage5에서 플레이어가 밟을 수 있는 바닥을 만듦
    void Stage5::rebuildSolids()
    {
        const float depth =
            GameConfig::height - Stage5Layout::floor + 40.f;// 바닥 충돌 영역의 깊이

        solids_.clear();// 기존 충돌 영역 전부 삭제


        // =====================================================
        // 1페이즈
        // trap1 → trap2 → trap3
        // 플레이어가 왼쪽에서 오른쪽으로 진행
        // =====================================================
        if (!secondPhase_)
        {
            float currentX = GameConfig::left;// 현재 바닥 생성 시작 위치


            // -------------------------
            // trap1
            // -------------------------
            if (trap1Open_)
            {
                // 왼쪽 끝 ~ trap1 앞까지 바닥 생성
                solids_.push_back({
                    currentX,
                    Stage5Layout::floor,
                    Stage5Layout::trap1X - currentX,
                    depth
                    });

                // trap1 구간은 비우고 그 다음 위치부터 시작
                currentX =
                    Stage5Layout::trap1X +
                    Stage5Layout::trap1Width;
            }


            // -------------------------
            // trap2
            // -------------------------
            if (trap2Open_)
            {
                // 현재 위치 ~ trap2 앞까지 바닥 생성
                solids_.push_back({
                    currentX,
                    Stage5Layout::floor,
                    Stage5Layout::trap2X - currentX,
                    depth
                    });

                // trap2 구간은 비움
                currentX =
                    Stage5Layout::trap2X +
                    Stage5Layout::trap2Width;
            }


            // -------------------------
            // trap3
            // -------------------------
            if (trap3Open_)
            {
                // 현재 위치 ~ trap3 앞까지 바닥 생성
                solids_.push_back({
                    currentX,
                    Stage5Layout::floor,
                    Stage5Layout::trap3X - currentX,
                    depth
                    });

                // trap3 구간은 비움
                currentX =
                    Stage5Layout::trap3X +
                    Stage5Layout::trap3Width;
            }


            // -------------------------
            // 마지막 남은 바닥
            // -------------------------
            solids_.push_back({
                currentX,
                Stage5Layout::floor,
                GameConfig::right - currentX,
                depth
                });
        }


        else
        {
            // =========================================
            // 2페이즈
            // trap4, trap5 상태를 그대로 반영
            // =========================================


            // -------------------------
            // 왼쪽 끝 ~ trap6 앞
            // -------------------------
            solids_.push_back({
                GameConfig::left,
                Stage5Layout::floor,
                Stage5Layout::trap6X - GameConfig::left,
                depth
                });


            // -------------------------
            // trap6
            // -------------------------
            if (!trap6Open_)
            {
                solids_.push_back({
                    Stage5Layout::trap6X,
                    Stage5Layout::floor,
                    Stage5Layout::trap6Width,
                    depth
                    });
            }


            // -------------------------
            // trap6 끝 ~ trap5 앞
            // -------------------------
            // trap6 끝 ~ trap7 앞
            solids_.push_back({
                Stage5Layout::trap6X + Stage5Layout::trap6Width,// trap6 끝 위치
                Stage5Layout::floor,
                Stage5Layout::trap7X -
                    (Stage5Layout::trap6X + Stage5Layout::trap6Width),
                depth
                });

            // 3페이즈에서 trap7이 닫혀 있을 때만 바닥 생성
            if (!thirdPhase_ || !trap7Open_)
            {
                solids_.push_back({
                    Stage5Layout::trap7X,// trap7 시작 X
                    Stage5Layout::floor,
                    Stage5Layout::trap7Width,// trap7 너비
                    depth
                    });
            }

            // trap7 끝 ~ trap5 앞
            solids_.push_back({
                Stage5Layout::trap7X + Stage5Layout::trap7Width,
                Stage5Layout::floor,
                Stage5Layout::trap5X -
                    (Stage5Layout::trap7X + Stage5Layout::trap7Width),
                depth
                });


            // trap5가 닫혀 있을 때만 바닥 생성
            if (!trap5Open_)
            {
                solids_.push_back({
                    Stage5Layout::trap5X,
                    Stage5Layout::floor,
                    Stage5Layout::trap5Width,
                    depth
                    });
            }

            // 계단 오른쪽 끝 ~ trap4 시작
            const float stairRight =
                Stage5Layout::stairX +
                Stage5Layout::stairStepWidth * 3.f;

            if (stairRight < Stage5Layout::trap4X)
            {
                solids_.push_back({
                    stairRight,
                    Stage5Layout::floor,
                    Stage5Layout::trap4X - stairRight,
                    depth
                    });
            }


            // trap4가 닫혀 있을 때만 바닥 생성
            if (!trap4Open_)
            {
                solids_.push_back({
                    Stage5Layout::trap4X,
                    Stage5Layout::floor,
                    Stage5Layout::trap4Width,
                    depth
                    });
            }


            // trap4 오른쪽은 현재 화면 끝과 같아서
            // 추가 바닥 없음

            // trap4가 열렸을 때
            // 떨어지는 도중 플레이어가 왼쪽 벽을 뚫고 나가지 못하도록 세로 충돌벽 생성
            if (trap4Open_)
            {
                solids_.push_back({
                    Stage5Layout::trap4X - 4.f,// 구멍 왼쪽 경계 바로 앞
                    Stage5Layout::floor,// 바닥 아래부터 시작
                    4.f,// 얇은 세로 벽
                    depth// 화면 아래까지 이어짐
                    });
            }
        }


        // =====================================================
        // 솟아오르는 바닥
        // =====================================================
        if (riseActive_)
        {
            float amount = riseAmount();// 현재 솟아오른 높이

            solids_.push_back({
                Stage5Layout::riseX,
                Stage5Layout::floor - amount,
                Stage5Layout::riseWidth,
                amount
                });
        }


        // =====================================================
        // 계단
        // =====================================================
        if (stairActive_)
        {
            float rise =
                stairRiseAmount();// 현재 계단이 올라온 높이

            float totalHeight =
                Stage5Layout::stairStepHeight * 3.f;// 계단 전체 높이

            float offset =
                totalHeight - rise;// 아직 올라오지 않은 만큼 아래로 이동


            // -------------------------
            // 계단 1단
            // -------------------------
            solids_.push_back({
                Stage5Layout::stairX,

                Stage5Layout::floor -
                    Stage5Layout::stairStepHeight +
                    offset,

                Stage5Layout::stairStepWidth * 3.f,

                Stage5Layout::stairStepHeight
                });


            // -------------------------
            // 계단 2단
            // -------------------------
            solids_.push_back({
                Stage5Layout::stairX +
                    Stage5Layout::stairStepWidth * 0.5f,

                Stage5Layout::floor -
                    Stage5Layout::stairStepHeight * 2.f +
                    offset,

                Stage5Layout::stairStepWidth * 2.f,

                Stage5Layout::stairStepHeight
                });


            // -------------------------
            // 계단 3단
            // -------------------------
            solids_.push_back({
                Stage5Layout::stairX +
                    Stage5Layout::stairStepWidth,

                Stage5Layout::floor -
                    Stage5Layout::stairStepHeight * 3.f +
                    offset,

                Stage5Layout::stairStepWidth,

                Stage5Layout::stairStepHeight
                });

            // 2페이즈에서 계단 아래쪽으로 들어가는 것을 막는 보이지 않는 벽
            if (secondPhase_)
            {
                solids_.push_back({
                    Stage5Layout::stairX,// X=681
                    Stage5Layout::floor,// 바닥 Y=400부터 시작
                    4.f,// 보이지 않는 벽 너비
                    GameConfig::height - Stage5Layout::floor// 화면 아래까지 막음
                    });
            }
        }

        // =====================================================
        // 내려오는 기둥 충돌
        // =====================================================
        if (dropColumnActive_)
        {
            float amount = dropColumnAmount();// 현재 기둥이 내려온 높이

            solids_.push_back({
                Stage5Layout::dropColumnX,// 기둥 X 위치
                GameConfig::ceiling,// 천장에서 시작
                Stage5Layout::dropColumnWidth,// 기둥 너비
                amount// 현재 내려온 높이
                });
        }
    }

    // Stage5를 처음 상태로 되돌리는 함수
    void Stage5::reset()
    {
        trap1Open_ = false;
        trap2Open_ = false;
        trap3Open_ = false;
        riseTriggered_ = false;
        riseActive_ = false;
        riseGoingDown_ = false;
        stairActive_ = false;
        doorMoved_ = false;// 문 이동 상태 초기화
        secondPhase_ = false;// 죽으면 첫 번째 구간부터 다시 시작
        secondPhaseResetDone_ = false;// 죽으면 세컨드 페이즈 초기화 상태도 리셋
        floorRestoring_ = false;// 바닥 복구 상태 초기화

        doorX_ = Stage5Layout::doorX;// 죽으면 문을 처음 위치로 되돌림
        doorY_ = Stage5Layout::floor;// 문의 Y 위치도 초기화

        // 현재는 기믹이 없으므로
        // 기본 바닥만 다시 만들어주면 됨
        trap1Time_ = 0.f;
        trap2Time_ = 0.f;
        trap3Time_ = 0.f;
        riseTime_ = 0.f;
        stairTime_ = 0.f;
        floorRestoreTime_ = 0.f;// 바닥 복구 시간 초기화

        trap4Open_ = false;// 네 번째 함정 초기화
        trap5Open_ = false;// 다섯 번째 함정 초기화

        trap4Time_ = 0.f;// 네 번째 함정 시간 초기화
        trap5Time_ = 0.f;// 다섯 번째 함정 시간 초기화

        dropColumnActive_ = false;// 기둥 함정 상태 초기화
        dropColumnTime_ = 0.f;// 기둥 애니메이션 시간 초기화

        trap6Open_ = false;// 여섯 번째 함정 초기화
        trap6Time_ = 0.f;// 여섯 번째 함정 시간 초기화

        thirdPhase_ = false;// 죽으면 3페이즈 상태 초기화

        trap7Open_ = false;// 3페이즈 바닥 함정 초기화
        trap7Time_ = 0.f;// 3페이즈 바닥 함정 시간 초기화

        restartRequested_ = false;// 죽거나 스테이지가 초기화되면 재시작 요청도 초기화

        fakeMessageActive_ = false;// 가짜 문 문구 상태 초기화
        fakeMessageTime_ = 0.f;// 문구 표시 시간 초기화
        restartRequested_ = false;// 게임 처음부터 재시작 요청 초기화

        rebuildSolids();
    }

    // 매 프레임마다 Stage5의 상태를 갱신하는 함수
    void Stage5::update(PlayerState& player, float dt)
    {
        if (!secondPhase_ && !trap1Open_ &&
            player.x + GameConfig::playerWidth >= Stage5Layout::trap1TriggerX)
        {
            // 첫 번째 함정 발동
            trap1Open_ = true;

            trap1Time_ = 0.f; // 함정 애니메이션 시간을 0부터 시작

            // 바닥 충돌 영역을 현재 상태에 맞게 다시 만듦
            rebuildSolids();
        }

        // 두 번째 함정 발동
        if (!secondPhase_ && !trap2Open_ &&
            player.x + GameConfig::playerWidth >= Stage5Layout::trap2TriggerX)
        {
            trap2Open_ = true;
            trap2Time_ = 0.f;   // 두 번째 함정 애니메이션 시작 시간
            rebuildSolids();
        }

        // 세 번째 함정 발동
        if (!secondPhase_ && !trap3Open_ &&
            player.x + GameConfig::playerWidth >= Stage5Layout::trap3TriggerX)
        {
            trap3Open_ = true;
            trap3Time_ = 0.f;   // 애니메이션 시작
            rebuildSolids();    // 실제 충돌 바닥 다시 생성
        }



        // 솟아오르는 바닥 함정 발동
        if (!secondPhase_ && !riseTriggered_ &&
            player.x + GameConfig::playerWidth >= Stage5Layout::riseTriggerX)
        {
            riseTriggered_ = true;// 같은 위치에 머물러도 한 번만 발동
            riseActive_ = true;// 함정 발동
            riseTime_ = 0.f;// 솟아오르는 애니메이션 시작
        }

        // 플레이어가 계단 근처에 도착하면 계단 함정 발동
        if (!stairActive_ &&
            player.x + GameConfig::playerWidth >= Stage5Layout::stairX - 40.f)
        {
            stairActive_ = true;// 계단 올라오기 시작
            stairTime_ = 0.f;// 애니메이션 시간 초기화
        }

        // 계단이 발동한 뒤 시간 증가
        if (stairActive_)
        {
            stairTime_ += dt;// 계단 애니메이션 시간 증가
            rebuildSolids();// 올라오는 위치에 맞게 충돌 영역 갱신
        }

        // 함정이 열린 뒤에는 매 프레임 시간 누적
        if (trap1Open_)
        {
            trap1Time_ += dt;
        }

        // 함정이 열린 뒤에는 매 프레임 시간 누적
        if (trap2Open_)
        {
            trap2Time_ += dt;
        }

        if (trap3Open_)
        {
            trap3Time_ += dt;
        }

        if (trap4Open_)
        {
            trap4Time_ += dt;// 네 번째 함정 애니메이션 시간 증가
        }

        if (trap5Open_)
        {
            trap5Time_ += dt;// 다섯 번째 함정 애니메이션 시간 증가
        }

        if (riseActive_)
        {
            riseTime_ += dt;// 함정 발동 후 시간 증가

            // 아직 내려가는 중이 아니고,
            // 올라가는 시간 + 2초 유지 시간이 지났으면 내려가기 시작
            if (!riseGoingDown_ &&
                riseTime_ >= Stage5Layout::riseDuration + Stage5Layout::riseHoldDuration)
            {
                riseGoingDown_ = true;// 내려가기 시작
                riseTime_ = 0.f;// 내려가는 시간 측정을 위해 다시 0으로 초기화
            }

            if (riseGoingDown_ && riseTime_ >= Stage5Layout::riseDuration)
            {
                riseActive_ = false;
                riseGoingDown_ = false;
                riseTime_ = 0.f;
            }

            rebuildSolids();// 현재 높이에 맞게 충돌 바닥 계속 갱신
        }
        // 플레이어가 가짜 문에 가까워지면
        // trap4 발동 + 문 이동 + 2페이즈 시작
        if (!doorMoved_ &&
            player.x + GameConfig::playerWidth
            >= Stage5Layout::doorX - 10.f)
        {
            trap4Open_ = true;// 4번 함정 먼저 발동
            trap4Time_ = 0.f;

            doorX_ = Stage5Layout::doorMoveX;// 문 이동
            doorY_ = Stage5Layout::doorMoveY;

            doorMoved_ = true;
            secondPhase_ = true;// 그다음 2페이즈 시작

            rebuildSolids();// trap4Open_ 상태를 반영해서 2페이즈 바닥 생성
        }
        if (secondPhase_ && !secondPhaseResetDone_)
        {
            floorRestoring_ = true;// 기존 함정 바닥 복구 시작
            floorRestoreTime_ = 0.f;// 복구 애니메이션 시간 초기화

            riseActive_ = false;// 솟아오르는 바닥 비활성화
            riseGoingDown_ = false;// 내려가는 상태 초기화
            riseTime_ = 0.f;// 시간 초기화

            secondPhaseResetDone_ = true;// 세컨드 페이즈 초기화는 한 번만 실행
        }

        if (floorRestoring_)
        {
            floorRestoreTime_ += dt;// 복구 애니메이션 시간 증가

            // 복구 시간이 끝났으면 완전히 원래 바닥으로 변경
            if (floorRestoreTime_ >= Stage5Layout::floorRestoreDuration)
            {
                floorRestoring_ = false;// 복구 애니메이션 종료

                trap1Open_ = false;// 첫 번째 함정 닫기
                trap2Open_ = false;// 두 번째 함정 닫기
                trap3Open_ = false;// 세 번째 함정 닫기


                trap1Time_ = 0.f;
                trap2Time_ = 0.f;
                trap3Time_ = 0.f;

                rebuildSolids();// 전체 바닥으로 충돌 영역 복구
            }
        }

        // 세컨드 페이즈에서 다섯 번째 함정 발동
        if (secondPhase_ &&
            !trap5Open_ &&
            player.x <= Stage5Layout::trap5TriggerX)
        {
            trap5Open_ = true;// 다섯 번째 함정 열기
            trap5Time_ = 0.f;// 애니메이션 시간 초기화

            rebuildSolids();// 충돌 바닥 다시 생성
        }

        // 가짜 문이 이동한 뒤에는 진짜 문이 플레이어를 피해 왼쪽으로 도망감
        if (secondPhase_ && doorMoved_)
        {
            float targetDoorX =
                player.x - Stage5Layout::doorEscapeDistance;// 플레이어보다 일정거리 왼쪽 위치 계산

            // 플레이어가 가까워져서 현재 문보다 더 왼쪽으로 가야 할 때만 이동
            if (targetDoorX < doorX_)
            {
                doorX_ = std::max(
                    Stage5Layout::doorStopX,
                    targetDoorX
                );// X=140보다 더 왼쪽으로는 못 감
            }
        }

        // 도망가던 문이 X=140에 도착하면 3페이즈 시작
        if (secondPhase_ &&
            !thirdPhase_ &&
            doorX_ <= Stage5Layout::doorStopX)
        {
            doorX_ = Stage5Layout::doorStopX;// 문 위치를 정확히 X=140으로 고정
            thirdPhase_ = true;// 3페이즈 시작
        }

        // 3페이즈에서 X=140 가짜 문에 들어가면 가짜 문 연출 시작
        if (thirdPhase_ &&
            !fakeMessageActive_ &&
            !restartRequested_)
        {
            const RectF fakeDoorBounds{
                doorX_,// 가짜 문의 X 위치
                doorY_ - Stage5Layout::doorHeight,// 가짜 문의 위쪽 Y 위치
                Stage5Layout::doorWidth,// 문 너비
                Stage5Layout::doorHeight// 문 높이
            };

            if (player.bounds().intersects(fakeDoorBounds))
            {
                fakeMessageActive_ = true;// "It's Fake!" 문구 표시 시작
                fakeMessageTime_ = 0.f;// 문구 표시 시간 초기화
            }
        }

        // 가짜 문 문구를 1초 동안 표시
        if (fakeMessageActive_)
        {
            fakeMessageTime_ += dt;// 문구 표시 시간 증가

            if (fakeMessageTime_ >= 1.f)
            {
                restartRequested_ = true;// 1초 뒤 StageManager에게 처음부터 재시작 요청
            }
        }

        // 내려오는 기둥 함정 발동
        if (secondPhase_ &&
            !dropColumnActive_ &&
            player.x <= Stage5Layout::dropColumnTriggerX)
        {
            dropColumnActive_ = true;// 기둥 함정 발동
            dropColumnTime_ = 0.f;// 애니메이션 시간 초기화
        }

        // 기둥이 발동한 뒤 애니메이션 시간 증가
        if (dropColumnActive_)
        {
            dropColumnTime_ += dt;// 기둥 내려오는 애니메이션 시간 증가
            rebuildSolids();// 내려온 높이에 맞춰 기둥 충돌 영역도 계속 갱신
        }

        // 세컨드 페이즈에서 여섯 번째 함정 발동
        if (secondPhase_ &&
            !trap6Open_ &&
            player.x <= Stage5Layout::trap6TriggerX)
        {
            trap6Open_ = true;// 여섯 번째 함정 열기
            trap6Time_ = 0.f;// 애니메이션 시간 초기화

            rebuildSolids();// trap6 구간의 충돌 바닥 제거
        }

        if (trap6Open_)
        {
            trap6Time_ += dt;// 여섯 번째 함정 애니메이션 시간 증가
        }

        // 3페이즈에서 일곱 번째 바닥 함정 발동
        // 3페이즈에서 오른쪽으로 이동하다 7번 함정 가까이에 오면 발동
        if (thirdPhase_ &&
            !trap7Open_ &&
            player.x + GameConfig::playerWidth >= Stage5Layout::trap7TriggerX)
        {
            trap7Open_ = true;// 일곱 번째 바닥 함정 열기
            trap7Time_ = 0.f;// 애니메이션 시간 초기화

            rebuildSolids();// trap7 구간의 충돌 바닥 제거
        }

        if (trap7Open_)
        {
            trap7Time_ += dt;// 일곱 번째 함정 애니메이션 시간 증가
        }

    }

    // 플레이어가 위험한 함정에 닿았는지 확인하는 함수
    bool Stage5::touchesHazard(const RectF&) const
    {
        return false;// Stage5에는 닿는 즉시 죽는 위험 영역이 없음
    }

    // 플레이어가 문에 닿았는지 확인하는 함수
    bool Stage5::isAtGoal(const PlayerState& player) const
    {
        // 아직 3페이즈가 시작되지 않았다면 클리어 불가능
        if (!thirdPhase_)
        {
            return false;
        }

        // X=500에 새로 생긴 진짜 문 위치
        const RectF realDoorBounds{
            Stage5Layout::realDoorX,// 진짜 문의 X 위치
            Stage5Layout::floor - Stage5Layout::doorHeight,// 문의 위쪽 Y
            Stage5Layout::doorWidth,// 문 너비
            Stage5Layout::doorHeight// 문 높이
        };

        return player.bounds().intersects(realDoorBounds);// 진짜 문에 닿았을 때만 클리어
    }

    // 플레이어가 문에 들어갈 때 이동할 최종 위치를 반환하는 함수
    Vec2 Stage5::goalPoint() const
    {
        return {
            Stage5Layout::realDoorX +
                (Stage5Layout::doorWidth - GameConfig::playerWidth) * 0.5f,// 진짜 문 중앙 X

            Stage5Layout::floor -
                GameConfig::playerHeight// 바닥에 맞춘 플레이어 Y
        };
    }

    // Stage5의 배경과 문을 화면에 그리는 함수
    void Stage5::draw(sf::RenderTarget& target) const
    {
        // RenderCommon.hpp 안의 그리기 함수들을 쉽게 사용하기 위해 선언
        using namespace render;

        // 플레이 가능한 공간의 배경을 그림
        box(
            target,
            GameConfig::left,                                  // 시작 X
            GameConfig::ceiling,                               // 시작 Y
            GameConfig::right - GameConfig::left,              // 가로 길이
            Stage5Layout::floor - GameConfig::ceiling,         // 세로 길이
            air                                                 // 배경 색상
        );

        if (trap1Open_)
        {
            // trap1Depth() 값만큼만 아래로 열리게 그림
            box(
                target,
                Stage5Layout::trap1X,      // 구멍 시작 X
                Stage5Layout::floor,       // 바닥 Y
                Stage5Layout::trap1Width,  // 구멍 너비
                trap1Depth(),              // 시간이 지날수록 점점 커지는 깊이
                air
            );
        }

        // 두 번째 함정이 열렸으면
        if (trap2Open_)
        {
            // 바닥 색으로 덮어서
            // trap2Depth()만큼 아래로 내려가는 것처럼 보이게 함
            box(
                target,
                Stage5Layout::trap2X,
                Stage5Layout::floor,
                Stage5Layout::trap2Width,
                trap2Depth(),
                air
            );
        }

        // 세 번째 함정 시각 효과
        if (trap3Open_)
        {
            box(
                target,
                Stage5Layout::trap3X,
                Stage5Layout::floor,
                Stage5Layout::trap3Width,
                trap3Depth(),
                air
            );
        }

        if (trap4Open_)
        {
            box(
                target,
                Stage5Layout::trap4X,// 네 번째 구멍 시작 X
                Stage5Layout::floor,// 바닥 Y
                Stage5Layout::trap4Width,// 네 번째 구멍 너비
                trap4Depth(),// 시간이 지날수록 커지는 깊이
                air
            );
        }

        if (trap5Open_)
        {
            box(
                target,
                Stage5Layout::trap5X,// 다섯 번째 구멍 시작 X
                Stage5Layout::floor,// 바닥 Y
                Stage5Layout::trap5Width,// 다섯 번째 구멍 너비
                trap5Depth(),// 시간이 지날수록 커지는 깊이
                air
            );
        }

        if (trap6Open_)
        {
            box(
                target,
                Stage5Layout::trap6X,// 여섯 번째 구멍 시작 X
                Stage5Layout::floor,// 바닥 Y
                Stage5Layout::trap6Width,// 여섯 번째 구멍 너비
                trap6Depth(),// 시간이 지날수록 아래로 열리는 깊이
                air
            );
        }

        if (riseActive_)
        {
            float amount = riseAmount();// 현재 솟아오른 높이 계산

            box(
                target,
                Stage5Layout::riseX,// 솟아오르는 땅 시작 X
                Stage5Layout::floor - amount,// 위로 올라간 만큼 Y 위치 이동
                Stage5Layout::riseWidth,// 솟아오르는 땅 너비
                amount,// 현재 솟아오른 높이
                earth
            );
        }

        if (dropColumnActive_)
        {
            float amount = dropColumnAmount();// 현재 기둥이 내려온 높이

            box(
                target,
                Stage5Layout::dropColumnX,// 기둥 X 위치
                GameConfig::ceiling,// 천장에서 시작
                Stage5Layout::dropColumnWidth,// 기둥 너비
                amount,// 현재 내려온 높이
                earth
            );
        }

        if (stairActive_)
        {
            float rise = stairRiseAmount();// 현재 올라온 높이

            float totalHeight = Stage5Layout::stairStepHeight * 3.f;// 계단 전체 높이
            float offset = totalHeight - rise;// 아직 올라오지 않은 만큼 아래로 이동

            // 가장 아래 1단
            box(
                target,
                Stage5Layout::stairX,
                Stage5Layout::floor - Stage5Layout::stairStepHeight + offset,
                Stage5Layout::stairStepWidth * 3.f,
                Stage5Layout::stairStepHeight,
                earth
            );

            // 가운데 2단
            box(
                target,
                Stage5Layout::stairX + Stage5Layout::stairStepWidth * 0.5f,
                Stage5Layout::floor - Stage5Layout::stairStepHeight * 2.f + offset,
                Stage5Layout::stairStepWidth * 2.f,
                Stage5Layout::stairStepHeight,
                earth
            );

            // 가장 위 3단
            box(
                target,
                Stage5Layout::stairX + Stage5Layout::stairStepWidth,
                Stage5Layout::floor - Stage5Layout::stairStepHeight * 3.f + offset,
                Stage5Layout::stairStepWidth,
                Stage5Layout::stairStepHeight,
                earth
            );
        }
        // 클리어 문을 그림
        drawDoor(
            target,
            doorX_,// 현재 문의 X 위치
            doorY_,// 현재 문의 바닥 Y 위치
            Stage5Layout::doorWidth,// 문 너비
            Stage5Layout::doorHeight// 문 높이
        );

        // 3페이즈가 시작되면 진짜 문을 X=500에 추가로 그림
        if (thirdPhase_)
        {
            drawDoor(
                target,
                Stage5Layout::realDoorX,// 진짜 문 X 위치
                Stage5Layout::floor,// 진짜 문 바닥 Y 위치
                Stage5Layout::doorWidth,// 문 너비
                Stage5Layout::doorHeight// 문 높이
            );
        }

        if (trap7Open_)
        {
            box(
                target,
                Stage5Layout::trap7X,// 일곱 번째 구멍 시작 X
                Stage5Layout::floor,// 바닥 Y
                Stage5Layout::trap7Width,// 일곱 번째 구멍 너비
                trap7Depth(),// 시간이 지날수록 아래로 열리는 깊이
                air
            );
        }

        // 가짜 문에 들어갔을 때 클리어 화면과 같은 레이아웃으로 문구 표시
        if (fakeMessageActive_)
        {
            box(
                target,
                240.f,// 바깥 박스 X
                171.f,// 바깥 박스 Y
                480.f,// 바깥 박스 너비
                233.f,// 바깥 박스 높이
                sf::Color(107, 78, 13)// 바깥 테두리 색
            );

            box(
                target,
                246.f,// 안쪽 박스 X
                177.f,// 안쪽 박스 Y
                468.f,// 안쪽 박스 너비
                221.f,// 안쪽 박스 높이
                air// 안쪽 배경 색
            );

            render::label(
                target,
                "ITS FAKE",
                480.f,// 화면 중앙 X
                260.f,// 박스 안쪽 중앙쯤 Y
                6.f,// 클리어 문구와 같은 크기
                ink,// 글자 색
                true// 가운데 정렬
            );
        }
    }
    // 첫 번째 바닥 함정이 현재 얼마나 열렸는지 계산
    float Stage5::trap1Depth() const
    {
        if (!trap1Open_)
            return 0.f;

        if (floorRestoring_)
        {
            const float progress = std::clamp(
                floorRestoreTime_ / Stage5Layout::floorRestoreDuration,
                0.f,
                1.f
            );

            const float eased = progress * progress;

            return (GameConfig::height - Stage5Layout::floor)
                * (1.f - eased);// 깊이가 최대값에서 0으로 줄어듦
        }

        const float progress = std::clamp(
            trap1Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );

        const float eased = progress * progress;

        return (GameConfig::height - Stage5Layout::floor) * eased;
    }

    float Stage5::trap2Depth() const
    {
        if (!trap2Open_)
            return 0.f;

        if (floorRestoring_)
        {
            const float progress = std::clamp(
                floorRestoreTime_ / Stage5Layout::floorRestoreDuration,
                0.f,
                1.f
            );

            const float eased = progress * progress;

            return (GameConfig::height - Stage5Layout::floor)
                * (1.f - eased);// 복구되면서 깊이가 줄어듦
        }

        const float progress = std::clamp(
            trap2Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );

        const float eased = progress * progress;

        return (GameConfig::height - Stage5Layout::floor) * eased;
    }

    float Stage5::trap3Depth() const
    {
        if (!trap3Open_)
            return 0.f;

        if (floorRestoring_)
        {
            const float progress = std::clamp(
                floorRestoreTime_ / Stage5Layout::floorRestoreDuration,
                0.f,
                1.f
            );

            const float eased = progress * progress;

            return (GameConfig::height - Stage5Layout::floor)
                * (1.f - eased);// 복구되면서 깊이가 줄어듦
        }

        const float progress = std::clamp(
            trap3Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );

        const float eased = progress * progress;

        return (GameConfig::height - Stage5Layout::floor) * eased;
    }

    float Stage5::trap4Depth() const
    {
        if (!trap4Open_)
            return 0.f;// 아직 함정이 발동하지 않았으면 깊이 0

        const float progress = std::clamp(
            trap4Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );// 애니메이션 진행도 0~1

        const float eased = progress * progress;// 점점 빠르게 내려가는 효과

        return (GameConfig::height - Stage5Layout::floor) * eased;
    }

    float Stage5::trap5Depth() const
    {
        if (!trap5Open_)
            return 0.f;// 아직 함정이 발동하지 않았으면 깊이 0

        const float progress = std::clamp(
            trap5Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );// 애니메이션 진행도 0~1

        const float eased = progress * progress;// 점점 빠르게 내려가는 효과

        return (GameConfig::height - Stage5Layout::floor) * eased;
    }

    float Stage5::riseAmount() const
    {
        if (!riseActive_)
            return 0.f;// 아직 함정이 발동하지 않았으면 높이 0

        // 바닥이 다시 내려가는 중일 때
        if (riseGoingDown_)
        {
            const float progress = std::clamp(
                riseTime_ / Stage5Layout::riseDuration,
                0.f,
                1.f
            );// 내려가는 진행도 0~1

            return Stage5Layout::riseHeight * (1.f - progress);// 70 → 0으로 감소
        }

        // 처음 바닥이 올라가는 중일 때
        const float progress = std::clamp(
            riseTime_ / Stage5Layout::riseDuration,
            0.f,
            1.f
        );// 올라가는 진행도 0~1

        return Stage5Layout::riseHeight * progress;// 0 → 70으로 증가
    }

    float Stage5::stairRiseAmount() const
    {
        if (!stairActive_)
            return 0.f;// 아직 계단이 발동하지 않았으면 0

        const float progress = std::clamp(
            stairTime_ / Stage5Layout::stairRiseDuration,
            0.f,
            1.f
        );// 계단이 올라오는 진행도 0~1

        return Stage5Layout::stairStepHeight * 3.f * progress;// 0 →105까지 올라옴
    }

    float Stage5::dropColumnAmount() const
    {
        if (!dropColumnActive_)
            return 0.f;// 아직 발동 안 했으면 내려온 높이 0

        const float progress = std::clamp(
            dropColumnTime_ / Stage5Layout::dropColumnDuration,
            0.f,
            1.f
        );// 애니메이션 진행도 0~1

        return Stage5Layout::dropColumnHeight * progress;// 0 → 118까지 내려옴
    }

    float Stage5::trap6Depth() const
    {
        if (!trap6Open_)
            return 0.f;// 아직 발동하지 않았으면 깊이 0

        const float progress = std::clamp(
            trap6Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );// 애니메이션 진행도 0~1

        const float eased = progress * progress;// 점점 빠르게 내려가는 효과

        return (GameConfig::height - Stage5Layout::floor) * eased;// 아래로 열리는 깊이
    }

    float Stage5::trap7Depth() const
    {
        if (!trap7Open_)
            return 0.f;// 아직 발동하지 않았으면 깊이 0

        const float progress = std::clamp(
            trap7Time_ / Stage5Layout::trap1OpenDuration,
            0.f,
            1.f
        );// 애니메이션 진행도 0~1

        const float eased = progress * progress;// 점점 빠르게 내려가는 효과

        return (GameConfig::height - Stage5Layout::floor) * eased;// 아래로 열리는 깊이
    }

    bool Stage5::restartRequested() const
    {
        return restartRequested_;// 현재 재시작 요청 상태 반환
    }
}
