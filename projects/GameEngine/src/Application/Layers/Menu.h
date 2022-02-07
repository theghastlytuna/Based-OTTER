#pragma once
#include "Application/ApplicationLayer.h"
#include "json.hpp"

/**
 * This example layer handles creating a default test scene, which we will use
 * as an entry point for creating a sample scene
 */
class Menu final : public ApplicationLayer {
public:
	MAKE_PTRS(Menu)

		Menu();
	virtual ~Menu();

	// Inherited from ApplicationLayer

	virtual void OnAppLoad(const nlohmann::json& config) override;
	virtual void RepositionUI() override;

	void BeginLayer();

protected:
	void _CreateScene();
};
