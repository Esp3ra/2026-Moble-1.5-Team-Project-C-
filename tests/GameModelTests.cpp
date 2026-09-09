#include "GameModel.hpp"
#include "Stage1.hpp"
#include "StageManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
int failures = 0;
int checks = 0;

void check(bool condition, const char* expression, const char* test, int line)
{
    ++checks;
    if (condition) return;
    ++failures;
    std::cerr << "FAIL " << test << ':' << line << ": " << expression << '\n';
}

#define CHECK(condition) check((condition), #condition, __func__, __LINE__)

bool near(float first, float second, float tolerance = .02f)
{
    return std::abs(first - second) <= tolerance;
}

struct TestWorld
{
    devil::Stage1 stage;
    devil::Game game{stage};
};

class AutoClearStage final : public devil::IStage
{
public:
    explicit AutoClearStage(std::string name) : name_(std::move(name))
    {
        solids_.push_back({devil::GameConfig::left, devil::Stage1Layout::floor,
            devil::GameConfig::right - devil::GameConfig::left, 210.f});
    }

    std::string_view label() const override { return name_; }
    devil::Vec2 spawnPoint() const override
    {
        return {devil::Stage1Layout::spawnX,
            devil::Stage1Layout::floor - devil::GameConfig::playerHeight};
    }
    void reset() override {}
    void update(const devil::PlayerState&, float) override {}
    const std::vector<devil::RectF>& solids() const override { return solids_; }
    bool touchesHazard(const devil::RectF&) const override { return false; }
    bool isAtGoal(const devil::PlayerState&) const override { return true; }
    devil::Vec2 goalPoint() const override
    {
        return {devil::Stage1Layout::spawnX + 20.f,
            devil::Stage1Layout::floor - devil::GameConfig::playerHeight};
    }
    void draw(sf::RenderTarget&) const override {}

private:
    std::string name_;
    std::vector<devil::RectF> solids_;
};

template<class Predicate>
bool advanceUntil(devil::Game& game, devil::Input input, Predicate done,
    int maximumSteps = 1800)
{
    for (int i = 0; i < maximumSteps; ++i)
    {
        if (done(game)) return true;
        game.update(input);
    }
    return done(game);
}

void checkAtSpawn(const TestWorld& world)
{
    const auto& game = world.game;
    CHECK(game.phase == devil::Phase::Playing);
    CHECK(near(game.player.x, devil::Stage1Layout::spawnX));
    CHECK(near(game.feet(), devil::Stage1Layout::floor));
    CHECK(near(game.player.velocityY, 0.f));
    CHECK(game.player.grounded);
    CHECK(!world.stage.holeOpen());
    CHECK(near(world.stage.holeTime(), 0.f));
    CHECK(near(world.stage.holeDepth(), 0.f));
    CHECK(near(game.player.coyoteTime, 0.f));
    CHECK(near(game.player.jumpBuffer, 0.f));
    CHECK(near(game.phaseTime, 0.f));
}

void commonStageRegistration()
{
    devil::StageManager stages;
    CHECK(stages.stageCount() == 1);
    CHECK(stages.currentIndex() == 0);
    CHECK(stages.isLastStage());
    CHECK(stages.currentStage().label() == "1 - 1");
    CHECK(&stages.game().stage() == &stages.currentStage());
}

void sequentialStageTransition()
{
    std::vector<std::unique_ptr<devil::IStage>> stageList;
    stageList.push_back(std::make_unique<AutoClearStage>("TEST 1"));
    stageList.push_back(std::make_unique<AutoClearStage>("TEST 2"));
    devil::StageManager stages(std::move(stageList));

    CHECK(stages.stageCount() == 2);
    stages.update({});
    CHECK(stages.game().phase == devil::Phase::EnteringDoor);
    for (int i = 0; i < 200 && stages.currentIndex() == 0; ++i) stages.update({});
    CHECK(stages.currentIndex() == 1);
    CHECK(stages.currentStage().label() == "TEST 2");
    CHECK(stages.game().phase == devil::Phase::Playing);
    CHECK(stages.game().elapsed > 0.f);

    stages.newRun();
    CHECK(stages.currentIndex() == 0);
    CHECK(stages.game().phase == devil::Phase::Playing);
    CHECK(near(stages.game().elapsed, 0.f));
}

void idleAndMovement()
{
    TestWorld world;
    auto& game = world.game;
    for (int i = 0; i < 360; ++i) game.update({});
    CHECK(near(game.player.x, devil::Stage1Layout::spawnX));
    CHECK(near(game.feet(), devil::Stage1Layout::floor));
    CHECK(game.player.grounded);
    CHECK(game.deaths == 0);
    CHECK(!world.stage.holeOpen());

    for (int i = 0; i < 10; ++i) game.update({1, false});
    CHECK(game.player.x > devil::Stage1Layout::spawnX);
    CHECK(game.player.facing == 1);
    for (int i = 0; i < 10; ++i) game.update({-1, false});
    CHECK(near(game.player.x, devil::Stage1Layout::spawnX));
    CHECK(game.player.facing == -1);

    for (int i = 0; i < 360; ++i) game.update({-1, false});
    CHECK(near(game.player.x, devil::GameConfig::left));
    CHECK(game.player.grounded);

    game.player.x = devil::GameConfig::right - devil::GameConfig::playerWidth - 3.f;
    for (int i = 0; i < 120; ++i) game.update({1, false});
    CHECK(near(game.player.x, devil::GameConfig::right - devil::GameConfig::playerWidth));
    CHECK(game.phase == devil::Phase::Playing);
    CHECK(game.player.grounded);
}

void jumpAndNoDoubleJump()
{
    TestWorld first;
    TestWorld second;
    auto& normal = first.game;
    auto& secondPress = second.game;
    const float startY = normal.player.y;
    normal.update({0, true});
    secondPress.update({0, true});
    CHECK(normal.player.velocityY < 0.f);
    CHECK(normal.player.y < startY);
    CHECK(!normal.player.grounded);

    float highestY = normal.player.y;
    bool descended = false;
    int landedAt = -1;
    for (int i = 0; i < 300; ++i)
    {
        normal.update({});
        secondPress.update({0, i == 11});
        highestY = std::min(highestY, normal.player.y);
        if (normal.player.velocityY > 0.f) descended = true;
        CHECK(near(normal.player.y, secondPress.player.y));
        CHECK(near(normal.player.velocityY, secondPress.player.velocityY));
        if (normal.player.grounded)
        {
            landedAt = i;
            break;
        }
    }
    CHECK(highestY < startY - devil::GameConfig::playerHeight);
    CHECK(descended);
    CHECK(landedAt > 11);
    CHECK(near(normal.feet(), devil::Stage1Layout::floor));
    CHECK(near(normal.player.velocityY, 0.f));
    CHECK(secondPress.player.grounded);

    normal.update({0, true});
    CHECK(normal.player.velocityY < 0.f);
    CHECK(!normal.player.grounded);
}

bool walkIntoTrap(TestWorld& world)
{
    auto& game = world.game;
    const bool opened = advanceUntil(game, {1, false},
        [&world](const devil::Game&) { return world.stage.holeOpen(); });
    CHECK(opened);
    CHECK(game.phase == devil::Phase::Playing);
    const bool died = advanceUntil(game, {1, false},
        [](const devil::Game& current) { return current.phase == devil::Phase::Dying; });
    CHECK(died);
    CHECK(game.player.y > devil::GameConfig::height);
    return died;
}

void progressiveFloorOpening()
{
    TestWorld world;
    auto& game = world.game;
    CHECK(advanceUntil(game, {1, false}, [&world](const devil::Game&)
        { return world.stage.holeOpen(); }));

    const auto hasSolidInsideOpening = [&world]()
    {
        return std::any_of(world.stage.solids().begin(), world.stage.solids().end(),
            [](const devil::RectF& solid)
            {
                const float center = devil::Stage1Layout::holeX +
                    devil::Stage1Layout::holeWidth * .5f;
                return solid.x < center && solid.right() > center &&
                    solid.y < devil::GameConfig::height &&
                    solid.bottom() > devil::Stage1Layout::floor;
            });
    };

    const float fullDepth = devil::GameConfig::height - devil::Stage1Layout::floor;
    const float firstDepth = world.stage.holeDepth();
    CHECK(firstDepth > 0.f);
    CHECK(firstDepth < fullDepth);
    CHECK(!hasSolidInsideOpening());
    for (int i = 0; i < 12; ++i) game.update({});
    CHECK(world.stage.holeDepth() > firstDepth + 10.f);
    CHECK(world.stage.holeDepth() < fullDepth);
    for (int i = 0; i < 60 && world.stage.holeDepth() < fullDepth; ++i) game.update({});
    CHECK(near(world.stage.holeDepth(), fullDepth));
    CHECK(!hasSolidInsideOpening());
    CHECK(game.phase == devil::Phase::Playing);
}

void deathRetryAndClear()
{
    TestWorld world;
    auto& game = world.game;
    for (int attempt = 1; attempt <= 2; ++attempt)
    {
        if (!walkIntoTrap(world)) return;
        CHECK(game.deaths == attempt);
        const float elapsedAtDeath = game.elapsed;
        CHECK(advanceUntil(game, {}, [](const devil::Game& current)
            { return current.phase == devil::Phase::Playing; }, 120));
        checkAtSpawn(world);
        CHECK(game.deaths == attempt);
        CHECK(game.elapsed >= elapsedAtDeath);
    }

    bool jumped = false;
    for (int i = 0; i < 1800 && !world.stage.isAtGoal(game.player); ++i)
    {
        const bool jumpNow = !jumped && game.player.x >= devil::Stage1Layout::holeX - 30.f;
        if (jumpNow) jumped = true;
        game.update({1, jumpNow});
    }
    CHECK(jumped);
    CHECK(world.stage.isAtGoal(game.player));
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 2);
    CHECK(world.stage.holeOpen());
    CHECK(game.player.grounded);

    const float entryStartX = game.player.x;
    for (int i = 0; i < 20; ++i) game.update({});
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.player.x > entryStartX);
    CHECK(game.player.x < world.stage.goalPoint().x);
    CHECK(advanceUntil(game, {}, [](const devil::Game& current)
        { return current.phase == devil::Phase::Cleared; }, 120));
    CHECK(near(game.player.x, world.stage.goalPoint().x));

    const float clearedX = game.player.x;
    const float clearedY = game.player.y;
    const float clearedElapsed = game.elapsed;
    for (int i = 0; i < 240; ++i) game.update({-1, true});
    CHECK(game.phase == devil::Phase::Cleared);
    CHECK(near(game.player.x, clearedX));
    CHECK(near(game.player.y, clearedY));
    CHECK(near(game.player.velocityY, 0.f));
    CHECK(near(game.elapsed, clearedElapsed));
    CHECK(game.deaths == 2);

    game.retry();
    checkAtSpawn(world);
    CHECK(game.deaths == 2);
    CHECK(near(game.elapsed, clearedElapsed));
    game.newRun();
    checkAtSpawn(world);
    CHECK(game.deaths == 0);
    CHECK(near(game.elapsed, 0.f));
}

void manualRetryDuringFall()
{
    TestWorld world;
    auto& game = world.game;
    CHECK(advanceUntil(game, {1, false}, [&world](const devil::Game& current)
        { return world.stage.holeOpen() && current.feet() > devil::Stage1Layout::floor + 5.f; }));
    CHECK(game.player.velocityY > 0.f);
    CHECK(!game.player.grounded);
    const float elapsed = game.elapsed;
    game.retry();
    checkAtSpawn(world);
    CHECK(game.deaths == 0);
    CHECK(near(game.elapsed, elapsed));
    game.update({});
    CHECK(game.player.grounded);
    CHECK(near(game.feet(), devil::Stage1Layout::floor));
}

void ceilingCollision()
{
    TestWorld world;
    auto& game = world.game;
    game.player.y = devil::GameConfig::ceiling + 1.f;
    game.player.velocityY = -devil::GameConfig::jumpSpeed;
    game.player.grounded = false;
    game.update({});
    CHECK(near(game.player.y, devil::GameConfig::ceiling));
    CHECK(game.player.velocityY >= 0.f);
    CHECK(!game.player.grounded);
    CHECK(advanceUntil(game, {}, [](const devil::Game& current) { return current.player.grounded; }));
    CHECK(near(game.feet(), devil::Stage1Layout::floor));
    CHECK(game.deaths == 0);
}

void pitSideCollisions()
{
    TestWorld rightWorld;
    auto& right = rightWorld.game;
    CHECK(advanceUntil(right, {1, false}, [&rightWorld](const devil::Game& current)
        { return rightWorld.stage.holeOpen() && current.feet() > devil::Stage1Layout::floor + 5.f; }));
    bool touchedRightSide = false;
    for (int i = 0; i < 300 && right.phase == devil::Phase::Playing; ++i)
    {
        right.update({1, false});
        CHECK(right.player.x + devil::GameConfig::playerWidth <=
            devil::Stage1Layout::holeX + devil::Stage1Layout::holeWidth + .02f);
        if (near(right.player.x + devil::GameConfig::playerWidth,
            devil::Stage1Layout::holeX + devil::Stage1Layout::holeWidth)) touchedRightSide = true;
    }
    CHECK(touchedRightSide);
    CHECK(right.phase == devil::Phase::Dying);

    TestWorld leftWorld;
    auto& left = leftWorld.game;
    CHECK(advanceUntil(left, {1, false}, [&leftWorld](const devil::Game& current)
        { return leftWorld.stage.holeOpen() && current.feet() > devil::Stage1Layout::floor + 5.f; }));
    bool touchedLeftSide = false;
    for (int i = 0; i < 300 && left.phase == devil::Phase::Playing; ++i)
    {
        left.update({-1, false});
        CHECK(left.player.x >= devil::Stage1Layout::holeX - .02f);
        if (near(left.player.x, devil::Stage1Layout::holeX)) touchedLeftSide = true;
    }
    CHECK(touchedLeftSide);
    CHECK(left.phase == devil::Phase::Dying);
}
}

int main()
{
    commonStageRegistration();
    sequentialStageTransition();
    idleAndMovement();
    jumpAndNoDoubleJump();
    manualRetryDuringFall();
    ceilingCollision();
    progressiveFloorOpening();
    pitSideCollisions();
    deathRetryAndClear();
    if (failures != 0)
    {
        std::cerr << "FAIL: " << failures << " of " << checks << " checks failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS: " << checks
        << " checks across common physics, Stage1 traps, retry, and clear flow.\n";
    return EXIT_SUCCESS;
}
