#include "Gameplay/Components/ControllerInput.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Application/Application.h"

ControllerInput::ControllerInput()
	: IComponent(),
	_controllerConnected(false),
	_enabled(true),
	_sensitivity(2.0f),
	_minSensitivity(1.0f),
	_maxSensitivity(3.0f)
{ }

ControllerInput::~ControllerInput() = default;

void ControllerInput::Awake()
{
	_window = Application::Get().GetWindow();

	
	auto buttons = glfwGetJoystickButtons(_controllerID, &_buttonCount);

	std::vector<char> temp;

	for (int i = 0; i < _buttonCount; i++)
	{
		//buttonList[i] = buttons[i];

		temp.push_back(buttons[i]);
	}

	buttonList = temp;
	prevButtonList = temp;
}

void ControllerInput::Update(float deltaTime)
{

	_controllerConnected = glfwJoystickPresent(_controllerID);

	auto buttons = glfwGetJoystickButtons(_controllerID, &_buttonCount);

	prevButtonList = buttonList;

	//unsigned char temp[_buttonCount] = {};

	std::vector<char> temp;

	for (int i = 0; i < _buttonCount; i++)
	{
		//buttonList[i] = buttons[i];

		temp.push_back(buttons[i]);
	}

	buttonList = temp;
}

void ControllerInput::RenderImGui()
{
}

nlohmann::json ControllerInput::ToJson() const
{
	return nlohmann::json();
}

ControllerInput::Sptr ControllerInput::FromJson(const nlohmann::json& blob)
{
	return ControllerInput::Sptr();
}

bool ControllerInput::IsValid()
{
	return _controllerConnected;
}

void ControllerInput::SetController(int ID)
{
	_controllerID = ID;
	_controllerConnected = glfwJoystickPresent(ID);
}

bool ControllerInput::GetButtonDown(int ID)
{
	auto buttons = glfwGetJoystickButtons(_controllerID, &_buttonCount);

	return buttons[ID];
}

float ControllerInput::GetAxisValue(int ID)
{
	auto axes = glfwGetJoystickAxes(_controllerID, &_axesCount);

	return axes[ID];
}

void ControllerInput::SetEnabled(bool enabled)
{
	_enabled = enabled;
}

bool ControllerInput::GetEnabled()
{
	return _enabled;
}

bool ControllerInput::GetButtonPressed(int ID)
{
	return (prevButtonList[ID] == false) && (buttonList[ID] == true);
}

void ControllerInput::SetSensitivity(float inSensitivity)
{
	if (inSensitivity >= _maxSensitivity)
	{
		_sensitivity = _maxSensitivity;
	}

	else if (inSensitivity <= _minSensitivity)
	{
		_sensitivity = _minSensitivity;
	}

	else _sensitivity = inSensitivity;
}

float ControllerInput::GetSensitivity()
{
	return _sensitivity;
}

float ControllerInput::GetMinSensitivity()
{
	return _minSensitivity;
}

float ControllerInput::GetMaxSensitivity()
{
	return _maxSensitivity;
}