#pragma once

namespace devil
{
// 모든 스테이지가 공유하는 화면, 플레이어 충돌 상자, 이동 물리 값입니다.
// 팀 작업 중에는 스테이지 파일에서 이 값을 다시 정의하지 않습니다.
struct GameConfig
{
    static constexpr float width = 960.f;
    static constexpr float height = 540.f;
    static constexpr float left = 96.f;
    static constexpr float right = 864.f;
    static constexpr float ceiling = 250.f;
    static constexpr float playerWidth = 16.f;
    static constexpr float playerHeight = 28.f;
    // Push 참고 영상의 화면 비율과 30fps 프레임 이동량을 기준으로 맞춘 공통 감각입니다.
    // 좌우 입력은 즉시 반응시키고 공중에서도 같은 속도를 유지합니다.
    static constexpr float speed = 192.f;
    static constexpr float gravity = 2048.f;
    static constexpr float jumpSpeed = 472.f;
    static constexpr float step = 1.f / 120.f;
    static constexpr float deathDelay = .38f;
    static constexpr float doorEnterDuration = .42f;
};

// 기존 코드에서 Level로 부르던 공통 상수의 호환 이름입니다.
using Level = GameConfig;

struct Vec2
{
    float x = 0.f;
    float y = 0.f;
};

struct RectF
{
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;

    float right() const { return x + width; }
    float bottom() const { return y + height; }
    bool intersects(const RectF& other) const
    {
        return x < other.right() && right() > other.x &&
            y < other.bottom() && bottom() > other.y;
    }
};

struct Input
{
    int direction = 0;
    bool jumpPressed = false;
};

enum class Phase { Playing, Dying, EnteringDoor, Cleared };

struct PlayerState
{
    float x = 0.f;
    float y = 0.f;
    float velocityY = 0.f;
    float coyoteTime = 0.f;
    float jumpBuffer = 0.f;
    float walkTime = 0.f;
    bool grounded = false;
    int facing = 1;

    float centerX() const { return x + GameConfig::playerWidth * .5f; }
    float feet() const { return y + GameConfig::playerHeight; }
    RectF bounds() const
    {
        return {x, y, GameConfig::playerWidth, GameConfig::playerHeight};
    }
};
}
