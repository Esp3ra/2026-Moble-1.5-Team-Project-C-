#include "RenderCommon.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace devil::render
{
const sf::Color earth{147, 108, 7};
const sf::Color air{247, 183, 85};
const sf::Color ink{37, 29, 16};
const sf::Color light{255, 215, 144};
const sf::Color silhouette{0, 0, 0};

namespace
{
// 공식 예고편의 연속 59.94fps 영상에서 측정한 보행 주기(약 280ms).
constexpr std::array<float, 4> walkingDurations{.096f, .060f, .057f, .067f};
constexpr float walkingCycle = .280f;
constexpr float modelWalkTicksPerSecond = 13.f;

std::array<unsigned char, 7> glyph(char c)
{
    switch (c)
    {
    case 'A': return {14,17,17,31,17,17,17};
    case 'B': return {30,17,17,30,17,17,30};
    case 'C': return {14,17,16,16,16,17,14};
    case 'D': return {30,17,17,17,17,17,30};
    case 'E': return {31,16,16,30,16,16,31};
    case 'F': return {31,16,16,30,16,16,16};
    case 'G': return {14,17,16,23,17,17,15};
    case 'H': return {17,17,17,31,17,17,17};
    case 'I': return {14,4,4,4,4,4,14};
    case 'J': return {7,2,2,2,18,18,12};
    case 'K': return {17,18,20,24,20,18,17};
    case 'L': return {16,16,16,16,16,16,31};
    case 'M': return {17,27,21,21,17,17,17};
    case 'N': return {17,25,25,21,19,19,17};
    case 'O': return {14,17,17,17,17,17,14};
    case 'P': return {30,17,17,30,16,16,16};
    case 'Q': return {14,17,17,17,21,18,13};
    case 'R': return {30,17,17,30,20,18,17};
    case 'S': return {15,16,16,14,1,1,30};
    case 'T': return {31,4,4,4,4,4,4};
    case 'U': return {17,17,17,17,17,17,14};
    case 'V': return {17,17,17,17,17,10,4};
    case 'W': return {17,17,17,21,21,21,10};
    case 'X': return {17,17,10,4,10,17,17};
    case 'Y': return {17,17,10,4,4,4,4};
    case 'Z': return {31,1,2,4,8,16,31};
    case '0': return {14,17,19,21,25,17,14};
    case '1': return {4,12,4,4,4,4,14};
    case '2': return {14,17,1,2,4,8,31};
    case '3': return {30,1,1,14,1,1,30};
    case '4': return {2,6,10,18,31,2,2};
    case '5': return {31,16,16,30,1,1,30};
    case '6': return {14,16,16,30,17,17,14};
    case '7': return {31,1,2,4,8,8,8};
    case '8': return {14,17,17,14,17,17,14};
    case '9': return {14,17,17,15,1,1,14};
    case '-': return {0,0,0,31,0,0,0};
    case '.': return {0,0,0,0,0,12,12};
    case ':': return {0,12,12,0,12,12,0};
    case '/': return {1,1,2,4,8,16,16};
    case '<': return {2,4,8,16,8,4,2};
    case '>': return {8,4,2,1,2,4,8};
    default: return {};
    }
}

float textWidth(std::string_view text, float scale)
{
    return text.empty() ? 0.f : (static_cast<float>(text.size()) * 6.f - 1.f) * scale;
}
}

std::size_t walkingFrame(float modelWalkTime)
{
    float remaining = std::fmod(modelWalkTime / modelWalkTicksPerSecond, walkingCycle);
    for (std::size_t frame = 0; frame < walkingDurations.size() - 1; ++frame)
    {
        if (remaining < walkingDurations[frame]) return frame;
        remaining -= walkingDurations[frame];
    }
    return walkingDurations.size() - 1;
}

void box(sf::RenderTarget& target, float x, float y, float width, float height, sf::Color color)
{
    if (width <= 0.f || height <= 0.f) return;
    sf::RectangleShape shape({width, height});
    shape.setPosition({std::round(x), std::round(y)});
    shape.setFillColor(color);
    target.draw(shape);
}

void label(sf::RenderTarget& target, std::string_view text, float x, float y,
    float scale, sf::Color color, bool centered)
{
    if (centered) x -= textWidth(text, scale) * .5f;
    for (const char c : text)
    {
        const auto rows = glyph(c);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (rows[row] & (1 << (4 - col)))
                    box(target, x + static_cast<float>(col) * scale,
                        y + static_cast<float>(row) * scale, scale, scale, color);
        x += 6.f * scale;
    }
}

void drawDoor(sf::RenderTarget& target, float x, float floorY, float width, float height)
{
    const float y = floorY - height;
    box(target, x - 3.f, y - 3.f, width + 6.f, height + 3.f, sf::Color(107, 79, 22));
    box(target, x, y, width, height, sf::Color(184, 184, 174));
    box(target, x + 3.f, y + 3.f, width - 6.f, height - 3.f, sf::Color(199, 201, 198));
    box(target, x + width - 7.f, y + 23.f, 3.f, 3.f, sf::Color(93, 85, 63));
}

void drawPlayer(sf::RenderTarget& target, const PlayerState& player,
    float scale, float opacity)
{
    // 공식 Steam 예고편의 지상 자세와 1-1 스크린샷의 공중 자세를
    // 2px 격자로 재구성했습니다. 자세별 외형 높이만 바뀌고 충돌 상자는 고정입니다.
    using Pose = std::array<std::string_view, 14>;
    static constexpr Pose standing{
        ".........", "...###...", "...###...", "...###...", "..#####..", "..#####..",
        "..#####..", "..#####..", "..#####..", "..#####..", "..#####..", "..##.##..",
        "..##.##..", "..##.##.."};
    static constexpr Pose walkingWide{
        ".........", "...###...", "...###...", "...###...", "..#####..", "..#####..",
        "..#####..", "..#####..", "..#####..", "..######.", "..######.", "..##..##.",
        ".##...##.", ".##......"};
    static constexpr Pose walkingNarrow{
        ".........", "...###...", "...###...", "...###...", "..#####..", "..#####..",
        "..#####..", "..#####..", "..#####..", "..#####..", "...####..", "...####..",
        "...###...", "....##..."};
    static constexpr Pose walkingOtherFoot{
        ".........", "...###...", "...###...", "...###...", "..#####..", "..#####..",
        "..#####..", "..#####..", "..#####..", "..#####..", "..#####..", ".###.##..",
        "###..##..", ".#...##.."};
    static constexpr Pose airborne{
        ".........", ".........", ".........", "...###...", "...###...", "...###...",
        ".######..", ".######..", ".######..", ".######..", "..#####..", "..#######",
        "#########", "####...##"};

    const Pose* pose = &standing;
    const bool mirrored = player.facing < 0;
    float visualOffsetY = 0.f;
    if (!player.grounded) pose = &airborne;
    else if (player.walkTime > 0.f)
    {
        const std::array<const Pose*, 4> walking{
            &airborne, &walkingOtherFoot, &walkingNarrow, &walkingWide};
        constexpr std::array<float, 4> offsets{-6.f, -2.f, 0.f, 0.f};
        const std::size_t frame = walkingFrame(player.walkTime);
        pose = walking[frame];
        visualOffsetY = offsets[frame];
    }

    constexpr int columns = 9;
    scale = std::clamp(scale, .05f, 1.f);
    const float pixel = 2.f * scale;
    sf::Color playerColor = silhouette;
    playerColor.a = static_cast<std::uint8_t>(std::round(
        std::clamp(opacity, 0.f, 1.f) * 255.f));
    const float x = std::round(player.centerX()) - columns * pixel * .5f;
    const float y = std::round(player.feet()) - static_cast<float>(pose->size()) * pixel
        + visualOffsetY * scale;
    for (std::size_t row = 0; row < pose->size(); ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int sourceColumn = mirrored ? columns - 1 - column : column;
            if ((*pose)[row][sourceColumn] == '#')
                box(target, x + static_cast<float>(column) * pixel,
                    y + static_cast<float>(row) * pixel, pixel, pixel, playerColor);
        }
    }
}
}
