#include "ScoreCounter.h"

#include "Application/Application.h"

ScoreCounter::ScoreCounter() :
	IComponent(),
	score(0),
	maxScore(1),
	winner(false)
{}

ScoreCounter::~ScoreCounter() = default;

void ScoreCounter::Awake()
{
}

void ScoreCounter::Update(float deltaTime)
{
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
	return ScoreCounter::Sptr();
}

void ScoreCounter::AddScore()
{
	score++;
	if (score >= maxScore) winner = true;

	Application& app = Application::Get();


}

int ScoreCounter::GetScore()
{
	return score;
}

bool ScoreCounter::ReachedMaxScore()
{
	return winner;
}


void ScoreCounter::ResetScore()
{
	score = 0;
	winner = false;
}