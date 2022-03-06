#include "BoomerangBehavior.h"
#include "Gameplay/GameObject.h"
#include "Utils/ImGuiHelper.h"
#include "HealthManager.h"

BoomerangBehavior::BoomerangBehavior()
{
}

BoomerangBehavior::~BoomerangBehavior()
{
}

void BoomerangBehavior::Awake()
{
	//Set up references
	_boomerangEntity = GetGameObject()->SelfRef();
	_rigidBody = GetGameObject()->Get<Gameplay::Physics::RigidBody>();
	_scene = GetGameObject()->GetScene();
	if (GetGameObject()->Name == "Boomerang 1") {
		_player = _scene->FindObjectByName("Player 1");
		_camera = GetGameObject()->GetScene()->PlayerCamera;
	}
	else
	{
		_player = _scene->FindObjectByName("Player 2");
		_camera = GetGameObject()->GetScene()->PlayerCamera2;
	}

	_rigidBody->SetMass(1);
}

void BoomerangBehavior::Update(float deltaTime)
{
	if (_state != boomerangState::INACTIVE) {
		updateTrackingPoint(deltaTime);
	}
	defyGravity();
	Seek(deltaTime);
}

void BoomerangBehavior::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat, "Launch Force", &_boomerangLaunchForce, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "Boomerang Acceleration", &_boomerangAcceleration, 1.0f);
}

void BoomerangBehavior::Seek(float deltaTime)
{
	glm::vec3 desiredVector = _targetPoint - _boomerangEntity->GetPosition();
	glm::vec3 currentVector = _rigidBody->GetLinearVelocity();

	desiredVector = glm::normalize(desiredVector);
	currentVector = glm::normalize(currentVector);

	glm::vec3 appliedVector = glm::normalize(desiredVector - currentVector);
	if (_state == boomerangState::RETURNING)
	{
		appliedVector = appliedVector * _boomerangAcceleration * deltaTime;
	}
	else
	{
		appliedVector = appliedVector * _boomerangAcceleration * deltaTime * ((_triggerInput + 1) / 2);
	}
	_rigidBody->ApplyForce(appliedVector);

	//TODO: Limit Angle of the applied vector to enforce turning speeds?
	//Might make it more interesting to control
}

void BoomerangBehavior::throwWang(glm::vec3 playerPosition, float chargeLevel)
{
	_state = boomerangState::FORWARD;
	glm::vec3 cameraLocalForward = glm::vec3(_camera->GetView()[0][2], _camera->GetView()[1][2], _camera->GetView()[2][2]) * -1.0f;
	_boomerangEntity->SetPosition(playerPosition + glm::vec3(0.0f, 0.0f, 1.5f) + cameraLocalForward * _projectileSpacing);
	_rigidBody->SetLinearVelocity(glm::vec3(0));
	_rigidBody->SetLinearVelocity(cameraLocalForward * _boomerangLaunchForce * chargeLevel);
}

void BoomerangBehavior::defyGravity()
{
	_rigidBody->ApplyForce(glm::vec3(0, 0, 12.81f));
}

void BoomerangBehavior::updateTrackingPoint(float deltaTime)
{
	if (_state == boomerangState::FORWARD) {
		_distance += _distanceDelta * deltaTime;
	}
	else {
		_distance -= _distanceDelta * deltaTime;
	}
	_targetPoint = glm::vec3(_camera->GetView()[0][2], _camera->GetView()[1][2], _camera->GetView()[2][2]) * -_distance;

}

void BoomerangBehavior::returnBoomerang()
{
	_state = boomerangState::RETURNING;
}

void BoomerangBehavior::SetAcceleration(float newAccel)
{
	_boomerangAcceleration = newAccel;
}

void BoomerangBehavior::SetInactivePosition(glm::vec3 newPosition)
{
	_inactivePosition = newPosition;
}

glm::vec3 BoomerangBehavior::getPosition()
{
	return GetGameObject()->GetPosition();
}

bool BoomerangBehavior::getReadyToThrow()
{
	if (_state == boomerangState::INACTIVE) {
		return true;
	}
	return false;
}

void BoomerangBehavior::OnCollisionEnter()
{
	returnBoomerang();
}

void BoomerangBehavior::makeBoomerangInactive()
{
	_boomerangEntity->SetPosition(_inactivePosition);
	_state = boomerangState::INACTIVE;
	_rigidBody->SetLinearVelocity(glm::vec3(0, 0, 0));
}

nlohmann::json BoomerangBehavior::ToJson() const {
	return {
		{ "Launch Force", _boomerangLaunchForce }
	};
}

BoomerangBehavior::Sptr BoomerangBehavior::FromJson(const nlohmann::json& blob)
{
	BoomerangBehavior::Sptr result = std::make_shared<BoomerangBehavior>();
	result->_boomerangLaunchForce = blob["Launch Force"];
	return result;
}
