#include "HealthManager.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "BoomerangBehavior.h"


HealthManager::HealthManager()
	: IComponent()
{ }

HealthManager::~HealthManager() = default;

void HealthManager::Awake()
{

	if (GetGameObject()->Name == "Player 1")
	{
		_playerID = 1;
		_enemyID = 2;
		_player = GetGameObject()->GetScene()->FindObjectByName("Player 1");
	}

	else
	{
		_playerID = 2;
		_enemyID = 1;
		_player = GetGameObject()->GetScene()->FindObjectByName("Player 2");
	}
}

void HealthManager::Update(float deltaTime)
{
	if (_loseHealth)
	{
		_healthVal -= _damage;
	}

	if (_gotHit)
	{
		_damageScreenOpacity = 1.0f;
		_gotHit = false;
	}

	else if (_damageScreenOpacity > 0.0f)
	{
		_damageScreenOpacity -= deltaTime / 1.2f;

		if (_damageScreenOpacity < 0.0f)
			_damageScreenOpacity = 0.0f;
	}

	_loseHealth = false;
}

void HealthManager::OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	//LOG_INFO("Entered trigger: {}", trigger->GetGameObject()->Name);
	if (trigger->GetGameObject()->Name == "Boomerang " + std::to_string(_enemyID))
	{
		_loseHealth = true;
		std::cout << GetGameObject()->Name << " lost health, now down to " << std::to_string(_healthVal - 1) << std::endl;

		trigger->GetGameObject()->Get<BoomerangBehavior>()->returnBoomerang();
		_gotHit = true;

		//Find the value of the boomerang ID, assign it to the enemy boomerang ID
		_enemyBoomerangID = trigger->GetGameObject()->Name[trigger->GetGameObject()->Name.length() - 1] - '0';

		//Get the boomerang's direction reletive to the player, and store the damage scaling number. Front is 0.1x as a minimum, back is 1.2x as a max, defining a sin curve

		//Step 1: Get the vector from the player to the wang and the player's current heading
		glm::vec3 playerToWang = glm::normalize(trigger->GetGameObject()->GetPosition() - _player->GetPosition());
		//glm::vec3 playerDir = glm::normalize(_player->GetRotationEuler());

		glm::vec3 cameraLocalForward;
		Gameplay::Camera::Sptr camera;
		if (_playerID == 1) {
			camera = GetGameObject()->GetScene()->PlayerCamera;
		}
		else {
			camera = GetGameObject()->GetScene()->PlayerCamera2;
		}
		cameraLocalForward = glm::normalize(glm::vec3(camera->GetView()[0][2], camera->GetView()[1][2], camera->GetView()[2][2]) * -1.0f);
		
		//Calculate the angle between the vectors
		float dot = cameraLocalForward.x * playerToWang.x + cameraLocalForward.y * playerToWang.y + cameraLocalForward.z * playerToWang.z;
		float divisor = glm::length(playerToWang) * glm::length(cameraLocalForward);
		float angle = glm::acos(dot / divisor);
		float damageScaler = (-0.55 * glm::cos(angle)) + 0.65;
		_damage = damageScaler;
		
		
	}

	else if (trigger->GetGameObject()->Name == "Boomerang " + std::to_string(_playerID))
	{
		trigger->GetGameObject()->Get<BoomerangBehavior>()->makeBoomerangInactive();
	}
}

float HealthManager::GetHealth()
{
	return _healthVal;
}

float HealthManager::GetMaxHealth()
{
	return _maxHealth;
}

void HealthManager::ResetHealth()
{
	_healthVal = _maxHealth;
}

float HealthManager::GetDamageOpacity()
{
	return _damageScreenOpacity;
}

int HealthManager::GotHitBy()
{
	return _enemyBoomerangID;
}

bool HealthManager::IsDead()
{
	return _healthVal <= 0.0f;
}

void HealthManager::RenderImGui()
{
}

nlohmann::json HealthManager::ToJson() const
{
	return nlohmann::json();
}

HealthManager::Sptr HealthManager::FromJson(const nlohmann::json& blob)
{
	return HealthManager::Sptr();
}
