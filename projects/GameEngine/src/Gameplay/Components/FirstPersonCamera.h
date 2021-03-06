#pragma once
#include "IComponent.h"
#include "Gameplay/Components/ControllerInput.h"

struct GLFWwindow;

/// <summary>
/// A simple behaviour that allows movement of a gameobject with WASD, mouse,
/// and ctrl + space
/// </summary>
class FirstPersonCamera : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<FirstPersonCamera> Sptr;

	FirstPersonCamera();
	virtual ~FirstPersonCamera();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(FirstPersonCamera);
	virtual nlohmann::json ToJson() const override;
	static FirstPersonCamera::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _shiftMultipler;
	glm::vec2 _mouseSensitivity;
	glm::vec3 _moveSpeeds;
	glm::dvec2 _prevMousePos;
	glm::vec2 _currentRot;

	bool _isMousePressed = false;
	GLFWwindow* _window;

	ControllerInput::Sptr _controller;
	glm::vec2 _controllerSensitivity;

	glm::vec2 _keyboardSensitivity;
};