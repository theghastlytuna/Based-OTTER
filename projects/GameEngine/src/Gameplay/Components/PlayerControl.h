#pragma once
#include "IComponent.h"
#include "Gameplay/Components/ControllerInput.h"
#include "BoomerangBehavior.h"

struct GLFWwindow;

/// <summary>
/// A simple behaviour that allows movement of a gameobject with WASD, mouse,
/// and ctrl + space
/// </summary>
class PlayerControl : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<PlayerControl> Sptr;

	PlayerControl();
	virtual ~PlayerControl();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	bool IsMoving();
	bool IsSprinting();
	bool GetJustThrew();
	bool Map2 = false;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(PlayerControl);
	virtual nlohmann::json ToJson() const override;
	static PlayerControl::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _shiftMultipler;
	glm::vec2 _mouseSensitivity;
	glm::vec3 _moveSpeeds;
	glm::dvec2 _prevMousePos;
	glm::vec2 _currentRot;
	bool boomerangThrown = false;
	Gameplay::GameObject::Sptr _boomerang;
	BoomerangBehavior::Sptr _boomerangBehavior;
	Gameplay::Camera::Sptr _camera;

	unsigned int playerID;
	Gameplay::GameObject::Sptr _enemy;


	bool _isMousePressed = false;
	bool _isMoving = false;
	bool _isSprinting = false;
	bool _justThrew = false;
	bool _charging = false;
	float soundTime = 0.0f;

	float _chargeAmount = 0.f;
	float _initialFov;
	float _spintVal = 5.0f;
	float _timeBetStep = 0.0f;
	GLFWwindow* _window;

	ControllerInput::Sptr _controller;
	glm::vec2 _controllerSensitivity;
	glm::vec3 _raycastPosition;

	glm::vec2 _keyboardSensitivity;
};
