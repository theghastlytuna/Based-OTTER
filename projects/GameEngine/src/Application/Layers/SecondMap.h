#pragma once
#include "Application/ApplicationLayer.h"
#include "json.hpp"
#include "Gameplay/Scene.h"

class SecondMap final : public ApplicationLayer
{
public:
	MAKE_PTRS(SecondMap)

	SecondMap();
	virtual ~SecondMap();

	// Inherited from ApplicationLayer

	virtual void OnAppLoad(const nlohmann::json& config) override;
	virtual void RepositionUI() override;

	void BeginLayer();

	void SetActive(bool active);
	bool IsActive();

	Gameplay::Scene::Sptr GetScene();

protected:
	void _CreateScene();

	Gameplay::Scene::Sptr _scene;

	bool _active = false;
};

