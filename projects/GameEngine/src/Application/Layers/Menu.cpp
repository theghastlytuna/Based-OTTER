#include "Menu.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/FirstPersonCamera.h"
#include "Gameplay/Components/MovingPlatform.h"
#include "Gameplay/Components/PlayerControl.h"
#include "Gameplay/Components/MorphAnimator.h"
#include "Gameplay/Components/BoomerangBehavior.h"
#include "Gameplay/Components/HealthManager.h"
#include "Gameplay/Components/ScoreCounter.h"
#include "Gameplay/Components/ControllerInput.h"
#include "Gameplay/Components/MenuElement.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"

// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

Menu::Menu() :
	ApplicationLayer()
{
	Name = "Menu";
	Overrides = AppLayerFunctions::OnAppLoad;
}

Menu::~Menu() = default;

void Menu::OnAppLoad(const nlohmann::json & config) {
	//_CreateScene();
}

void Menu::BeginLayer()
{
	_CreateScene();
}

void Menu::_CreateScene() {

	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	}

	else {

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

		scene->SetAmbientLight(glm::vec3(1.f));

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("MainCam1");
		{
			camera->SetPosition(glm::vec3(5.0f));
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();

			Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
			scene->WorldCamera = cam;
		}

		//Set up the scene's camera 
		GameObject::Sptr camera2 = scene->CreateGameObject("MainCam2");
		{
			camera2->SetPosition(glm::vec3(5.0f));
			camera2->LookAt(glm::vec3(0.0f));

			Camera::Sptr cam = camera2->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera! 
			scene->MainCamera2 = cam;
		}

		GameObject::Sptr detachedCam = scene->CreateGameObject("DeCam1");
		{
			ControllerInput::Sptr controller1 = detachedCam->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);

			detachedCam->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam->Add<Camera>();
			scene->PlayerCamera = cam;
		}

		GameObject::Sptr detachedCam2 = scene->CreateGameObject("DeCam2");
		{
			ControllerInput::Sptr controller2 = detachedCam2->Add<ControllerInput>();
			controller2->SetController(GLFW_JOYSTICK_2);

			detachedCam2->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam2->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam2->Add<Camera>();
			scene->PlayerCamera2 = cam;
		}

		GameObject::Sptr player1 = scene->CreateGameObject("Menu Control");
		{
			ControllerInput::Sptr controller1 = player1->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);
		}

		////////////Main menu elements////////////////////////
		GameObject::Sptr menuBG = scene->CreateGameObject("Menu BG");
		{
			menuBG->SetRenderFlag(5);

			RectTransform::Sptr transform = menuBG->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

			GuiPanel::Sptr canPanel = menuBG->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/realBG.png"));

			canPanel->SetTransparency(0.1f); ///////////////Temporary
		}

		GameObject::Sptr playDesert = scene->CreateGameObject("Play Desert");
		{
			playDesert->SetRenderFlag(5);

			RectTransform::Sptr transform = playDesert->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x  - 400, app.GetWindowSize().y /2 + 100 });
			transform->SetMax({ app.GetWindowSize().x + 200, app.GetWindowSize().y /2 });

			playDesert->Add<MenuElement>();

			GuiPanel::Sptr panel = playDesert->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/play_desert.png"));
		}

		GameObject::Sptr playJungle = scene->CreateGameObject("Play Jungle");
		{
			playJungle->SetRenderFlag(5);

			RectTransform::Sptr transform = playJungle->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x - 400, app.GetWindowSize().y / 2 });
			transform->SetMax({ app.GetWindowSize().x + 200, app.GetWindowSize().y / 2 - 100 });

			playJungle->Add<MenuElement>();

			GuiPanel::Sptr panel = playJungle->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/play_jungle.png"));
		}

		GameObject::Sptr options = scene->CreateGameObject("Options Button");
		{
			options->SetRenderFlag(5);

			RectTransform::Sptr transform = options->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x - 200, app.GetWindowSize().y / 2 - 50 });
			transform->SetMax({ app.GetWindowSize().x + 200, app.GetWindowSize().y / 2 + 50 });

			options->Add<MenuElement>();

			GuiPanel::Sptr panel = options->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/options.png"));
		}

		GameObject::Sptr exit = scene->CreateGameObject("Exit Button");
		{
			exit->SetRenderFlag(5);

			RectTransform::Sptr transform = exit->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - 200, app.GetWindowSize().y / 2 + 150 });
			transform->SetMax({ app.GetWindowSize().x / 2 + 200, app.GetWindowSize().y / 2 + 250 });

			exit->Add<MenuElement>();

			GuiPanel::Sptr panel = exit->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/exit.png"));
		}

		GameObject::Sptr logo = scene->CreateGameObject("Logo");
		{
			logo->SetRenderFlag(5);

			RectTransform::Sptr transform = logo->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ 650, 250 });

			//transform->SetRotationDeg(-35.0f);

			GuiPanel::Sptr canPanel = logo->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/title.png"));
		}

		//////////////Loading screen//////////////////
		GameObject::Sptr loadingScreen = scene->CreateGameObject("Loading Screen");
		{
			loadingScreen->SetRenderFlag(5);

			RectTransform::Sptr transform = loadingScreen->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

			GuiPanel::Sptr canPanel = loadingScreen->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/gameLoad.png"));

			canPanel->SetTransparency(0.0f);
		}

		////////////Options screen elements///////////////////////
		GameObject::Sptr volumeText = scene->CreateGameObject("Volume Text");
		{
			volumeText->SetRenderFlag(5);

			RectTransform::Sptr transform = volumeText->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 9, app.GetWindowSize().y / 8 });
			transform->SetMax({ app.GetWindowSize().x / 3, app.GetWindowSize().y / 4 });

			volumeText->Add<MenuElement>();

			GuiPanel::Sptr canPanel = volumeText->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeTextFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr volumeBar = scene->CreateGameObject("Volume Bar");
		{
			volumeBar->SetRenderFlag(5);

			RectTransform::Sptr transform = volumeBar->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 8 });
			transform->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 4});

			//volumeBar->Add<MenuElement>();

			GuiPanel::Sptr canPanel = volumeBar->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeBarFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr volumeSelector = scene->CreateGameObject("Volume Selector");
		{
			volumeSelector->SetRenderFlag(5);

			RectTransform::Sptr transform = volumeSelector->Add<RectTransform>();
			transform->SetMin({ (volumeBar->Get<RectTransform>()->GetMin().x + volumeBar->Get<RectTransform>()->GetMax().x) / 2 - 10, app.GetWindowSize().y / 8 });
			transform->SetMax({ (volumeBar->Get<RectTransform>()->GetMin().x + volumeBar->Get<RectTransform>()->GetMax().x) / 2 + 10, app.GetWindowSize().y / 4 });

			//volumeSelector->Add<MenuElement>();

			GuiPanel::Sptr canPanel = volumeSelector->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelector.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensText = scene->CreateGameObject("Sensitivity Text");
		{
			sensText->SetRenderFlag(5);

			RectTransform::Sptr transform = sensText->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 9, 3 * app.GetWindowSize().y / 8 });
			transform->SetMax({ app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 });

			sensText->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensText->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/sensitivityTextFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensBar = scene->CreateGameObject("Sensitivity Bar");
		{
			sensBar->SetRenderFlag(5);

			RectTransform::Sptr transform = sensBar->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2, 3 * app.GetWindowSize().y / 8 });
			transform->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 2 });

			//sensBar->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensBar->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeBarFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensSelector = scene->CreateGameObject("Sensitivity Selector");
		{
			sensSelector->SetRenderFlag(5);

			RectTransform::Sptr transform = sensSelector->Add<RectTransform>();
			transform->SetMin({ (sensBar->Get<RectTransform>()->GetMin().x + sensBar->Get<RectTransform>()->GetMax().x) / 2 - 10, 3 * app.GetWindowSize().y / 8 });
			transform->SetMax({ (sensBar->Get<RectTransform>()->GetMin().x + sensBar->Get<RectTransform>()->GetMax().x) / 2 + 10, app.GetWindowSize().y / 2 });

			//sensSelector->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensSelector->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelector.png"));

			canPanel->SetTransparency(0.0f);
		}

		///////////////////////////////////////////////

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		//ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		//scene->Save("scene.json");

		_scene.push_back(scene);
		currentSceneNum++;
		// Send the scene to the application
		//app.LoadScene(scene);
		//_active = true;
	}
}

void Menu::SetActive(bool active)
{
	_active = active;
}

bool Menu::IsActive()
{
	return _active;
}

Gameplay::Scene::Sptr Menu::GetScene()
{
	return _scene[currentSceneNum];
}

void Menu::RepositionUI()
{
	Application& app = Application::Get();

	////////Main menu elements
	Gameplay::GameObject::Sptr menuBG = app.CurrentScene()->FindObjectByName("Menu BG");
	Gameplay::GameObject::Sptr logo = app.CurrentScene()->FindObjectByName("Logo");
	Gameplay::GameObject::Sptr playDesert = app.CurrentScene()->FindObjectByName("Play Desert");
	Gameplay::GameObject::Sptr playJungle = app.CurrentScene()->FindObjectByName("Play Jungle");
	Gameplay::GameObject::Sptr optionsBut = app.CurrentScene()->FindObjectByName("Options Button");
	Gameplay::GameObject::Sptr exitBut = app.CurrentScene()->FindObjectByName("Exit Button");

	///////Loading screen
	Gameplay::GameObject::Sptr loadingScreen = app.CurrentScene()->FindObjectByName("Loading Screen");

	////////Options elements
	Gameplay::GameObject::Sptr volumeText = app.CurrentScene()->FindObjectByName("Volume Text");
	Gameplay::GameObject::Sptr volumeBar = app.CurrentScene()->FindObjectByName("Volume Bar");

	Gameplay::GameObject::Sptr sensText = app.CurrentScene()->FindObjectByName("Sensitivity Text");
	Gameplay::GameObject::Sptr sensBar = app.CurrentScene()->FindObjectByName("Sensitivity Bar");

	menuBG->Get<RectTransform>()->SetMin({ 0, 0 });
	menuBG->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

	playDesert->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 100, app.GetWindowSize().y / 2 + 100 });
	playDesert->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 300, app.GetWindowSize().y / 2 });
	if (playDesert->Get<MenuElement>()->IsSelected()) playDesert->Get<MenuElement>()->GrowElement();

	playJungle->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 100, app.GetWindowSize().y / 2 });
	playJungle->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 300, app.GetWindowSize().y / 2 + 100});
	if (playJungle->Get<MenuElement>()->IsSelected()) playJungle->Get<MenuElement>()->GrowElement();

	optionsBut->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 + 100});
	optionsBut->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 350, app.GetWindowSize().y / 2 + 200 });
	if (optionsBut->Get<MenuElement>()->IsSelected()) optionsBut->Get<MenuElement>()->GrowElement();

	exitBut->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 2 + 200 });
	exitBut->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 400, app.GetWindowSize().y / 2 + 300 });
	if (exitBut->Get<MenuElement>()->IsSelected()) exitBut->Get<MenuElement>()->GrowElement();
	logo->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 6, 0});
	logo->Get<RectTransform>()->SetMax({ app.GetWindowSize().x - (app.GetWindowSize().x / 6), app.GetWindowSize().y / 3 });

	loadingScreen->Get<RectTransform>()->SetMin({ 0, 0 });
	loadingScreen->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

	volumeText->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 9, app.GetWindowSize().y / 8 });
	volumeText->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 3, app.GetWindowSize().y / 4 });
	if (volumeText->Get<MenuElement>()->IsSelected()) volumeText->Get<MenuElement>()->GrowElement();

	volumeBar->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 8 });
	volumeBar->Get<RectTransform>()->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 4 });

	sensText->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 9, 3 * app.GetWindowSize().y / 8 });
	sensText->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 });
	if (sensText->Get<MenuElement>()->IsSelected()) sensText->Get<MenuElement>()->GrowElement();

	sensBar->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2, 3 * app.GetWindowSize().y / 8 });
	sensBar->Get<RectTransform>()->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 2 });
}
