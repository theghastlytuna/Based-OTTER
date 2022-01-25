#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/RenderComponent.h"

class ScoreCounter : public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<ScoreCounter> Sptr;

	ScoreCounter();
	virtual ~ScoreCounter();

	void AddScore(int change);

	virtual void Awake() override;
	virtual void RenderImGui() override;
	MAKE_TYPENAME(ScoreCounter);
	virtual nlohmann::json ToJson() const override;
	static ScoreCounter::Sptr FromJson(const nlohmann::json& blob);

	Gameplay::Material::Sptr numberArray[10];

private:
	RenderComponent::Sptr _renderer;
	int score = 0;
};