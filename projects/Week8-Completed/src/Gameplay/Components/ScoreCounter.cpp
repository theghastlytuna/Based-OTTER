#include "Gameplay/Components/ScoreCounter.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"

ScoreCounter::ScoreCounter() :
    IComponent(),
    _renderer(nullptr)
{ }

ScoreCounter::~ScoreCounter() = default;

void ScoreCounter::AddScore(int change)
{
    score += change;

    if (score < 10)
        _renderer->SetMaterial(numberArray[score]);
    else
        _renderer->SetMaterial(numberArray[9]);
}

void ScoreCounter::Awake()
{
    _renderer = GetComponent<RenderComponent>();
    score = 0;
}

void ScoreCounter::RenderImGui()
{
}

nlohmann::json ScoreCounter::ToJson() const
{
    return nlohmann::json();
}

ScoreCounter::Sptr ScoreCounter::FromJson(const nlohmann::json& blob)
{
    ScoreCounter::Sptr result = std::make_shared<ScoreCounter>();
    return result;
}
