#include "Gameplay/Components/PuckResetBehaviour.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Components/ScoreCounter.h"

PuckResetBehaviour::PuckResetBehaviour() : 
	IComponent()
{ }
PuckResetBehaviour::~PuckResetBehaviour() = default;

void PuckResetBehaviour::Update(float deltaTime)
{
	if (_body->GetGameObject()->GetPosition().x > 13 || _body->GetGameObject()->GetPosition().x < -13
		|| _body->GetGameObject()->GetPosition().y > 8 || _body->GetGameObject()->GetPosition().y < -8)
	{
		LOG_INFO("Puck Out Of Bounds! Position Reset");
		_body->GetGameObject()->SetPostion(glm::vec3(0.f, 0.f, 0.f));
	}
}

void PuckResetBehaviour::OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	LOG_INFO("Entered trigger: {}", trigger->GetGameObject()->Name);
	if (trigger->GetGameObject()->Name == "Red Goal")
	{
		_body->GetGameObject()->SetPostion(glm::vec3(3.f, 0.f, 0.f));
		_body->SetVelocity(glm::vec3(0.f));
		trigger->GetComponent<ScoreCounter>()->AddScore(1);
	}
	else if (trigger->GetGameObject()->Name == "Blue Goal")
	{
		_body->GetGameObject()->SetPostion(glm::vec3(-3.f, 0.f, 0.f));
		_body->SetVelocity(glm::vec3(0.f));
		trigger->GetComponent<ScoreCounter>()->AddScore(1);
	}
}

void PuckResetBehaviour::OnLeavingTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	LOG_INFO("Left trigger: {}", trigger->GetGameObject()->Name);
}

void PuckResetBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
}

void PuckResetBehaviour::RenderImGui()
{
}

nlohmann::json PuckResetBehaviour::ToJson() const
{
    return nlohmann::json();
}

PuckResetBehaviour::Sptr PuckResetBehaviour::FromJson(const nlohmann::json& blob)
{
	PuckResetBehaviour::Sptr result = std::make_shared<PuckResetBehaviour>();
	return result;
}
