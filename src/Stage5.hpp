#pragma once

#include "IStage.hpp"

namespace devil
{
    struct Stage5Layout
    {
        static constexpr float floor = 400.f; //바닥 y좌표
        static constexpr float spawnX = 100.f; //플레이어 시작 x좌표
        static constexpr float doorX = 820.f; //현재 테스트용 문 x좌표
        static constexpr float doorWidth = 40.f; //문너비
        static constexpr float doorHeight = 36.f; //문높이

        static constexpr float doorMoveX = 637.f; //이동 후 문 x
        static constexpr float doorMoveY = 400.f; //이동 후 문 바닥 y

        static constexpr float trap1X = 120.f; // 첫 번째로 사라질 바닥의 시작 X 위치
        static constexpr float trap1Width = 50.f;  // 사라질 바닥의 너비
        static constexpr float trap1TriggerX = trap1X - 1.f;   // 플레이어가 이 위치까지 오면 함정 발동

        static constexpr float trap2X = 260.f; // 두 번째로 사라질 바닥의 시작 X 위치
        static constexpr float trap2Width = 50.f; // 두 번째 함정의 너비
        static constexpr float trap2TriggerX = trap2X - 30.f;  // 플레이어가 이 위치까지 오면 두 번째 함정 발동

        static constexpr float trap3X = 400.f;// 세 번째로 사라질 바닥의 시작 X 위치
        static constexpr float trap3Width = 50.f;// 세 번째 함정의 너비
        static constexpr float trap3TriggerX = trap3X - 30.f;// 플레이어가 이 위치까지 오면 세 번째 함정 발동

        static constexpr float riseX = 450.f;// 솟아오르는 바닥의 시작 X 위치
        static constexpr float riseWidth = 50.f;// 솟아오르는 바닥의 너비
        static constexpr float riseTriggerX = riseX - 40.f;// 플레이어가 이 위치까지 오면 바닥이 솟아오름
        static constexpr float riseHeight = 150.f;// 바닥이 위로 솟아오르는 높이
        static constexpr float riseDuration = 0.18f;// 바닥이 완전히 솟아오르는 데 걸리는 시간
        static constexpr float riseHoldDuration = 0.2f;// 바닥이 완전히 올라온 뒤 유지되는 시간

        static constexpr float stairX = 681.f;// 피라미드 계단의 시작 X 위치
        static constexpr float stairStepWidth = 45.f;// 계단 한 칸의 가로 길이
        static constexpr float stairStepHeight = 35.f;// 계단 한 칸의 높이
        static constexpr float stairRiseDuration = 0.08f;// 계단이 완전히 솟아오르는 데 걸리는 시간

        static constexpr float trap4X = 816.f;// 네 번째로 사라질 바닥의 시작 X 위치
        static constexpr float trap4Width = 48.f;// 네 번째 함정의 너비
        //발동조건은 Stage5.cpp update 함수에 있음

        static constexpr float trap5X = 633.f;// 다섯 번째로 사라질 바닥의 시작 X 위치
        static constexpr float trap5Width = 48.f;// 다섯 번째 함정의 너비
        static constexpr float trap5TriggerX = trap5X + trap5Width + 60.f;// 플레이어가 이 위치까지 오면 다섯 번째 함정 발동

        static constexpr float trap1OpenDuration = .27f; // 첫 번째 바닥이 완전히 열리는 데 걸리는 시간

        static constexpr float floorRestoreDuration = .27f;// 바닥이 다시 올라오는 시간

        static constexpr float doorEscapeDistance = 100.f;// 플레이어와 문이 유지할 거리
        static constexpr float doorStopX = 140.f;// 진짜 문이 도망가다 멈출 X 위치

        static constexpr float dropColumnX = 590.f;// 내려오는 기둥의 X 위치
        static constexpr float dropColumnWidth = 48.f;// 기둥 너비
        static constexpr float dropColumnHeight = 111.f;// 기둥이 내려올 최대 높이
        static constexpr float dropColumnDuration = 0.08f;// 완전히 내려오는 데 걸리는 시간
        static constexpr float dropColumnTriggerX =
            dropColumnX + dropColumnWidth + 30.f;// 왼쪽으로 이동하다 이 위치에 오면 기둥 발동

        static constexpr float trap6X = 230.f;// 여섯 번째로 사라질 바닥의 시작 X 위치
        static constexpr float trap6Width = 50.f;// 여섯 번째로 사라질 바닥의 너비
        static constexpr float trap6TriggerX = trap6X + trap6Width + 30.f;// 왼쪽으로 이동하다 이 위치에 오면 발동

        static constexpr float realDoorX = 500.f;// 3페이즈에서 나타날 진짜 문의 X 위치

        static constexpr float trap7X = 446.f;// 3페이즈에서 사라질 바닥의 시작 X 위치
        static constexpr float trap7Width = 50.f;// 3페이즈 바닥 함정 너비
        static constexpr float trap7TriggerX =
            trap7X - 30.f;// 오른쪽으로 이동하다 trap7 가까이에 오면 발동
    };

    class Stage5 final : public IStage // IStage상속받아 스테이지5 구현
    {
    public:
        Stage5();

        std::string_view label() const override { return "5"; } //스테이지 이름 반환

        Vec2 spawnPoint() const override; //시작위치 반환
        void reset() override; //리셋함수
        void update(PlayerState& player, float dt) override; //매프레임마다 상태 갱신

        const std::vector<RectF>& solids() const override //플레이어가 밟거나 부딫힐 수 있는 물체 목록
        {
            return solids_;
        }

        bool touchesHazard(const RectF& playerBounds) const override; //함정에 닿았나?
        bool isAtGoal(const PlayerState& player) const override; //문에 도착했나?
        Vec2 goalPoint() const override; //문에 들어갈 때 플레이어가 이동할 최종 위치 반환

        void draw(sf::RenderTarget& target) const override; //모든 벽 등등 그림
        float stairRiseAmount() const;// 현재 계단이 얼마나 올라왔는지 계산

        bool restartRequested() const;// 가짜 문에 들어가서 게임 처음부터 재시작을 요청했는지 반환

    private:
        std::vector<RectF> solids_; //stage5에서 충돌 가능한 물체 저장
        bool trap1Open_ = false;
        bool trap2Open_ = false;
        bool trap3Open_ = false;
        bool riseTriggered_ = false;
        bool riseActive_ = false;
        bool riseGoingDown_ = false;
        bool stairActive_ = false;
        bool doorMoved_ = false;
        bool secondPhase_ = false;// 계단과 문 이동 이후 새로운 함정 구간인지 확인
        bool secondPhaseResetDone_ = false;// 세컨드 페이즈 진입 초기화를 이미 했는지 확인

        void rebuildSolids(); //현재 상태에 맞게 물체 충돌 영역을 다시 만드는 함수
        // 첫 번째 바닥 함정이 열린 뒤 지난 시간
        float trap1Time_ = 0.f;
        float trap2Time_ = 0.f;
        float trap3Time_ = 0.f;
        float riseTime_ = 0.f;
        float stairTime_ = 0.f;
        // 첫 번째 함정이 화면에서 얼마나 깊게 열렸는지 계산
        float trap1Depth() const;
        float trap2Depth() const;
        float trap3Depth() const;
        float riseAmount() const; // 현재 바닥이 위로 솟아오른 높이 계산

        float doorX_ = Stage5Layout::doorX; //현재문의 x위치
        float doorY_ = Stage5Layout::floor; //현재문의 바닥 y위치

        bool floorRestoring_ = false;// 기존 함정 바닥들이 복구 중인지 확인
        float floorRestoreTime_ = 0.f;// 바닥 복구 애니메이션 시간

        bool trap4Open_ = false;// 네 번째 바닥 함정이 열렸는지 확인
        bool trap5Open_ = false;// 다섯 번째 바닥 함정이 열렸는지 확인

        float trap4Time_ = 0.f;// 네 번째 바닥 함정 애니메이션 시간
        float trap5Time_ = 0.f;// 다섯 번째 바닥 함정 애니메이션 시간

        float trap4Depth() const;// 네 번째 바닥이 내려간 깊이 계산
        float trap5Depth() const;// 다섯 번째 바닥이 내려간 깊이 계산

        bool dropColumnActive_ = false;// 기둥 함정이 발동했는지 확인
        float dropColumnTime_ = 0.f;// 기둥 내려오는 애니메이션 시간
        float dropColumnAmount() const;// 현재 기둥이 얼마나 내려왔는지 계산

        bool trap6Open_ = false;// 여섯 번째 바닥 함정이 열렸는지 확인
        float trap6Time_ = 0.f;// 여섯 번째 함정 애니메이션 시간
        float trap6Depth() const;// 여섯 번째 바닥이 현재 얼마나 내려갔는지 계산

        bool thirdPhase_ = false;// 문이 X=140에 도착해서 3페이즈가 시작됐는지 확인

        bool trap7Open_ = false;// 3페이즈 바닥 함정이 열렸는지 확인
        float trap7Time_ = 0.f;// 3페이즈 바닥 함정 애니메이션 시간
        float trap7Depth() const;// 3페이즈 바닥이 현재 얼마나 내려갔는지 계산

        bool restartRequested_ = false;// 가짜 문에 들어가면 true로 변경

        bool fakeMessageActive_ = false;// 가짜 문 문구를 표시 중인지 확인
        float fakeMessageTime_ = 0.f;// 가짜 문 문구를 표시한 시간
    };
}
