#include "PickUpBehaviour.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Components/HealthManager.h"

PickUpBehaviour::PickUpBehaviour() :
	IComponent()
{ }
PickUpBehaviour::~PickUpBehaviour() = default;

void PickUpBehaviour::Update(float deltaTime)
{
	if (cooldownTimer > 0)
	{
		cooldownTimer -= deltaTime;
	}
	else if (_renderer->GetMaterial() != DefaultMaterial)
	{
		_renderer->SetMaterial(DefaultMaterial);
	}
}

void PickUpBehaviour::Awake()
{
	_renderer = GetComponent<RenderComponent>();
}

void PickUpBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	LOG_INFO("Body has entered {} trigger volume: {}", GetGameObject()->Name, body->GetGameObject()->Name);
	if (body->GetGameObject()->Name == "Player 1" || body->GetGameObject()->Name == "Player 2")
	{
		if (cooldownTimer <= 0)
		{
			switch (pickUpType)
			{
			case 0 : //health pack
				LOG_INFO("Player {} Health PickUp applied: {}", GetGameObject()->Name, body->GetGameObject()->Name);
				cooldownTimer = 60.f;
				_renderer->SetMaterial(DepletedMaterial);
				body->GetGameObject()->Get<HealthManager>()->ResetHealth();
				break;

			default:
				LOG_INFO("Player {} This PickUp is broken: {}", GetGameObject()->Name, body->GetGameObject()->Name);
				break;
			}
		}
	}
}

void PickUpBehaviour::OnTriggerVolumeLeaving(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
	LOG_INFO("Body has left {} trigger volume: {}", GetGameObject()->Name, body->GetGameObject()->Name);
}

void PickUpBehaviour::RenderImGui() { }

nlohmann::json PickUpBehaviour::ToJson() const {
	return { };
}

PickUpBehaviour::Sptr PickUpBehaviour::FromJson(const nlohmann::json& blob) {
	PickUpBehaviour::Sptr result = std::make_shared<PickUpBehaviour>();
	return result;
}
