#pragma once
#include "Application/ApplicationLayer.h"
#include "json.hpp"
#include "Gameplay/Scene.h"

/**
 * This example layer handles creating a default test scene, which we will use 
 * as an entry point for creating a sample scene
 */
class DefaultSceneLayer final : public ApplicationLayer {
public:
	MAKE_PTRS(DefaultSceneLayer)

	DefaultSceneLayer();
	virtual ~DefaultSceneLayer();

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