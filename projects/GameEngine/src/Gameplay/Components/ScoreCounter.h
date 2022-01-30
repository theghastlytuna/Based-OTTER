#pragma once
#include "IComponent.h"

class ScoreCounter : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<ScoreCounter> Sptr;

	ScoreCounter();
	virtual ~ScoreCounter();
	virtual void Awake() override;
	virtual void Update(float deltaTime) override;
	virtual void RenderImGui() override;
	MAKE_TYPENAME(ScoreCounter);
	virtual nlohmann::json ToJson() const override;
	static ScoreCounter::Sptr FromJson(const nlohmann::json& blob);

	void AddScore();
	int GetScore();
	void ResetScore();

	bool ReachedMaxScore();

private:

	int score;
	int maxScore;
	bool winner;
};

