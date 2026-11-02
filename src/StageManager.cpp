#include "StageManager.hpp"

#include "IStage.hpp"
#include "FinalStage.hpp"
#include "Stage1.hpp"
#include "Stage2.hpp"
#include "Stage3.hpp"
#include "Stage4.hpp"
#include "Stage5.hpp"
#include "Stage6.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace devil
{
StageManager::StageManager()
{
    // 새 스테이지를 합칠 때 공통 담당자가 이 목록에 StageX를 순서대로 등록합니다.
    stages_.push_back(std::make_unique<Stage1>());
    stages_.push_back(std::make_unique<Stage2>());
    stages_.push_back(std::make_unique<Stage3>());
    stages_.push_back(std::make_unique<Stage4>());
    stages_.push_back(std::make_unique<Stage5>());
    stages_.push_back(std::make_unique<Stage6>());
    stages_.push_back(std::make_unique<FinalStage>());
    game_ = std::make_unique<Game>(*stages_.front());
}

StageManager::StageManager(std::vector<std::unique_ptr<IStage>> stages)
    : stages_(std::move(stages))
{
    if (stages_.empty()) throw std::invalid_argument("StageManager requires at least one stage.");
    game_ = std::make_unique<Game>(*stages_.front());
}

StageManager::~StageManager() = default;

IStage& StageManager::currentStage() { return *stages_[currentIndex_]; }
const IStage& StageManager::currentStage() const { return *stages_[currentIndex_]; }

void StageManager::loadStage(std::size_t index, bool preserveRunStats)
{
    currentIndex_ = index;
    if (game_) game_->startStage(currentStage(), preserveRunStats);
}

void StageManager::update(Input input, float dt)
{
    game_->update(input, dt);

    if (auto* stage5 = dynamic_cast<Stage5*>(&currentStage());
        stage5 && stage5->restartRequested())
    {
        newRun();
        return;
    }

    if (game_->phase == Phase::Cleared && !isLastStage() && game_->phaseTime >= .65f)
        loadStage(currentIndex_ + 1, true);
}

void StageManager::retry()
{
    game_->retry();
}

void StageManager::newRun()
{
    currentIndex_ = 0;
    game_->startStage(currentStage(), false);
}
}
