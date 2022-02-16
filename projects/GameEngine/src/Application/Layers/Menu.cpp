#include "Menu.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture2D.h"
#include "Graphics/TextureCube.h"
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
		/*
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});

		ShaderProgram::Sptr animShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/morphAnim.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});

		///////////////////// NEW SHADERS ////////////////////////////////////////////

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		*/
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

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

		GameObject::Sptr menuBG = scene->CreateGameObject("Menu BG");
		{
			menuBG->SetRenderFlag(5);

			RectTransform::Sptr transform = menuBG->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

			GuiPanel::Sptr canPanel = menuBG->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/placeholderBG.jpg"));
		}

		GameObject::Sptr play = scene->CreateGameObject("Play Button");
		{
			play->SetRenderFlag(5);
			RectTransform::Sptr transform = play->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - 200, app.GetWindowSize().y / 2 - 250 });
			transform->SetMax({ app.GetWindowSize().x / 2 + 200, app.GetWindowSize().y / 2 - 150 });

			play->Add<MenuElement>();

			GuiPanel::Sptr panel = play->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/Play Game.png"));
		}

		GameObject::Sptr options = scene->CreateGameObject("Options Button");
		{
			options->SetRenderFlag(5);
			RectTransform::Sptr transform = options->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - 200, app.GetWindowSize().y / 2 - 50 });
			transform->SetMax({ app.GetWindowSize().x / 2 + 200, app.GetWindowSize().y / 2 + 50 });

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
			transform->SetMin({ -150, 0 });
			transform->SetMax({ 500, 250 });

			transform->SetRotationDeg(-35.0f);

			GuiPanel::Sptr canPanel = logo->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/title_placeholder.png"));
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		//ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		//scene->Save("scene.json");

		_scene = scene;

		// Send the scene to the application
		app.LoadScene(scene);
		_active = true;
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
	return _scene;
}

void Menu::RepositionUI()
{
	Application& app = Application::Get();
	
	Gameplay::GameObject::Sptr menuBG = app.CurrentScene()->FindObjectByName("Menu BG");
	Gameplay::GameObject::Sptr playBut = app.CurrentScene()->FindObjectByName("Play Button");
	Gameplay::GameObject::Sptr optionsBut = app.CurrentScene()->FindObjectByName("Options Button");
	Gameplay::GameObject::Sptr exitBut = app.CurrentScene()->FindObjectByName("Exit Button");

	menuBG->Get<RectTransform>()->SetMin({ 0, 0 });
	menuBG->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

	playBut->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 200, app.GetWindowSize().y / 2 - 250 });
	playBut->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 200, app.GetWindowSize().y / 2 - 150 });

	optionsBut->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 200, app.GetWindowSize().y / 2 - 50 });
	optionsBut->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 200, app.GetWindowSize().y / 2 + 50 });

	exitBut->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 200, app.GetWindowSize().y / 2 + 150 });
	exitBut->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 200, app.GetWindowSize().y / 2 + 250 });
}