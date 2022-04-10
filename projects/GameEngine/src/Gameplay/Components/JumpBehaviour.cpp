#include "Gameplay/Components/JumpBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"
#include "Application/SoundManaging.h"

void JumpBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
	if (_body == nullptr) {
		IsEnabled = false;
	}

	_controller = GetComponent<ControllerInput>();

	if (_controller == nullptr)
	{
		IsEnabled = false;
	}
}

void JumpBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Impulse", &_impulse, 1.0f);
}

void JumpBehaviour::OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	LOG_INFO("Entered trigger: {}", trigger->GetGameObject()->Name);
	if (trigger->GetGameObject()->Name.find("Ground") != std::string::npos)
	{
		_onGround = true;
		_numGroundsEntered++;
	}
}

void JumpBehaviour::OnLeavingTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	LOG_INFO("Exited trigger: {}", trigger->GetGameObject()->Name);
	if (trigger->GetGameObject()->Name.find("Ground") != std::string::npos)
	{
		_numGroundsEntered--;
		if (_numGroundsEntered <= 0)
		{
			_numGroundsEntered = 0;
			_onGround = false;
		}
	}
}

nlohmann::json JumpBehaviour::ToJson() const {
	return nlohmann::json();
}

JumpBehaviour::JumpBehaviour() :
	IComponent(),
	_impulse(12.0f)
{ }

JumpBehaviour::~JumpBehaviour() = default;

JumpBehaviour::Sptr JumpBehaviour::FromJson(const nlohmann::json& blob) {
	return JumpBehaviour::Sptr();
}

void JumpBehaviour::Update(float deltaTime) {
	//If the object has a controller connected, use that to find input
	_startingJump = false;
	_jumpCooldown -= deltaTime;

	if (_controller->IsValid())
	{
		if (_controller->GetButtonDown(GLFW_GAMEPAD_BUTTON_A) && _onGround && _jumpCooldown <= 0 && _controller->GetEnabled())
		{
			_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, _impulse));
			_jumpCooldown = 0.5f;
			_startingJump = true;

			SoundManaging::Current().PlayEvent("Jump", GetGameObject());
		}
	}
	
	
	else 
	{
		if (InputEngine::GetKeyState(GLFW_KEY_SPACE) == ButtonState::Pressed && _onGround && _jumpCooldown <= 0 && InputEngine::GetEnabled()) {
			_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, _impulse));

			_jumpCooldown = 0.5f;
			_startingJump = true;

			SoundManaging::Current().PlayEvent("Jump", GetGameObject());
		}
		
		/*
		Gameplay::IComponent::Sptr ptr = Panel.lock();
		if (ptr != nullptr) {
			ptr->IsEnabled = !ptr->IsEnabled;
		}
		*/
	}
}

bool JumpBehaviour::IsInAir()
{
	return !_onGround;
}

bool JumpBehaviour::IsStartingJump()
{
	return _startingJump;
}