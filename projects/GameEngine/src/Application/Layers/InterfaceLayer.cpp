#include "InterfaceLayer.h"
#include "Graphics/GuiBatcher.h"
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include "../Application.h"
#include "Gameplay/Components/GUI/RectTransform.h"
#include "DefaultSceneLayer.h"
#include "Menu.h"

InterfaceLayer::InterfaceLayer() :
	ApplicationLayer()
{
	Name = "Interface";
	Overrides = AppLayerFunctions::OnRender | AppLayerFunctions::OnWindowResize;
}

InterfaceLayer::~InterfaceLayer()
{ }

void InterfaceLayer::OnRender() {
	// Gets the application instance
	Application& app = Application::Get();

	// We can use the application's viewport to set our OpenGL viewport, as well as clip rendering to that area
	const glm::uvec4& viewport = app.GetPrimaryViewport();
	glViewport(viewport.x, viewport.y, viewport.z, viewport.w / 2.0f);

	// Disable culling
	glDisable(GL_CULL_FACE);
	// Disable depth testing, we're going to use order-dependant layering
	glDisable(GL_DEPTH_TEST);
	// Disable depth writing
	glDepthMask(GL_FALSE);

	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Our projection matrix will be our entire window for now
	glm::mat4 proj = glm::ortho(0.0f, (float)app.GetWindowSize().x, (float)app.GetWindowSize().y, 0.0f, -1.0f, 1.0f);
	GuiBatcher::SetProjection(proj);

	// Iterate over and render all the GUI objects
	app.CurrentScene()->RenderGUI(1);

	// Flush the Gui Batch renderer
	GuiBatcher::Flush();

	// We can use the application's viewport to set our OpenGL viewport, as well as clip rendering to that area
	const glm::uvec4& viewport2 = app.GetPrimaryViewport();
	glViewport(viewport2.x, viewport2.w / 2.0f, viewport2.z, viewport2.w / 2.0f);

	// Disable culling
	glDisable(GL_CULL_FACE);
	// Disable depth testing, we're going to use order-dependant layering
	glDisable(GL_DEPTH_TEST);
	// Disable depth writing
	glDepthMask(GL_FALSE);

	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Iterate over and render all the GUI objects
	app.CurrentScene()->RenderGUI(2);

	// Flush the Gui Batch renderer
	GuiBatcher::Flush();

	// We can use the application's viewport to set our OpenGL viewport, as well as clip rendering to that area
	const glm::uvec4& viewport3 = app.GetPrimaryViewport();
	glViewport(viewport3.x, viewport3.y, viewport3.z, viewport3.w);

	// Disable culling
	glDisable(GL_CULL_FACE);
	// Disable depth testing, we're going to use order-dependant layering
	glDisable(GL_DEPTH_TEST);
	// Disable depth writing
	glDepthMask(GL_FALSE);

	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Iterate over and render all the GUI objects
	app.CurrentScene()->RenderGUI(5);

	// Flush the Gui Batch renderer
	GuiBatcher::Flush();

	// Disable alpha blending
	glDisable(GL_BLEND);
	// Disable scissor testing
	glDisable(GL_SCISSOR_TEST);
	// Re-enable depth writing
	glDepthMask(GL_TRUE);
}

void InterfaceLayer::OnWindowResize(const glm::ivec2& oldSize, const glm::ivec2& newSize) {
	// Notify our GUI batcher class of the new window size
	GuiBatcher::SetWindowSize(newSize);

	Application& app = Application::Get();

	app.GetLayer<DefaultSceneLayer>()->RepositionUI();
	//app.GetLayer<Menu>()->RepositionUI();

	/*
	Gameplay::GameObject::Sptr crosshair = app.CurrentScene()->FindObjectByName("Crosshairs");
	Gameplay::GameObject::Sptr crosshair2 = app.CurrentScene()->FindObjectByName("Crosshairs 2");

	Gameplay::GameObject::Sptr killUI = app.CurrentScene()->FindObjectByName("Score Counter 1");
	Gameplay::GameObject::Sptr killUI2 = app.CurrentScene()->FindObjectByName("Score Counter 2");

	crosshair->Get<RectTransform>()->SetMin({ newSize.x / 2 - 50, newSize.y / 2 - 50 });
	crosshair->Get<RectTransform>()->SetMax({ newSize.x / 2 + 50, newSize.y / 2 + 50 });

	crosshair2->Get<RectTransform>()->SetMin({ newSize.x / 2 - 50, newSize.y / 2 - 50 });
	crosshair2->Get<RectTransform>()->SetMax({ newSize.x / 2 + 50, newSize.y / 2 + 50 });

	killUI->Get<RectTransform>()->SetMin({ newSize.x - 250, 5 });
	killUI->Get<RectTransform>()->SetMax({ newSize.x - 80, 100 });

	killUI2->Get<RectTransform>()->SetMin({ newSize.x - 250, 5 });
	killUI2->Get<RectTransform>()->SetMax({ newSize.x - 80, 100 });

	for (int i = 0; i < 10; i++)
	{
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMin({ newSize.x - 85, 10 });
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMax({ newSize.x - 10, 95 });

		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMin({ newSize.x - 85, 10 });
		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMax({ newSize.x - 10, 95 });
	}
	*/
}
