#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Physics/TriggerVolume.h"

class PickUpBehaviour : public Gameplay::IComponent
{
public:

	typedef std::shared_ptr<PickUpBehaviour> Sptr;
	PickUpBehaviour();
	virtual ~PickUpBehaviour();

	// Inherited from IComponent

	virtual void Update(float deltaTime) override;
	virtual void Awake() override;
	virtual void OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) override;
	virtual void OnTriggerVolumeLeaving(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static PickUpBehaviour::Sptr FromJson(const nlohmann::json& blob);
	MAKE_TYPENAME(PickUpBehaviour);

	Gameplay::Material::Sptr DefaultMaterial;
	Gameplay::Material::Sptr DepletedMaterial;

protected:
	int pickUpType = 0;
	float cooldownTimer = 0;
	RenderComponent::Sptr _renderer;
};