#include "Gameplay/Components/PlayerControl.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/Camera.h"

#include "Gameplay/Physics/RigidBody.h"
#include "Application/Application.h"

#include "Application/SoundManaging.h"

PlayerControl::PlayerControl()
	: IComponent(),
	_mouseSensitivity({ 0.2f, 0.2f }),
	_moveSpeeds(glm::vec3(6.0f)),
	_shiftMultipler(2.0f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false),
	_isMoving(false),
	_isSprinting(false),
	_spintVal(2.0f),
	_controllerSensitivity({ 1.5f, 1.5f })
{ }

PlayerControl::~PlayerControl() = default;

void PlayerControl::Awake()
{
	_window = Application::Get().GetWindow();

	_controller = GetComponent<ControllerInput>();

	if (_controller == nullptr)
	{
		IsEnabled = false;
	}
	
	if (GetGameObject()->Name == "Player 1") {
		playerID = 1;
	}
	else {
		playerID = 2;
	}

	_boomerang = GetGameObject()->GetScene()->FindObjectByName("Boomerang " + std::to_string(playerID));
	if (_boomerang->Has<BoomerangBehavior>()) {
		_boomerangBehavior = _boomerang->Get<BoomerangBehavior>();
	}
	else {
		std::cout << "Ayo the pizza here" << std::endl;
	}

	if (playerID == 1) {
		_camera = GetGameObject()->GetScene()->PlayerCamera;
		_enemy = GetGameObject()->GetScene()->FindObjectByName("Player 2");
	}
	else {
		_camera = GetGameObject()->GetScene()->PlayerCamera2;
		_enemy = GetGameObject()->GetScene()->FindObjectByName("Player 1");
	}

	//IMPORTANT: This only works because the detachedCam is the only child of the player. If anything to do with children or the detached cam changes, this might break
	_initialFov = GetGameObject()->GetChildren()[0]->Get<Gameplay::Camera>()->GetFovDegrees();
}

void PlayerControl::Update(float deltaTime)
{
	//If there is a valid controller connected, then use it to find input
	if (_controller->IsValid())
	{
		_controllerSensitivity = { _controller->GetSensitivity(), _controller->GetSensitivity() };

		glm::vec3 playerToEnemy = glm::normalize(GetGameObject()->GetPosition() - _enemy->GetPosition());

		glm::vec3 cameraLocalForward = glm::normalize(glm::vec3(_camera->GetView()[0][2], _camera->GetView()[1][2], _camera->GetView()[2][2]));

		//Calculate the angle between the vectors
		float dot = cameraLocalForward.x * playerToEnemy.x + cameraLocalForward.y * playerToEnemy.y + cameraLocalForward.z * playerToEnemy.z;
		float divisor = glm::length(playerToEnemy) * glm::length(cameraLocalForward);
		float angle = glm::acos(dot / divisor);

		/*float distance = glm::length(GetGameObject()->GetPosition() - _enemy->GetPosition());
		std::cout << "Angle: " << angle << " Distance: " << distance << std::endl;*/



		if (angle < 0.1) {
			_controllerSensitivity *= 0.2f;
		}
		else if (angle < 0.3) {
			_controllerSensitivity *= 0.5f;
		}
		else if (angle < 0.5) {
			_controllerSensitivity *= 0.7f;
		}

		_isMoving = false;
		_justThrew = false;

		bool Wang = _controller->GetButtonDown(GLFW_GAMEPAD_BUTTON_X);
		bool Point = _controller->GetButtonDown(GLFW_GAMEPAD_BUTTON_Y);
		bool Target = _controller->GetButtonDown(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
		bool returnaloni = _controller->GetButtonDown(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);

		float leftX = _controller->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X);
		float leftY = _controller->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y);

		float rightX = _controller->GetAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_X);
		float rightY = _controller->GetAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_Y);

		float leftTrigger = _controller->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER);
		float rightTrigger = _controller->GetAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);

		//_isSprinting = _controller->GetButtonDown(GLFW_GAMEPAD_BUTTON_B);

		_isSprinting = (leftTrigger >= 1.0f);

		if (!_controller->GetEnabled())
		{
			Wang = false;
			Point = false;
			Target = false;
			returnaloni = false;
			leftX = 0.0f;
			leftY = 0.0f;
			rightX = 0.0f;
			rightY = 0.0f;
			leftTrigger = 0.0f;
			rightTrigger = 0.0f;
		}

		//Since controller joysticks are physical, they often won't be perfect, meaning at a neutral state it might still have slight movement.
		//Check to make sure that the axes aren't outputting an extremely small number, and if they are then set the input to 0.
		if (rightX > 0.2 || rightX < -0.2) _currentRot.x += static_cast<float>(rightX) * _controllerSensitivity.x;	
		if (rightY > 0.2 || rightY < -0.2) _currentRot.y += static_cast<float>(rightY) * _controllerSensitivity.y;

		glm::quat rotX = glm::angleAxis(glm::radians(-_currentRot.x), glm::vec3(0, 0, 1));
		glm::quat rotY = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));

		glm::quat currentRot = rotX * rotY;

		GetGameObject()->SetRotation(currentRot);

		glm::vec3 input = glm::vec3(0.0f);

		//Since controller joysticks are physical, they often won't be perfect, meaning at a neutral state it might still have slight movement.
		//Check to make sure that the axes aren't outputting an extremely small number, and if they are then set the input to 0.
		if (leftY < 0.2 && leftY > -0.2) input.z = 0.0f;
		else
		{
			_isMoving = true;
			input.z = leftY * -_moveSpeeds.x;
		}

		if (leftX < 0.2 && leftX > -0.2) input.x = 0.0f;
		else
		{
			_isMoving = true;
			input.x = leftX * -_moveSpeeds.y;
		}

		if (input.x == 0.0f && input.z == 0.0f)
		{
			_isMoving = false;
			soundTime = 0.0f;
		}

		input *= deltaTime;

		glm::vec3 worldMovement = glm::vec3((currentRot * glm::vec4(input, 1.0f)).x, (currentRot * glm::vec4(input, 1.0f)).y, 0.0f);

		if (worldMovement != glm::vec3(0.0f))
		{
			worldMovement = 10.0f * glm::normalize(worldMovement);

			_timeBetStep += deltaTime;

			if (_isSprinting)
			{
				worldMovement *= _spintVal;
				if (_timeBetStep >= 0.4)
				{
					SoundManaging::Current().PlayEvent("footsteps", GetGameObject());
					_timeBetStep = 0.0f;
				}
			}

			else
			{
				if (_timeBetStep >= 0.8f)
				{
					SoundManaging::Current().PlayEvent("footsteps", GetGameObject());
					_timeBetStep = 0.0f;
				}
			}
		}

		else _timeBetStep = 1.5f;
		GetGameObject()->Get<Gameplay::Physics::RigidBody>()->ApplyForce(worldMovement);




		//Wang Throwing
		//trigger input from controller goes from -1 to 1
		//also makes sure that the controller is enabled since pausing sets trigger input to 0
		if (rightTrigger > -1 && GetGameObject()->Get<ControllerInput>()->GetEnabled()) { //If Trigger Pulled Down
			if (_boomerangBehavior->getReadyToThrow()) { //If it's unthrown
				if (_chargeAmount < 3.f) //Charge it up
				{
				//if the player can throw the boomerang, increase charge level as long as button is held and below charge cap (3.f)
				_chargeAmount += 0.02;
				//IMPORTANT: This only works because the detachedCam is the only child of the player. If anything to do with children or the detached cam changes, this might break
				GetGameObject()->GetChildren()[0]->Get<Gameplay::Camera>()->SetFovDegrees(_initialFov - (_chargeAmount * 10));
				}
			}
			else //tracking to raycasted point
			{
				_boomerangBehavior->returnBoomerang();
			}
		}
		else if (rightTrigger == -1) //Release Trigger
		{
			_boomerangBehavior->_triggerInput = -1;
			if (_chargeAmount > 0.5)
			{
				//if the player is not holding the button, but has charged their throw above the minimum, throw the boomerang
				_boomerangBehavior->throwWang(GetGameObject()->GetPosition(), _chargeAmount);
				_justThrew = true;
				_chargeAmount = 0;
				
			}
			else
			{
				//else, just reset the charge level (so players can't "prime" a throw)
				_chargeAmount = 0;
			}
			GetGameObject()->GetChildren()[0]->Get<Gameplay::Camera>()->SetFovDegrees(_initialFov);
		}

		if (returnaloni) {
			_boomerangBehavior->makeBoomerangInactive();
		}
	}

	//Else, use KBM
	else
	{
		if (glfwGetMouseButton(_window, 0)) {
			if (_isMousePressed == false) {
				glfwGetCursorPos(_window, &_prevMousePos.x, &_prevMousePos.y);
			}
			_isMousePressed = true;
		}
		else {
			_isMousePressed = false;
		}

		if (_isMousePressed) {
			glm::dvec2 currentMousePos;
			glfwGetCursorPos(_window, &currentMousePos.x, &currentMousePos.y);

			_currentRot.x += static_cast<float>(currentMousePos.x - _prevMousePos.x) * _mouseSensitivity.x;
			_currentRot.y += static_cast<float>(currentMousePos.y - _prevMousePos.y) * _mouseSensitivity.y;
			glm::quat rotX = glm::angleAxis(glm::radians(_currentRot.x), glm::vec3(0, 0, 1));
			glm::quat rotY = glm::angleAxis(glm::radians(90.f), glm::vec3(1, 0, 0));
			glm::quat currentRot = rotX * rotY;
			GetGameObject()->SetRotation(currentRot);

			_prevMousePos = currentMousePos;

			glm::vec3 input = glm::vec3(0.0f);
			if (glfwGetKey(_window, GLFW_KEY_W)) {
				input.z -= _moveSpeeds.x;
			}
			if (glfwGetKey(_window, GLFW_KEY_S)) {
				input.z += _moveSpeeds.x;
			}
			if (glfwGetKey(_window, GLFW_KEY_A)) {
				input.x -= _moveSpeeds.y;
			}
			if (glfwGetKey(_window, GLFW_KEY_D)) {
				input.x += _moveSpeeds.y;
			}
			if (glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL)) {
				input.y -= _moveSpeeds.z;
			}
			if (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT)) {
				input *= _shiftMultipler;
			}

			input *= deltaTime;

			glm::vec3 worldMovement = glm::vec3((currentRot * glm::vec4(input, 1.0f)).x, (currentRot * glm::vec4(input, 1.0f)).y, 0.0f);

			if (worldMovement != glm::vec3(0.0f))
			{
				worldMovement = 10.0f * glm::normalize(worldMovement);
			}
			GetGameObject()->Get<Gameplay::Physics::RigidBody>()->ApplyForce(worldMovement);
		}
	}

	if (_isMoving)
	{
		SoundManaging& soundManaging = SoundManaging::Current();
		soundTime += deltaTime;

		if (soundTime >= 1.0f)
		{
			soundManaging.PlaySound("Step");
			soundTime = 0.0f;
		}
	}

	if (GetGameObject()->GetPosition().z < -5) {
		GetGameObject()->SetPosition({ 2,-2,4 });
	}
}

bool PlayerControl::IsMoving()
{
	return _isMoving;
}

bool PlayerControl::IsSprinting()
{
	return _isSprinting;
}

bool PlayerControl::GetJustThrew()
{
	return _justThrew;
}

void PlayerControl::RenderImGui()
{
}

nlohmann::json PlayerControl::ToJson() const
{
	return nlohmann::json();
}

PlayerControl::Sptr PlayerControl::FromJson(const nlohmann::json& blob)
{
	return PlayerControl::Sptr();
}