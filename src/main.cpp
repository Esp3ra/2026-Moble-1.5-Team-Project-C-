#include "GameModel.hpp"
#include "IStage.hpp"
#include "RenderCommon.hpp"
#include "Stage1.hpp"
#include "StageManager.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace
{
using devil::Game;
using devil::GameConfig;
using devil::IStage;
using devil::Phase;
using devil::PlayerState;
using devil::Stage1;
using devil::Stage1Layout;
using namespace devil::render;

constexpr std::size_t plannedStageCount = 6;

void drawTopHud(sf::RenderTarget& target, std::size_t stageIndex, std::size_t stageCount)
{
    // 원작 1-1처럼 조작 아이콘은 왼쪽 위, 방 진행 칸은 화면 위 중앙에 둡니다.
    box(target, 20.f, 19.f, 7.f, 30.f, ink);
    box(target, 34.f, 19.f, 7.f, 30.f, ink);

    // 픽셀 형태의 다시 시작 화살표입니다.
    box(target, 64.f, 19.f, 25.f, 5.f, ink);
    box(target, 64.f, 19.f, 5.f, 26.f, ink);
    box(target, 64.f, 40.f, 25.f, 5.f, ink);
    box(target, 84.f, 34.f, 5.f, 11.f, ink);
    box(target, 59.f, 14.f, 5.f, 15.f, ink);
    box(target, 54.f, 19.f, 15.f, 5.f, ink);

    stageCount = std::clamp<std::size_t>(stageCount, 1, 8);
    stageIndex = std::min(stageIndex, stageCount - 1);
    constexpr float side = 16.f;
    constexpr float gap = 7.f;
    const float totalWidth = static_cast<float>(stageCount) * side +
        static_cast<float>(stageCount - 1) * gap;
    const float startX = (GameConfig::width - totalWidth) * .5f;
    for (std::size_t index = 0; index < stageCount; ++index)
    {
        const float x = startX + static_cast<float>(index) * (side + gap);
        box(target, x, 20.f, side, side, ink);
        if (index > stageIndex) box(target, x + 3.f, 23.f, side - 6.f, side - 6.f, earth);
    }
}

void drawScene(sf::RenderTarget& target, const IStage& stage, const Game& game,
    bool paused, bool finalStage = true, std::size_t stageIndex = 0,
    std::size_t stageCount = plannedStageCount)
{
    target.clear(earth);
    stage.draw(target);

    if (game.phase != Phase::Dying && game.phase != Phase::Cleared)
    {
        if (game.phase == Phase::EnteringDoor)
        {
            const float amount = std::clamp(game.phaseTime / GameConfig::doorEnterDuration, 0.f, 1.f);
            const float smooth = amount * amount * (3.f - 2.f * amount);
            const float opacity = 1.f - std::clamp((amount - .55f) / .45f, 0.f, 1.f);
            drawPlayer(target, game.player, 1.f - smooth * .4f, opacity);
        }
        else
        {
            drawPlayer(target, game.player);
        }
    }

    drawTopHud(target, stageIndex, stageCount);

    if (game.phase == Phase::Cleared || paused)
    {
        box(target, 240.f, 171.f, 480.f, 233.f, sf::Color(107, 78, 13));
        box(target, 246.f, 177.f, 468.f, 221.f, air);
        const std::string clearTitle = "LEVEL " + std::string(stage.label()) + " CLEAR";
        label(target, paused ? "PAUSED" : clearTitle, 480.f, 204.f, 3.f, ink, true);
        if (paused)
        {
            label(target, "ESC / P  RESUME", 480.f, 265.f, 2.f, ink, true);
            label(target, "R  RETRY     Q  QUIT", 480.f, 304.f, 1.f, ink, true);
        }
        else
        {
            const auto seconds = std::to_string(static_cast<int>(game.elapsed));
            label(target, "DEATHS " + std::to_string(game.deaths) + "   TIME " + seconds + "S",
                480.f, 262.f, 1.f, ink, true);
            label(target, finalStage ? "ALL STAGES COMPLETE" : "NEXT STAGE",
                480.f, 291.f, 1.f, ink, true);
            if (finalStage)
            {
                box(target, 354.f, 331.f, 252.f, 40.f, ink);
                label(target, "ENTER  PLAY AGAIN", 480.f, 345.f, 1.f, light, true);
            }
            else
            {
                label(target, "GET READY", 480.f, 345.f, 1.f, ink, true);
            }
        }
    }
}

bool saveFrame(sf::RenderTexture& surface, const IStage& stage, const Game& game,
    const std::filesystem::path& path)
{
    drawScene(surface, stage, game, false);
    surface.display();
    return surface.getTexture().copyToImage().saveToFile(path);
}

bool savePlayerPoses(const std::filesystem::path& path)
{
    sf::RenderTexture sheet({960u, 540u});
    sf::RenderTexture stamp({24u, 32u});
    sheet.clear(air);
    label(sheet, "CHARACTER POSES - 5X AND ACTUAL SIZE", 480.f, 16.f, 2.f, ink, true);
    constexpr std::array<std::string_view, 6> names{
        "IDLE", "WALK 1", "WALK 2", "WALK 3", "WALK 4", "JUMP"};
    constexpr std::array<float, 4> durations{.096f, .060f, .057f, .067f};
    std::array<float, 6> times{};
    float elapsed = 0.f;
    for (std::size_t frame = 0; frame < durations.size(); ++frame)
    {
        times[frame + 1] = (elapsed + durations[frame] * .5f) * 13.f;
        elapsed += durations[frame];
    }
    for (int row = 0; row < 2; ++row)
    {
        const float top = 55.f + static_cast<float>(row) * 235.f;
        label(sheet, row == 0 ? "FACING RIGHT" : "FACING LEFT", 16.f, top, 1.f, ink);
        for (std::size_t column = 0; column < names.size(); ++column)
        {
            const float left = static_cast<float>(column) * 160.f;
            label(sheet, names[column], left + 80.f, top + 22.f, 1.f, ink, true);
            PlayerState pose;
            pose.x = 4.f;
            pose.y = 0.f;
            pose.facing = row == 0 ? 1 : -1;
            pose.walkTime = times[column];
            pose.grounded = column != names.size() - 1;
            stamp.clear(sf::Color::Transparent);
            drawPlayer(stamp, pose);
            stamp.display();
            sf::Sprite enlarged(stamp.getTexture());
            enlarged.setPosition({left + 4.f, top + 45.f});
            enlarged.setScale({5.f, 5.f});
            sheet.draw(enlarged);
            pose.x = left + 132.f;
            pose.y = top + 157.f;
            drawPlayer(sheet, pose);
            box(sheet, left + 8.f, top + 185.f, 144.f, 1.f, earth);
        }
    }
    sheet.display();
    return sheet.getTexture().copyToImage().saveToFile(path);
}

int verifyMotion(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    sf::RenderTexture surface({960u, 540u});
    sf::RenderTexture stamp({24u, 32u});
    std::ofstream timeline(directory / "timeline.csv");
    timeline << "frame,seconds,direction,grounded,walking_frame\n";
    Stage1 stage;
    Game game(stage);
    std::array<bool, 4> sawWalkingFrames{};
    bool sawLeft = false;
    bool sawRight = false;
    bool sawJump = false;
    bool sawLanding = false;
    for (int frame = 0; frame < 240; ++frame)
    {
        const int direction = frame >= 18 && frame < 84 ? 1
            : frame >= 102 && frame < 168 ? -1
            : frame >= 186 && frame < 228 ? 1 : 0;
        for (int step = 0; step < 2; ++step)
            game.update({direction, frame == 186 && step == 0});
        if (game.phase != Phase::Playing) return 1;
        if (!game.player.grounded) sawJump = true;
        else if (sawJump) sawLanding = true;
        if (game.player.grounded && game.player.walkTime > 0.f)
        {
            sawWalkingFrames[walkingFrame(game.player.walkTime)] = true;
            sawLeft = sawLeft || game.player.facing < 0;
            sawRight = sawRight || game.player.facing > 0;
        }
        drawScene(surface, stage, game, false);
        label(surface, "WALK MOTION - 5X", 480.f, 76.f, 1.f, light, true);
        PlayerState pose = game.player;
        pose.x = 4.f;
        pose.y = 0.f;
        stamp.clear(air);
        drawPlayer(stamp, pose);
        box(stamp, 0.f, 28.f, 24.f, 4.f, earth);
        stamp.display();
        sf::Sprite enlarged(stamp.getTexture());
        enlarged.setPosition({420.f, 94.f});
        enlarged.setScale({5.f, 5.f});
        surface.draw(enlarged);
        surface.display();
        std::ostringstream filename;
        filename << "frame-" << std::setw(4) << std::setfill('0') << frame << ".png";
        if (!surface.getTexture().copyToImage().saveToFile(directory / filename.str())) return 1;
        const int frameIndex = game.player.grounded && game.player.walkTime > 0.f
            ? static_cast<int>(walkingFrame(game.player.walkTime)) : -1;
        timeline << frame << ',' << static_cast<float>(frame + 1) / 60.f << ','
            << direction << ',' << game.player.grounded << ',' << frameIndex << '\n';
    }
    const bool ok = timeline.good() && sawLeft && sawRight && sawJump && sawLanding &&
        std::all_of(sawWalkingFrames.begin(), sawWalkingFrames.end(), [](bool seen) { return seen; });
    std::ofstream result(directory / "verification.txt");
    result << (ok ? "PASS" : "FAIL") << "\n60 FPS / 240 frames / production renderer\n"
        << "Idle -> right -> idle -> left -> idle -> jump -> land -> idle\n"
        << "All walking poses, both facings, jump and landing observed: " << ok << '\n';
    return ok ? 0 : 1;
}

int verify(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    sf::RenderTexture surface({960u, 540u});
    Stage1 stage;
    Game game(stage);
    bool ok = saveFrame(surface, stage, game, directory / "01-start.png");
    bool sawDeath = false;
    for (int i = 0; i < 900; ++i)
    {
        game.update({1, false});
        if (game.phase == Phase::Dying) sawDeath = true;
        if (sawDeath && game.phase == Phase::Playing) break;
    }
    ok = ok && sawDeath && game.deaths == 1 && !stage.holeOpen() &&
        game.player.x == Stage1Layout::spawnX;
    bool jumped = false;
    bool savedTrap = false;
    bool savedJump = false;
    bool reachedDoor = false;
    for (int i = 0; i < 1000 && game.phase == Phase::Playing; ++i)
    {
        if (stage.isAtGoal(game.player))
        {
            reachedDoor = true;
            break;
        }
        const bool jumpNow = !jumped && game.player.x >= Stage1Layout::holeX - 28.f;
        game.update({1, jumpNow});
        jumped = jumped || jumpNow;
        const float fullHoleDepth = GameConfig::height - Stage1Layout::floor;
        if (stage.holeOpen() && stage.holeDepth() >= 36.f &&
            stage.holeDepth() < fullHoleDepth && !savedTrap)
        {
            ok = saveFrame(surface, stage, game, directory / "02-trap.png") && ok;
            savedTrap = true;
        }
        if (jumped && game.centerX() >= Stage1Layout::holeX + 5.f && !savedJump)
        {
            ok = saveFrame(surface, stage, game, directory / "03-jump.png") && ok;
            savedJump = true;
        }
    }
    reachedDoor = reachedDoor || game.phase == Phase::EnteringDoor;
    ok = ok && savedTrap && savedJump && reachedDoor && game.phase == Phase::EnteringDoor;
    const float entryStartX = game.player.x;
    for (int i = 0; i < 20; ++i) game.update({});
    const bool enteredDoor = game.phase == Phase::EnteringDoor && game.player.x > entryStartX;
    ok = saveFrame(surface, stage, game, directory / "04-door-entering.png") &&
        enteredDoor && ok;
    for (int i = 0; i < 120 && game.phase != Phase::Cleared; ++i) game.update({});
    ok = game.phase == Phase::Cleared && ok;
    ok = saveFrame(surface, stage, game, directory / "05-clear.png") && ok;
    const bool savedPoses = savePlayerPoses(directory / "06-character-poses.png");
    ok = savedPoses && ok;
    std::ofstream result(directory / "verification.txt");
    result << (ok ? "PASS" : "FAIL") << "\nSFML offscreen rendering: 960x540\n"
        << "Walk into pit -> die -> respawn: " << sawDeath << "\n"
        << "Floor gap opened progressively: " << savedTrap << "\n"
        << "Touch door -> automatic entry motion: " << enteredDoor << "\n"
        << "Door entry -> clear: " << (game.phase == Phase::Cleared) << "\n"
        << "Character pose sheet (idle, walk, jump; both facings): " << savedPoses << "\n"
        << "Deaths: " << game.deaths << "\n";
    return ok ? 0 : 1;
}

void updateView(sf::RenderWindow& window)
{
    const auto size = window.getSize();
    if (size.x == 0 || size.y == 0) return;
    sf::View view(sf::FloatRect({0.f, 0.f}, {GameConfig::width, GameConfig::height}));
    const float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
    constexpr float desired = GameConfig::width / GameConfig::height;
    if (aspect > desired)
    {
        const float width = desired / aspect;
        view.setViewport(sf::FloatRect({(1.f - width) * .5f, 0.f}, {width, 1.f}));
    }
    else
    {
        const float height = aspect / desired;
        view.setViewport(sf::FloatRect({0.f, (1.f - height) * .5f}, {1.f, height}));
    }
    window.setView(view);
}
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc >= 3 && std::string_view(argv[1]) == "--verify") return verify(argv[2]);
        if (argc >= 3 && std::string_view(argv[1]) == "--verify-motion") return verifyMotion(argv[2]);

        sf::RenderWindow window(sf::VideoMode({960u, 540u}),
            "Level Devil - Stage 1-1 | C++ / SFML");
        window.setVerticalSyncEnabled(true);
        window.setKeyRepeatEnabled(false);
        updateView(window);

        devil::StageManager stages;
        Game& game = stages.game();
        bool paused = false;
        bool jumpQueued = false;
        float accumulator = 0.f;
        sf::Clock clock;

        while (window.isOpen())
        {
            const float frameTime = std::min(clock.restart().asSeconds(), .1f);
            while (const auto event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>()) window.close();
                if (event->is<sf::Event::Resized>()) updateView(window);
                if (event->is<sf::Event::FocusLost>())
                {
                    paused = true;
                    jumpQueued = false;
                }
                if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                {
                    using K = sf::Keyboard::Key;
                    if (key->code == K::Escape || key->code == K::P)
                    {
                        paused = !paused;
                        jumpQueued = false;
                    }
                    if (key->code == K::Q && paused) window.close();
                    if (key->code == K::R ||
                        (key->code == K::Enter && game.phase == Phase::Cleared && stages.isLastStage()))
                    {
                        if (game.phase == Phase::Cleared) stages.newRun();
                        else stages.retry();
                        paused = false;
                        accumulator = 0.f;
                        jumpQueued = false;
                    }
                    if (!paused && game.phase == Phase::Playing &&
                        (key->code == K::Space || key->code == K::W || key->code == K::Up))
                        jumpQueued = true;
                }
                if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>();
                    mouse && mouse->button == sf::Mouse::Button::Left)
                {
                    const auto point = window.mapPixelToCoords(mouse->position);
                    if (sf::FloatRect({12.f, 12.f}, {38.f, 44.f}).contains(point))
                    {
                        paused = !paused;
                        jumpQueued = false;
                    }
                    else if (sf::FloatRect({53.f, 11.f}, {43.f, 43.f}).contains(point))
                    {
                        stages.retry();
                        paused = false;
                        accumulator = 0.f;
                        jumpQueued = false;
                    }
                    else if (game.phase == Phase::Cleared && stages.isLastStage() && !paused &&
                        sf::FloatRect({354.f, 331.f}, {252.f, 40.f}).contains(point))
                    {
                        stages.newRun();
                    }
                }
            }
            if (!window.isOpen()) break;

            if (paused || !window.hasFocus())
            {
                accumulator = 0.f;
                jumpQueued = false;
            }
            else
            {
                accumulator += frameTime;
                while (accumulator >= GameConfig::step)
                {
                    using K = sf::Keyboard::Key;
                    const int direction = static_cast<int>(sf::Keyboard::isKeyPressed(K::D) ||
                        sf::Keyboard::isKeyPressed(K::Right)) -
                        static_cast<int>(sf::Keyboard::isKeyPressed(K::A) ||
                            sf::Keyboard::isKeyPressed(K::Left));
                    stages.update({direction, jumpQueued});
                    jumpQueued = false;
                    accumulator -= GameConfig::step;
                }
            }
            drawScene(window, stages.currentStage(), game, paused, stages.isLastStage(),
                stages.currentIndex(), std::max(plannedStageCount, stages.stageCount()));
            window.display();
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::ofstream("LevelDevil-error.log") << error.what() << '\n';
        return 1;
    }
}
