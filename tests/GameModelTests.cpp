#include "GameModel.hpp"
#include "FinalStage.hpp"
#include "Stage1.hpp"
#include "Stage2.hpp"
#include "Stage3.hpp"
#include "Stage4.hpp"
#include "Stage5.hpp"
#include "Stage6.hpp"
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
    void update(devil::PlayerState&, float) override {}
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
    CHECK(stages.stageCount() == 7);
    CHECK(stages.currentIndex() == 0);
    CHECK(!stages.isLastStage());
    CHECK(stages.currentStage().label() == "1 - 1");
    CHECK(&stages.game().stage() == &stages.currentStage());

    stages.game().player.x = devil::Stage1Layout::doorX;
    stages.game().player.y = devil::Stage1Layout::floor - devil::GameConfig::playerHeight;
    stages.update({});
    CHECK(stages.game().phase == devil::Phase::EnteringDoor);
    for (int i = 0; i < 240 && stages.currentIndex() == 0; ++i) stages.update({});
    CHECK(stages.currentIndex() == 1);
    CHECK(stages.currentStage().label() == "2 - 5");
    CHECK(!stages.isLastStage());
    CHECK(near(stages.game().player.x, devil::Stage2Layout::spawnX));
    CHECK(near(stages.game().feet(), devil::Stage2Layout::floor));
}

void stageThreeFourAssignment()
{
    devil::Stage3 coinsStage;
    devil::Stage4 pushStage;
    CHECK(coinsStage.label() == "4 - 1");
    CHECK(pushStage.label() == "1 - 3");
    CHECK(near(devil::Stage3Layout::coinSize, 15.f));
    CHECK(near(devil::Stage4Layout::startBlockWidth, 28.f));
}

bool solidAt(const devil::IStage& stage, float x, float y)
{
    const devil::RectF probe{x, y, 1.f, 1.f};
    return std::any_of(stage.solids().begin(), stage.solids().end(),
        [&](const devil::RectF& solid) { return solid.intersects(probe); });
}

void checkSolidGeometry(const devil::IStage& stage)
{
    for (const devil::RectF& solid : stage.solids())
    {
        CHECK(std::isfinite(solid.x));
        CHECK(std::isfinite(solid.y));
        CHECK(solid.width > 0.f);
        CHECK(solid.height > 0.f);
    }
}

void mergedStageReachabilityMargins()
{
    const float jumpHeight = devil::GameConfig::jumpSpeed * devil::GameConfig::jumpSpeed
        / (2.f * devil::GameConfig::gravity);
    const float jumpRange = devil::GameConfig::speed
        * (2.f * devil::GameConfig::jumpSpeed / devil::GameConfig::gravity + .055f);

    CHECK(jumpRange > devil::Stage2Layout::holeWidth
        + devil::Stage2Layout::edgeWidth + devil::GameConfig::playerWidth);
    CHECK(jumpRange > devil::Stage3Layout::holeWidth + devil::GameConfig::playerWidth);
    CHECK(jumpRange > devil::Stage4Layout::leftHoleWidth + devil::GameConfig::playerWidth);
    CHECK(jumpRange > devil::Stage5Layout::trap1Width + devil::GameConfig::playerWidth);
    CHECK(jumpHeight > devil::Stage3Layout::floorY - devil::Stage3Layout::platform1Y);
    CHECK(jumpHeight > devil::Stage4Layout::startBlockHeight);
    CHECK(jumpHeight > devil::Stage5Layout::stairStepHeight);
    CHECK(devil::Stage5Layout::floor
        - (devil::GameConfig::ceiling + devil::Stage5Layout::dropColumnHeight)
        > devil::GameConfig::playerHeight);
}

void mergedStageMechanics()
{
    {
        devil::Stage2 stage;
        devil::PlayerState player{};
        player.x = devil::Stage2Layout::firstMoveTriggerX;
        player.y = devil::Stage2Layout::floor - devil::GameConfig::playerHeight;
        player.grounded = true;
        stage.update(player, devil::Stage2Layout::spikeMoveDuration);
        CHECK(stage.firstSpikeX() < devil::Stage2Layout::firstSpikeX);

        player.x = devil::Stage2Layout::holeTriggerX;
        stage.update(player, devil::GameConfig::step);
        CHECK(stage.holeOpen());
        CHECK(!solidAt(stage, devil::Stage2Layout::holeX + 1.f,
            devil::Stage2Layout::floor + 1.f));
        checkSolidGeometry(stage);
        stage.reset();
        CHECK(!stage.holeOpen());
    }

    {
        devil::Stage3 stage;
        devil::PlayerState player{};
        player.x = devil::Stage3Layout::coin1X;
        player.y = devil::Stage3Layout::floorY - devil::GameConfig::playerHeight;
        player.grounded = true;
        stage.update(player, devil::GameConfig::step);
        CHECK(!solidAt(stage, devil::Stage3Layout::holeX + 1.f,
            devil::Stage3Layout::floorY + 1.f));

        player.x = devil::Stage3Layout::coin2X;
        player.y = devil::Stage3Layout::platform1Y - devil::GameConfig::playerHeight;
        stage.update(player, devil::GameConfig::step);
        player.x = devil::GameConfig::right - devil::GameConfig::playerWidth;
        stage.update(player, 10.f);
        CHECK(stage.touchesHazard({devil::Stage3Layout::wallStopX,
            devil::Stage3Layout::wallY, devil::GameConfig::playerWidth,
            devil::GameConfig::playerHeight}));

        player.x = devil::Stage3Layout::coin3X;
        player.y = devil::Stage3Layout::floorY - devil::GameConfig::playerHeight;
        stage.update(player, devil::GameConfig::step);
        const devil::Vec2 goal = stage.goalPoint();
        player.x = goal.x;
        player.y = goal.y;
        CHECK(stage.isAtGoal(player));
        checkSolidGeometry(stage);
        stage.reset();
        CHECK(!stage.isAtGoal(player));
    }

    {
        devil::Stage4 stage;
        devil::PlayerState player{};
        player.x = devil::Stage4Layout::spawnX;
        player.y = devil::Stage4Layout::floor - devil::GameConfig::playerHeight;
        player.grounded = true;
        stage.update(player, .16f);
        CHECK(player.x > devil::Stage4Layout::spawnX);

        player.x = devil::Stage4Layout::leftHoleX - devil::GameConfig::playerWidth;
        stage.update(player, .3f);
        CHECK(solidAt(stage,
            devil::Stage4Layout::leftHoleX + devil::Stage4Layout::leftHoleWidth + 1.f,
            devil::Stage4Layout::floor - 10.f));

        player.x = devil::Stage4Layout::blockTriggerX;
        stage.update(player, .2f);
        const float approachingWallX = devil::GameConfig::right
            - devil::Stage4Layout::blockApproachSpeed * .2f;
        CHECK(stage.touchesHazard({approachingWallX,
            devil::Stage4Layout::floor - devil::Stage4Layout::blockHeight,
            devil::GameConfig::playerWidth, devil::GameConfig::playerHeight}));
        checkSolidGeometry(stage);
        stage.reset();
    }

    {
        devil::Stage5 stage;
        devil::PlayerState player{};
        player.y = devil::Stage5Layout::floor - devil::GameConfig::playerHeight;
        player.grounded = true;
        player.x = devil::Stage5Layout::trap1TriggerX - devil::GameConfig::playerWidth;
        stage.update(player, devil::GameConfig::step);
        CHECK(!solidAt(stage, devil::Stage5Layout::trap1X + 1.f,
            devil::Stage5Layout::floor + 1.f));

        player.x = devil::Stage5Layout::doorX - 10.f - devil::GameConfig::playerWidth;
        stage.update(player, devil::GameConfig::step);
        player.x = devil::Stage5Layout::doorStopX + devil::Stage5Layout::doorEscapeDistance;
        stage.update(player, devil::GameConfig::step);
        const devil::Vec2 goal = stage.goalPoint();
        player.x = goal.x;
        player.y = goal.y;
        CHECK(stage.isAtGoal(player));
        checkSolidGeometry(stage);

        player.x = devil::Stage5Layout::doorStopX;
        player.y = devil::Stage5Layout::floor - devil::GameConfig::playerHeight;
        stage.update(player, devil::GameConfig::step);
        stage.update(player, 1.f);
        CHECK(stage.restartRequested());
        stage.reset();
        CHECK(!stage.restartRequested());

        player.x = devil::Stage5Layout::riseTriggerX;
        player.y = devil::Stage5Layout::floor - devil::GameConfig::playerHeight;
        player.grounded = true;
        stage.update(player, devil::GameConfig::step);
        stage.update(player, devil::Stage5Layout::riseDuration
            + devil::Stage5Layout::riseHoldDuration);
        stage.update(player, devil::Stage5Layout::riseDuration);
        CHECK(std::all_of(stage.solids().begin(), stage.solids().end(),
            [](const devil::RectF& solid) { return solid.height > 0.f; }));
        stage.update(player, devil::GameConfig::step);
        CHECK(!solidAt(stage, devil::Stage5Layout::riseX + 1.f,
            devil::Stage5Layout::floor - 1.f));
    }
}

void stage2PlayableRoute()
{
    devil::Stage2 stage;
    devil::Game game(stage);
    bool firstSpikeJump = false;
    bool mainGapJump = false;
    bool landingJump = false;
    bool finalSpikeJump = false;

    for (int frame = 0; frame < 2400 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{-1, false};
        if (!firstSpikeJump && game.player.x <= 620.f && game.player.grounded)
        {
            input.jumpPressed = true;
            firstSpikeJump = true;
        }
        else if (firstSpikeJump && !mainGapJump && game.player.x <= 510.f && game.player.grounded)
        {
            input.jumpPressed = true;
            mainGapJump = true;
        }
        else if (mainGapJump && !landingJump && game.player.x <= 430.f && game.player.grounded)
        {
            input.jumpPressed = true;
            landingJump = true;
        }
        else if (landingJump && !finalSpikeJump && game.player.x <= 315.f && game.player.grounded)
        {
            input.jumpPressed = true;
            finalSpikeJump = true;
        }
        game.update(input);
    }

    CHECK(firstSpikeJump);
    CHECK(mainGapJump);
    CHECK(landingJump);
    CHECK(finalSpikeJump);
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 0);
}

void stage3PlayableRoute()
{
    devil::Stage3 stage;
    devil::Game game(stage);
    bool holeJump = false;
    bool platformJump = false;
    bool spikeJump = false;
    bool returning = false;
    bool returnSpikeJump = false;
    bool platformReturnJump = false;

    for (int frame = 0; frame < 3000 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{returning ? -1 : 1, false};
        if (returning && returnSpikeJump && !platformReturnJump
            && game.player.x <= 582.f && !game.player.grounded)
            input.direction = 0;
        if (!returning && !holeJump && game.player.x >= 332.f && game.player.grounded)
        {
            input.jumpPressed = true;
            holeJump = true;
        }
        else if (!returning && holeJump && !platformJump && game.player.x >= 430.f
            && game.player.grounded)
        {
            input.jumpPressed = true;
            platformJump = true;
        }
        else if (!returning && platformJump && !spikeJump && game.player.x >= 565.f
            && (game.player.grounded || game.player.coyoteTime > 0.f))
        {
            input.jumpPressed = true;
            spikeJump = true;
        }
        else if (returning && !returnSpikeJump && game.player.x <= 640.f
            && game.player.grounded)
        {
            input.jumpPressed = true;
            returnSpikeJump = true;
        }
        else if (returning && returnSpikeJump && !platformReturnJump
            && game.player.x <= 582.f && game.player.grounded)
        {
            input.direction = -1;
            input.jumpPressed = true;
            platformReturnJump = true;
        }

        game.update(input);
        if (!returning && game.player.x >= 680.f)
            returning = true;
    }

    CHECK(holeJump);
    CHECK(platformJump);
    CHECK(spikeJump);
    CHECK(returning);
    CHECK(returnSpikeJump);
    CHECK(platformReturnJump);
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 0);
}

void stage4PlayableRoute()
{
    devil::Stage4 stage;
    devil::Game game(stage);
    int mode = 0;
    int waitFrames = 0;
    bool startJump = false;
    bool gapJump = false;
    bool doorJump = false;

    for (int frame = 0; frame < 3600 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{};
        if (mode == 0)
        {
            input.jumpPressed = true;
            startJump = true;
            mode = 1;
            waitFrames = 90;
        }
        else if (mode == 1)
        {
            if (--waitFrames <= 0) mode = 2;
        }
        else if (mode == 2)
        {
            input.direction = 1;
            if (game.player.x >= devil::Stage4Layout::leftHoleX
                - devil::GameConfig::playerWidth)
            {
                mode = 3;
                waitFrames = 280;
                input.direction = 0;
            }
        }
        else if (mode == 3)
        {
            if (--waitFrames <= 0) mode = 4;
        }
        else if (mode == 4)
        {
            input.direction = 1;
            if (!gapJump && game.player.grounded)
            {
                input.jumpPressed = true;
                gapJump = true;
            }
            if (game.player.x >= devil::Stage4Layout::blockTriggerX)
            {
                mode = 5;
                waitFrames = 180;
                input.direction = -1;
            }
        }
        else if (mode == 5)
        {
            input.direction = -1;
            if (--waitFrames <= 0) mode = 6;
        }
        else if (mode == 6)
        {
            input.direction = 1;
            if (game.player.x >= devil::Stage4Layout::blockTriggerX)
            {
                mode = 7;
                waitFrames = 60;
                input.direction = 0;
            }
        }
        else if (mode == 7)
        {
            if (--waitFrames <= 0) mode = 8;
        }
        else
        {
            input.direction = 1;
            if (!doorJump && game.player.x >= 714.f && game.player.grounded)
            {
                input.jumpPressed = true;
                doorJump = true;
            }
        }
        game.update(input);
    }

    CHECK(startJump);
    CHECK(gapJump);
    CHECK(doorJump);
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 0);
}

void stage5PlayableRoute()
{
    devil::Stage5 stage;
    devil::Game game(stage);
    int mode = 0;
    int waitFrames = 0;
    bool firstJump = false;
    bool secondJump = false;
    bool thirdJump = false;
    bool trap5Jump = false;
    bool trap6LeftJump = false;
    bool trap6RightJump = false;
    bool realDoorJump = false;
    int stairLevel = 0;
    int returnStairState = 0;

    for (int frame = 0; frame < 6000 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{};
        if (mode == 0)
        {
            input.direction = 1;
            input.jumpPressed = true;
            firstJump = true;
            mode = 1;
        }
        else if (mode == 1)
        {
            input.direction = 1;
            if (game.player.grounded && game.player.x > devil::Stage5Layout::trap1X
                + devil::Stage5Layout::trap1Width)
                mode = 2;
        }
        else if (mode == 2)
        {
            input.direction = 1;
            if (game.player.x >= 238.f && game.player.grounded)
            {
                input.jumpPressed = true;
                secondJump = true;
                mode = 3;
            }
        }
        else if (mode == 3)
        {
            input.direction = 1;
            if (game.player.grounded && game.player.x > devil::Stage5Layout::trap2X
                + devil::Stage5Layout::trap2Width)
                mode = 4;
        }
        else if (mode == 4)
        {
            input.direction = 1;
            if (game.player.x >= 394.f)
            {
                input.direction = 0;
                mode = 5;
                waitFrames = 80;
            }
        }
        else if (mode == 5)
        {
            if (--waitFrames <= 0) mode = 6;
        }
        else if (mode == 6)
        {
            input.direction = 1;
            if (game.player.grounded)
            {
                input.jumpPressed = true;
                thirdJump = true;
                mode = 7;
            }
        }
        else if (mode == 7)
        {
            input.direction = 1;
            if (game.player.grounded && game.player.x > devil::Stage5Layout::riseX
                + devil::Stage5Layout::riseWidth)
                mode = 8;
        }
        else if (mode == 8)
        {
            input.direction = 1;
            const float feet = game.player.feet();
            if (stairLevel == 0 && game.player.x >= 650.f && game.player.grounded)
            {
                input.jumpPressed = true;
                stairLevel = 1;
            }
            else if (stairLevel == 1 && game.player.grounded && feet < 370.f)
            {
                input.jumpPressed = true;
                stairLevel = 2;
            }
            else if (stairLevel == 2 && !game.player.grounded && game.player.x >= 750.f)
                input.direction = 0;
            else if (stairLevel == 2 && game.player.grounded && feet < 335.f)
            {
                input.jumpPressed = true;
                stairLevel = 3;
            }
            else if (stairLevel == 3 && !game.player.grounded && game.player.x >= 750.f)
                input.direction = 0;

            if (game.player.x >= 794.f)
                mode = 9;
        }
        else if (mode == 9)
        {
            input.direction = -1;
            const float feet = game.player.feet();
            if (returnStairState == 0 && game.player.grounded)
            {
                input.jumpPressed = true;
                returnStairState = 1;
            }
            else if (returnStairState == 1 && game.player.grounded && feet < 335.f)
                returnStairState = 2;
            else if (returnStairState == 2 && game.player.grounded && game.player.x <= 772.f)
            {
                input.jumpPressed = true;
                returnStairState = 3;
            }
            else if (returnStairState == 3 && game.player.grounded && feet < 300.f)
                returnStairState = 4;
            else if (returnStairState == 4 && game.player.x <= 709.f)
            {
                input.direction = 0;
                returnStairState = 5;
            }
            else if (returnStairState == 5)
            {
                input.direction = 0;
                if (game.player.grounded && feet > 325.f)
                    returnStairState = 6;
            }
            else if (returnStairState == 6 && game.player.x <= 687.f)
            {
                input.direction = 0;
                returnStairState = 7;
            }
            else if (returnStairState == 7)
            {
                input.direction = 0;
                if (game.player.grounded && feet > 360.f)
                {
                    input.direction = -1;
                    input.jumpPressed = true;
                    trap5Jump = true;
                    returnStairState = 8;
                }
            }
            if (trap5Jump && !trap6LeftJump && game.player.x <= 300.f
                && game.player.grounded)
            {
                input.jumpPressed = true;
                trap6LeftJump = true;
            }
            if (trap6LeftJump && game.player.x < 220.f && game.player.grounded)
                mode = 10;
        }
        else
        {
            input.direction = 1;
            if (!trap6RightJump && game.player.grounded)
            {
                input.jumpPressed = true;
                trap6RightJump = true;
            }
            else if (trap6RightJump && !realDoorJump && game.player.x >= 430.f
                && game.player.grounded)
            {
                input.jumpPressed = true;
                realDoorJump = true;
            }
        }

        game.update(input);
    }

    CHECK(firstJump);
    CHECK(secondJump);
    CHECK(thirdJump);
    CHECK(stairLevel == 3);
    CHECK(trap5Jump);
    CHECK(trap6LeftJump);
    CHECK(trap6RightJump);
    CHECK(realDoorJump);
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 0);
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

void stage6TrapSequence()
{
    devil::Stage6 stage;
    devil::PlayerState player;
    player.x = devil::Stage6Layout::spawnX;
    player.y = devil::Stage6Layout::startFloor - devil::GameConfig::playerHeight;
    player.grounded = true;

    CHECK(!stage.chaseStarted());
    CHECK(!stage.chaseGone());
    CHECK(!stage.bridgeTriggered());
    CHECK(!stage.firstSawReady());
    CHECK(!stage.secondSawReady());
    CHECK(!stage.thirdSawReady());
    for (std::size_t index = 0; index < devil::Stage6Layout::bridgeTileCount; ++index)
        CHECK(near(stage.bridgeDropProgress(index), 0.f));

    player.x = devil::Stage6Layout::chaseTriggerX - devil::GameConfig::playerWidth;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.chaseStarted());
    CHECK(stage.chaseX() > devil::GameConfig::left - devil::Stage6Layout::chaseRadius - 2.f);
    CHECK(stage.touchesHazard({stage.chaseX() - 4.f, stage.chaseY() - 4.f, 8.f, 8.f}));

    player.x = devil::Stage6Layout::startLedgeEnd - devil::GameConfig::playerWidth;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.bridgeTriggered());
    for (int i = 0; i < 60; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(stage.firstSawReady());
    CHECK(!stage.secondSawReady());
    CHECK(!stage.thirdSawReady());

    for (int i = 0; i < 25; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(stage.bridgeDropProgress(0) > 0.f);
    CHECK(stage.bridgeDropProgress(0) >
        stage.bridgeDropProgress(devil::Stage6Layout::bridgeTileCount - 1));

    for (int i = 0; i < 360 && !stage.chaseGone(); ++i)
        stage.update(player, devil::GameConfig::step);
    CHECK(stage.chaseGone());
    CHECK(stage.secondSawReady());
    CHECK(stage.thirdSawReady());
    for (std::size_t index = 0; index < devil::Stage6Layout::bridgeTileCount; ++index)
        CHECK(near(stage.bridgeDropProgress(index), 1.f));

    const devil::RectF firstTop = stage.sawTop(0);
    CHECK(!stage.touchesHazard({firstTop.x + 8.f,
        firstTop.y - devil::GameConfig::playerHeight,
        devil::GameConfig::playerWidth, devil::GameConfig::playerHeight}));
    CHECK(stage.touchesHazard({devil::Stage6Layout::firstSawX - 4.f,
        devil::Stage6Layout::firstSawY - 4.f, 8.f, 8.f}));

    player.x = devil::Stage6Layout::doorSawTriggerX - devil::GameConfig::playerWidth;
    player.y = devil::Stage6Layout::rightLedgeFloor - devil::GameConfig::playerHeight;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.doorSawTriggered());
    CHECK(near(stage.doorSawProgress(), 0.f));
    for (int i = 0; i < 24; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(near(stage.doorSawProgress(), 1.f));
    CHECK(stage.touchesHazard({devil::Stage6Layout::doorSawX - 4.f,
        stage.doorSawY() - 4.f, 8.f, 8.f}));
    for (int i = 0; i < 60; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(near(stage.doorSawProgress(), 0.f));

    player.x = devil::Stage6Layout::doorX;
    player.y = devil::Stage6Layout::rightLedgeFloor - devil::GameConfig::playerHeight;
    CHECK(stage.isAtGoal(player));

    stage.reset();
    CHECK(!stage.chaseStarted());
    CHECK(!stage.chaseGone());
    CHECK(!stage.bridgeTriggered());
    CHECK(!stage.firstSawReady());
    CHECK(!stage.secondSawReady());
    CHECK(!stage.thirdSawReady());
}

void stage6DynamicSolidsAndDeath()
{
    devil::Stage6 stage;
    devil::Game game(stage);

    devil::PlayerState trigger = game.player;
    trigger.x = 210.f;
    for (int i = 0; i < 36; ++i) stage.update(trigger, devil::GameConfig::step);
    game.player.x = stage.chaseX() - 8.f;
    game.player.y = devil::Stage6Layout::startFloor - devil::GameConfig::playerHeight;
    game.update({});
    CHECK(game.phase == devil::Phase::Dying);
    CHECK(game.deaths == 1);

    game.retry();
    CHECK(game.phase == devil::Phase::Playing);
    CHECK(!stage.chaseStarted());
    CHECK(!stage.bridgeTriggered());

    trigger = game.player;
    trigger.x = devil::Stage6Layout::startLedgeEnd - devil::GameConfig::playerWidth;
    stage.update(trigger, devil::GameConfig::step);
    for (int i = 0; i < 160; ++i) stage.update(trigger, devil::GameConfig::step);

    for (std::size_t index = 0; index < devil::Stage6Layout::bridgeTileCount; ++index)
    {
        const float center = devil::Stage6Layout::bridgeX +
            (static_cast<float>(index) + .5f) * devil::Stage6Layout::bridgeTileWidth;
        const bool covered = std::any_of(stage.solids().begin(), stage.solids().end(),
            [center](const devil::RectF& solid)
            {
                return solid.x < center && solid.right() > center &&
                    solid.y <= devil::Stage6Layout::lowerFloor &&
                    solid.bottom() > devil::Stage6Layout::lowerFloor;
            });
        CHECK(!covered);
    }

    trigger.velocityY = -devil::GameConfig::jumpSpeed;
    stage.update(trigger, devil::GameConfig::step);
    const bool ascendingBlockedBySawTop = std::any_of(stage.solids().begin(),
        stage.solids().end(), [&stage](const devil::RectF& solid)
        {
            for (std::size_t index = 0; index < devil::Stage6Layout::platformSawCount; ++index)
                if (near(solid.y, stage.sawTop(index).y)) return true;
            return false;
        });
    CHECK(!ascendingBlockedBySawTop);
    trigger.velocityY = 0.f;
    stage.update(trigger, devil::GameConfig::step);

    for (std::size_t index = 0; index < devil::Stage6Layout::platformSawCount; ++index)
    {
        const devil::RectF top = stage.sawTop(index);
        const float center = top.x + top.width * .5f;
        const bool covered = std::any_of(stage.solids().begin(), stage.solids().end(),
            [center, top](const devil::RectF& solid)
            {
                return solid.x < center && solid.right() > center && near(solid.y, top.y);
            });
        CHECK(covered);
    }
}

void stage6PlayableRoute()
{
    devil::Stage6 stage;
    devil::Game game(stage);
    bool jumpedFromBridge = false;
    bool jumpedFromFirstSaw = false;
    bool jumpedFromSecondSaw = false;
    bool jumpedFromThirdSaw = false;
    bool jumpedDoorSaw = false;

    for (int frame = 0; frame < 3600 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{1, false};
        if (!jumpedFromBridge && game.player.grounded &&
            near(game.feet(), devil::Stage6Layout::startFloor, .2f) &&
            game.player.x >= 292.f)
        {
            input.jumpPressed = true;
            jumpedFromBridge = true;
        }
        else if (jumpedFromBridge && !jumpedFromFirstSaw && game.player.grounded &&
            near(game.feet(), stage.sawTop(0).y, .2f))
        {
            if (game.player.x < 420.f)
                input.direction = 1;
            else if (!stage.secondSawReady())
                input.direction = 0;
            else
            {
                input.jumpPressed = true;
                jumpedFromFirstSaw = true;
            }
        }
        else if (jumpedFromFirstSaw && !jumpedFromSecondSaw && game.player.grounded &&
            near(game.feet(), stage.sawTop(1).y, .2f))
        {
            if (game.player.x < 536.f)
                input.direction = 1;
            else if (!stage.thirdSawReady())
                input.direction = 0;
            else
            {
                input.jumpPressed = true;
                jumpedFromSecondSaw = true;
            }
        }
        else if (jumpedFromSecondSaw && !jumpedFromThirdSaw && game.player.grounded &&
            near(game.feet(), stage.sawTop(2).y, .2f))
        {
            input.jumpPressed = true;
            jumpedFromThirdSaw = true;
        }
        else if (jumpedFromThirdSaw && !jumpedDoorSaw && game.player.grounded &&
            near(game.feet(), devil::Stage6Layout::rightLedgeFloor, .2f) &&
            game.player.x >= 708.f)
        {
            input.jumpPressed = true;
            jumpedDoorSaw = true;
        }
        game.update(input);
    }

    CHECK(jumpedFromBridge);
    CHECK(jumpedFromFirstSaw);
    CHECK(jumpedFromSecondSaw);
    CHECK(jumpedFromThirdSaw);
    CHECK(jumpedDoorSaw);
    CHECK(stage.bridgeTriggered());
    CHECK(stage.firstSawReady());
    CHECK(stage.secondSawReady());
    CHECK(stage.thirdSawReady());
    CHECK(stage.doorSawTriggered());
    CHECK(stage.chaseGone());
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 0);
    CHECK(advanceUntil(game, {}, [](const devil::Game& current)
        { return current.phase == devil::Phase::Cleared; }, 120));
}

void finalStageTrapSequence()
{
    devil::FinalStage stage;
    devil::PlayerState player;
    player.x = devil::FinalStageLayout::spawnX;
    player.y = devil::FinalStageLayout::upperFloor - devil::GameConfig::playerHeight;
    player.grounded = true;

    CHECK(!stage.chaserActive());
    CHECK(!stage.upperGapOpen());
    CHECK(!stage.pushWallTriggered());
    CHECK(!stage.upperSpikeTriggered());
    CHECK(!stage.descentStarted());
    CHECK(!stage.floorSawTriggered());
    CHECK(!stage.lowerGapOpen());
    CHECK(!stage.dartActive());

    player.x = devil::FinalStageLayout::chaserTriggerX;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.chaserActive());

    player.x = devil::FinalStageLayout::upperGapTriggerX -
        devil::GameConfig::playerWidth * .5f;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.pushWallTriggered());
    CHECK(near(stage.pushWallProgress(), 0.f));
    for (int i = 0; i < 24; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(stage.upperGapOpen());
    CHECK(stage.upperGapProgress() > 0.f);
    CHECK(stage.pushWallTriggered());
    CHECK(near(stage.pushWallProgress(), 1.f));
    CHECK(near(stage.pushWallBounds().height, devil::FinalStageLayout::pushWallHeight));
    CHECK(!stage.upperSpikeTriggered());

    player.x = devil::FinalStageLayout::upperSpikeTriggerX -
        devil::GameConfig::playerWidth * .5f;
    stage.update(player, devil::GameConfig::step);
    CHECK(near(stage.upperSpikeProgress(), 0.f));
    for (int i = 0; i < 24; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(stage.upperSpikeTriggered());
    CHECK(near(stage.upperSpikeProgress(), 1.f));

    player.x = devil::FinalStageLayout::shaftLeft;
    player.y = devil::FinalStageLayout::upperFloor;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.descentStarted());
    for (std::size_t index = 0; index < devil::FinalStageLayout::wallSawCount; ++index)
        CHECK(stage.touchesHazard(stage.wallSawBounds(index)));

    player.y = devil::FinalStageLayout::lowerFloor - devil::GameConfig::playerHeight;
    player.x = devil::FinalStageLayout::floorSawTriggerX -
        devil::GameConfig::playerWidth * .5f;
    stage.update(player, devil::GameConfig::step);
    CHECK(near(stage.floorSawProgress(), 0.f));
    for (int i = 0; i < 36; ++i) stage.update(player, devil::GameConfig::step);
    CHECK(stage.floorSawTriggered());
    CHECK(near(stage.floorSawProgress(), 1.f));
    CHECK(near(stage.floorSawVisibleHeight(), devil::FinalStageLayout::floorSawExposedHeight));
    CHECK(stage.floorSawVisibleHeight() < devil::FinalStageLayout::floorSawRadius * 2.f);
    CHECK(stage.touchesHazard({devil::FinalStageLayout::floorSawX - 4.f,
        stage.floorSawY() - 4.f, 8.f, 8.f}));

    player.x = devil::FinalStageLayout::lowerGapTriggerX -
        devil::GameConfig::playerWidth * .5f;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.lowerGapOpen());

    player.x = devil::FinalStageLayout::dartTriggerX -
        devil::GameConfig::playerWidth * .5f;
    stage.update(player, devil::GameConfig::step);
    CHECK(stage.dartActive());
    CHECK(stage.touchesHazard({stage.dartX(), devil::FinalStageLayout::dartY,
        devil::FinalStageLayout::dartWidth, devil::FinalStageLayout::dartHeight}));

    player.x = devil::FinalStageLayout::doorX;
    CHECK(stage.isAtGoal(player));

    stage.reset();
    CHECK(!stage.chaserActive());
    CHECK(!stage.upperGapOpen());
    CHECK(!stage.pushWallTriggered());
    CHECK(!stage.upperSpikeTriggered());
    CHECK(!stage.descentStarted());
    CHECK(!stage.floorSawTriggered());
    CHECK(!stage.lowerGapOpen());
    CHECK(!stage.dartActive());
}

void finalStagePushWallPunishesRush()
{
    devil::FinalStage stage;
    devil::Game game(stage);
    bool jumped = false;
    bool touchedWallFace = false;

    for (int frame = 0; frame < 360 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{1, false};
        if (!jumped && game.player.grounded && game.player.x >= 318.f)
        {
            input.jumpPressed = true;
            jumped = true;
        }
        game.update(input);
        if (stage.pushWallProgress() > .8f &&
            near(game.player.x,
                devil::FinalStageLayout::pushWallX - devil::GameConfig::playerWidth, .2f))
            touchedWallFace = true;
    }

    CHECK(jumped);
    CHECK(stage.pushWallTriggered());
    CHECK(touchedWallFace);
    CHECK(game.phase == devil::Phase::Dying);
    CHECK(game.deaths == 1);
}

bool clearsFinalFloorSawAt(float jumpX)
{
    devil::FinalStage stage;
    devil::Game game(stage);

    devil::PlayerState descentMarker = game.player;
    descentMarker.x = devil::FinalStageLayout::shaftLeft;
    descentMarker.y = devil::FinalStageLayout::upperFloor;
    stage.update(descentMarker, devil::GameConfig::step);

    game.player.x = 720.f;
    game.player.y = devil::FinalStageLayout::lowerFloor - devil::GameConfig::playerHeight;
    game.player.velocityY = 0.f;
    game.player.grounded = true;
    bool jumped = false;
    for (int frame = 0; frame < 360 && game.phase == devil::Phase::Playing &&
        game.player.x > 570.f; ++frame)
    {
        devil::Input input{-1, false};
        if (!jumped && game.player.grounded && game.player.x <= jumpX)
        {
            input.jumpPressed = true;
            jumped = true;
        }
        game.update(input);
    }
    return jumped && game.phase == devil::Phase::Playing && game.player.x <= 570.f;
}

void finalStageFloorSawJumpWindow()
{
    CHECK(clearsFinalFloorSawAt(636.f));
    CHECK(clearsFinalFloorSawAt(648.f));
    CHECK(clearsFinalFloorSawAt(660.f));
}

void finalStageContinuousPlayableRoute()
{
    devil::FinalStage stage;
    devil::Game game(stage);
    bool jumpedUpperGap = false;
    bool jumpedUpperSpikes = false;
    bool jumpedFloorSaw = false;
    bool jumpedLowerGap = false;
    bool jumpedDart = false;

    for (int frame = 0; frame < 4800 && game.phase == devil::Phase::Playing; ++frame)
    {
        devil::Input input{1, false};
        if (!stage.descentStarted())
        {
            if (!jumpedUpperGap && stage.upperGapOpen() && !stage.pushWallGone())
            {
                // 첫 벽이 완전히 내려갈 때까지 구덩이 왼쪽 안전 지대에서 기다립니다.
                input.direction = 0;
            }
            else if (!jumpedUpperGap && stage.pushWallGone() &&
                game.player.grounded && game.player.x >= 318.f)
            {
                input.jumpPressed = true;
                jumpedUpperGap = true;
            }
            else if (jumpedUpperGap && !jumpedUpperSpikes &&
                game.player.grounded && game.player.x >= 480.f)
            {
                input.jumpPressed = true;
                jumpedUpperSpikes = true;
            }
        }
        else if (!game.player.grounded ||
            game.player.feet() < devil::FinalStageLayout::lowerFloor - .2f)
        {
            if (game.player.feet() < 235.f)
                input.direction = -1;
            else if (game.player.feet() < 365.f)
                input.direction = 1;
            else
                input.direction = -1;
        }
        else
        {
            input.direction = -1;
            if (!jumpedFloorSaw && game.player.x <= 650.f)
            {
                input.jumpPressed = true;
                jumpedFloorSaw = true;
            }
            else if (jumpedFloorSaw && !jumpedLowerGap && game.player.x <= 492.f)
            {
                input.jumpPressed = true;
                jumpedLowerGap = true;
            }
            else if (jumpedLowerGap && !jumpedDart && game.player.x <= 304.f)
            {
                input.jumpPressed = true;
                jumpedDart = true;
            }
        }
        game.update(input);
    }

    CHECK(jumpedUpperGap);
    CHECK(jumpedUpperSpikes);
    CHECK(jumpedFloorSaw);
    CHECK(jumpedLowerGap);
    CHECK(jumpedDart);
    CHECK(stage.chaserActive());
    CHECK(stage.upperGapOpen());
    CHECK(stage.upperSpikeTriggered());
    CHECK(stage.descentStarted());
    CHECK(stage.floorSawTriggered());
    CHECK(stage.lowerGapOpen());
    CHECK(game.phase == devil::Phase::EnteringDoor);
    CHECK(game.deaths == 0);
    CHECK(advanceUntil(game, {}, [](const devil::Game& current)
        { return current.phase == devil::Phase::Cleared; }, 120));
}
}

int main()
{
    commonStageRegistration();
    stageThreeFourAssignment();
    mergedStageReachabilityMargins();
    mergedStageMechanics();
    stage2PlayableRoute();
    stage3PlayableRoute();
    stage4PlayableRoute();
    stage5PlayableRoute();
    sequentialStageTransition();
    idleAndMovement();
    jumpAndNoDoubleJump();
    manualRetryDuringFall();
    ceilingCollision();
    progressiveFloorOpening();
    pitSideCollisions();
    deathRetryAndClear();
    stage6TrapSequence();
    stage6DynamicSolidsAndDeath();
    stage6PlayableRoute();
    finalStageTrapSequence();
    finalStagePushWallPunishesRush();
    finalStageFloorSawJumpWindow();
    finalStageContinuousPlayableRoute();
    if (failures != 0)
    {
        std::cerr << "FAIL: " << failures << " of " << checks << " checks failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS: " << checks
        << " checks across common physics, Stage1-6, FinalStage, retry, and clear flow.\n";
    return EXIT_SUCCESS;
}
