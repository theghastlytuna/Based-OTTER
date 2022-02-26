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
	_enabled(true)
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

	return _enabled ? buttons[ID] : false;
}

float ControllerInput::GetAxisValue(int ID)
{
	auto axes = glfwGetJoystickAxes(_controllerID, &_axesCount);

	return _enabled ? axes[ID] : 0.0f;
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