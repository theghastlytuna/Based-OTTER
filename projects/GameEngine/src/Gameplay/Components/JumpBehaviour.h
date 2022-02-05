#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/ControllerInput.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class JumpBehaviour : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<JumpBehaviour> Sptr;

	std::weak_ptr<Gameplay::IComponent> Panel;

	JumpBehaviour();
	virtual ~JumpBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	bool IsInAir();
	bool IsStartingJump();

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(JumpBehaviour);
	virtual nlohmann::json ToJson() const override;
	static JumpBehaviour::Sptr FromJson(const nlohmann::json& blob);

	virtual void OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger) override;
	virtual void OnLeavingTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger) override;

protected:
	float _impulse;

	bool _isPressed = false;
	Gameplay::Physics::RigidBody::Sptr _body;

	//Boolean to represent whether the attached object is on a ground object
	bool _onGround = false;

	bool _startingJump = false;

	ControllerInput::Sptr _controller;
};