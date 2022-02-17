#include "Application/Application.h"

#include <Windows.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Logging.h"
#include "Gameplay/InputEngine.h"
#include "Application/Timing.h"
#include <filesystem>
#include "Layers/GLAppLayer.h"
#include "Utils/FileHelpers.h"
#include "Utils/ResourceManager/ResourceManager.h"

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

#include "Utils/ResourceManager/ResourceManager.h"

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
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/FirstPersonCamera.h"
#include "Gameplay/Components/MovingPlatform.h"
#include "Gameplay/Components/PlayerControl.h"
#include "Gameplay/Components/MorphAnimator.h"
#include "Gameplay/Components/BoomerangBehavior.h"
#include "Gameplay/Components/HealthManager.h"
#include "Gameplay/Components/ScoreCounter.h"
#include "Gameplay/Components/ControllerInput.h"
#include "Gameplay/Components/MenuElement.h"
#include "Gameplay/Components/PickUpBehaviour.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Layers/RenderLayer.h"
#include "Layers/InterfaceLayer.h"
#include "Layers/DefaultSceneLayer.h"
#include "Layers/LogicUpdateLayer.h"
#include "Layers/ImGuiDebugLayer.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Layers/Menu.h"

Application* Application::_singleton = nullptr;
std::string Application::_applicationName = "INFR-2350U - DEMO";

#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

using namespace Gameplay;

Application::Application() :
	_window(nullptr),
	_windowSize({DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT}),
	_isRunning(false),
	_isEditor(true),
	_windowTitle("INFR - 2350U"),
	_currentScene(nullptr),
	_targetScene(nullptr)
{ }

Application::~Application() = default;

Application& Application::Get() {
	LOG_ASSERT(_singleton != nullptr, "Failed to get application! Get was called before the application was started!");
	return *_singleton;
}

void Application::Start(int argCount, char** arguments) {
	LOG_ASSERT(_singleton == nullptr, "Application has already been started!");
	_singleton = new Application();
	_singleton->_Run();
}

GLFWwindow* Application::GetWindow() { return _window; }

const glm::ivec2& Application::GetWindowSize() const { return _windowSize; }


const glm::uvec4& Application::GetPrimaryViewport() const {
	return _primaryViewport;
}

void Application::SetPrimaryViewport(const glm::uvec4& value) {
	_primaryViewport = value;
}

void Application::ResizeWindow(const glm::ivec2& newSize)
{
	_HandleWindowSizeChanged(newSize);
}

void Application::Quit() {
	_isRunning = false;
}

bool Application::LoadScene(const std::string& path) {
	if (std::filesystem::exists(path)) {

		std::string manifestPath = std::filesystem::path(path).stem().string() + "-manifest.json";
		if (std::filesystem::exists(manifestPath)) {
			LOG_INFO("Loading manifest from \"{}\"", manifestPath);
			ResourceManager::LoadManifest(manifestPath);
		}

		Gameplay::Scene::Sptr scene = Gameplay::Scene::Load(path);
		LoadScene(scene);
		return scene != nullptr;
	}
	return false;
}

void Application::LoadScene(const Gameplay::Scene::Sptr& scene) {
	_targetScene = scene;
}

void Application::SaveSettings()
{
	std::filesystem::path appdata = getenv("APPDATA");
	std::filesystem::path settingsPath = appdata / _applicationName / "app-settings.json";

	if (!std::filesystem::exists(appdata / _applicationName)) {
		std::filesystem::create_directory(appdata / _applicationName);
	}

	FileHelpers::WriteContentsToFile(settingsPath.string(), _appSettings.dump(1, '\t'));
}

void Respawn(GameObject::Sptr player)
{
	glm::vec3 spawnPoints[] = { glm::vec3(44.5, 20.0, 0.5),
								glm::vec3(26.0, 40.0, 0.5),
								glm::vec3(-18.5, -4.0, 0.5),
								glm::vec3(8.0, -5.0, 7.0) };
	int selection = rand() % 4;
	player->SetPosition(spawnPoints[selection]);
	player->Get<HealthManager>()->ResetHealth();
}

void Application::_Run()
{
	// TODO: Register layers
	_layers.push_back(std::make_shared<GLAppLayer>());
	_layers.push_back(std::make_shared<Menu>());
	_layers.push_back(std::make_shared<DefaultSceneLayer>());
	_layers.push_back(std::make_shared<RenderLayer>());
	_layers.push_back(std::make_shared<InterfaceLayer>());
	_layers.push_back(std::make_shared<LogicUpdateLayer>());

	// If we're in editor mode, we add all the editor layers
	if (_isEditor) {
		_layers.push_back(std::make_shared<ImGuiDebugLayer>());
	}

	// Either load the settings, or use the defaults
	_ConfigureSettings();

	// We'll grab these since we'll need them!
	_windowSize.x = JsonGet(_appSettings, "window_width", DEFAULT_WINDOW_WIDTH);
	_windowSize.y = JsonGet(_appSettings, "window_height", DEFAULT_WINDOW_HEIGHT);

	// By default, we want our viewport to be the whole screen
	_primaryViewport = { 0, 0, _windowSize.x, _windowSize.y };

	// Register all component and resource types
	_RegisterClasses();

	// Load all layers
	_Load();

	// Grab current time as the previous frame
	double lastFrame =  glfwGetTime();

	// Done loading, app is now running!
	_isRunning = true;

	bool p1Dying = false;
	bool p2Dying = false;

	float p1HitTimer = 0.0f;
	float p2HitTimer = 0.0f;

	bool menuScreen = true;
	bool firstFrame = true;

	//GetLayer<DefaultSceneLayer>()->BeginLayer();

	float selectTime = 0.5f;

	MenuElement::Sptr currentElement;
	std::vector<MenuElement::Sptr> menuItems;
	int currentItemInd = 0;

		// Infinite loop as long as the application is running
	while (_isRunning) {
		// Handle scene switching
		if (_targetScene != nullptr) {
			_HandleSceneChange();
		}

		// Receive events like input and window position/size changes from GLFW
		glfwPollEvents();

		// Handle closing the app via the close button
		if (glfwWindowShouldClose(_window)) {
			_isRunning = false;
		}

		// Grab the timing singleton instance as a reference
		Timing& timing = Timing::_singleton;

		// Figure out the current time, and the time since the last frame
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);
		float scaledDt = dt * timing._timeScale;

		// Update all timing values
		timing._unscaledDeltaTime = dt;
		timing._deltaTime = scaledDt;
		timing._timeSinceAppLoad += scaledDt;
		timing._unscaledTimeSinceAppLoad += dt;
		timing._timeSinceSceneLoad += scaledDt;
		timing._unscaledTimeSinceSceneLoad += dt;

		ImGuiHelper::StartFrame();

		//GameObject::Sptr player1 = _currentScene->FindObjectByName("Player 1");
		if (firstFrame)
		{
			
			currentElement = _currentScene->FindObjectByName("Play Button")->Get<MenuElement>();
			currentElement->GrowElement();

			menuItems.push_back(_currentScene->FindObjectByName("Play Button")->Get<MenuElement>());
			menuItems.push_back(_currentScene->FindObjectByName("Options Button")->Get<MenuElement>());
			menuItems.push_back(_currentScene->FindObjectByName("Exit Button")->Get<MenuElement>());

			firstFrame = false;
		}

		if (menuScreen)
		{
			bool downSelect;
			bool upSelect;
			bool confirm;

			confirm = _currentScene->FindObjectByName("Menu Control")->Get<ControllerInput>()->GetButtonDown(GLFW_GAMEPAD_BUTTON_A);

			ControllerInput::Sptr menuControl = _currentScene->FindObjectByName("Menu Control")->Get<ControllerInput>();
			if (menuControl->IsValid())
			{
				downSelect = menuControl->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) > 0.2f;
				upSelect = menuControl->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) < -0.2f;
				confirm = menuControl->GetButtonDown(GLFW_GAMEPAD_BUTTON_A);
			}

			else
			{
				downSelect = glfwGetKey(_window, GLFW_KEY_S);
				upSelect = glfwGetKey(_window, GLFW_KEY_W);
				confirm = glfwGetKey(_window, GLFW_KEY_ENTER);

			}

			//if (_currentScene->FindObjectByName("Menu Control")->Get<ControllerInput>()->GetButtonDown(GLFW_GAMEPAD_BUTTON_A))
			if (downSelect && selectTime >= 0.3f)
			{
				currentElement->ShrinkElement();
				currentItemInd++;

				if (currentItemInd >= menuItems.size()) currentItemInd = 0;
				currentElement = menuItems[currentItemInd];

				currentElement->GrowElement();
				selectTime = 0.0f;
			}

			else if (upSelect && selectTime >= 0.3f)
			{
				currentElement->ShrinkElement();
				currentItemInd--;
				if (currentItemInd < 0) currentItemInd = menuItems.size() - 1;

				currentElement = menuItems[currentItemInd];

				currentElement->GrowElement();
				selectTime = 0.0f;
			}

			else if (currentElement == _currentScene->FindObjectByName("Play Button")->Get<MenuElement>() && confirm)
			{
				menuScreen = false;

				GetLayer<Menu>()->SetActive(false);

				GetLayer<DefaultSceneLayer>()->BeginLayer();

				LoadScene(GetLayer<DefaultSceneLayer>()->GetScene());

				GetLayer<DefaultSceneLayer>()->SetActive(true);

				GetLayer<DefaultSceneLayer>()->GetScene()->IsPlaying = true;

				menuScreen = false;
			}

			else selectTime += dt;
				
				//_currentScene->~Scene();
				//GetLayer<DefaultSceneLayer>()->BeginLayer();
				//menuScreen = false;
			
			
		}
		
		else
		{
			GameObject::Sptr player1 = _currentScene->FindObjectByName("Player 1");
			GameObject::Sptr player2 = _currentScene->FindObjectByName("Player 2");
			GameObject::Sptr boomerang1 = _currentScene->FindObjectByName("Boomerang 1");
			GameObject::Sptr boomerang2 = _currentScene->FindObjectByName("Boomerang 2");

			GameObject::Sptr detachedCam = _currentScene->FindObjectByName("Detached Camera");

			//Boomerang Visual indicator!!!
			GameObject::Sptr dBoom1 = _currentScene->FindObjectByName("Display Boomerang 1");

			Camera::Sptr camera = _currentScene->PlayerCamera;
			
			glm::vec3 cameraLocalForward;

			cameraLocalForward = glm::vec3(camera->GetView()[0][2], camera->GetView()[1][2], camera->GetView()[2][2]) * -1.0f;

			dBoom1->SetPosition(player1->GetPosition() + glm::vec3(0.0f, 0.0f, 1.5f) + cameraLocalForward * 0.5f);

			dBoom1->SetRotation(camera->GetGameObject()->GetRotation());
			////////////////////Handle some UI stuff/////////////////////////////////

			GameObject::Sptr p1Health = _currentScene->FindObjectByName("Player1Health");
			GameObject::Sptr p2Health = _currentScene->FindObjectByName("Player2Health");

			GameObject::Sptr p1DamageScreen = _currentScene->FindObjectByName("DamageFlash1");
			GameObject::Sptr p2DamageScreen = _currentScene->FindObjectByName("DamageFlash2");

			//Find the current health of the player, divide it by the maximum health, lerp between the minimum and maximum health bar values using this as the interp. parameter
			p1Health->Get<RectTransform>()->SetMax({ glm::lerp(5.0f, 195.0f,
				player1->Get<HealthManager>()->GetHealth() / player1->Get<HealthManager>()->GetMaxHealth()), 45 });

			p1Health->Get<GuiPanel>()->SetColor(glm::lerp(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
				player1->Get<HealthManager>()->GetHealth() / player1->Get<HealthManager>()->GetMaxHealth()));

			p1DamageScreen->Get<GuiPanel>()->SetColor(glm::vec4(
				p1DamageScreen->Get<GuiPanel>()->GetColor().x,
				p1DamageScreen->Get<GuiPanel>()->GetColor().y,
				p1DamageScreen->Get<GuiPanel>()->GetColor().x,
				player1->Get<HealthManager>()->GetDamageOpacity()));

			p2Health->Get<RectTransform>()->SetMax({ glm::lerp(5.0f, 195.0f,
				player2->Get<HealthManager>()->GetHealth() / player2->Get<HealthManager>()->GetMaxHealth()), 45 });

			p2Health->Get<GuiPanel>()->SetColor(glm::lerp(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
				player2->Get<HealthManager>()->GetHealth() / player2->Get<HealthManager>()->GetMaxHealth()));

			p2DamageScreen->Get<GuiPanel>()->SetColor(glm::vec4(
				p2DamageScreen->Get<GuiPanel>()->GetColor().x,
				p2DamageScreen->Get<GuiPanel>()->GetColor().y,
				p2DamageScreen->Get<GuiPanel>()->GetColor().x,
				player2->Get<HealthManager>()->GetDamageOpacity()));

			if (player1->Get<MorphAnimator>() != nullptr)
			{
				if (player1->Get<HealthManager>()->IsDead() && !p1Dying)
				{
					std::string enemyNum = std::to_string(player1->Get<HealthManager>()->GotHitBy());

					GameObject::Sptr enemy = _currentScene->FindObjectByName("Player " + enemyNum);

					enemy->Get<ScoreCounter>()->AddScore();

					std::cout << "Player " << enemyNum << "'s score: "
						<< _currentScene->FindObjectByName("Player " + enemyNum)->Get<ScoreCounter>()->GetScore();

					glm::vec4 enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 1.0f));

					enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 0.0f));

					player1->Get<MorphAnimator>()->ActivateAnim("Die");
					p1Dying = true;
				}


				else if (p1Dying && player1->Get<MorphAnimator>()->IsEndOfClip())
				{
					Respawn(player1);
					p1Dying = false;
				}
				else if (!p1Dying)
				{
					//If the player is pressing the throw button and is in a the appropriate state, activate the throw anim
					if (player1->Get<PlayerControl>()->GetJustThrew())
					{
						player1->Get<MorphAnimator>()->ActivateAnim("Attack");
					}

					//If the player has just jumped, activate the jump anim
					else if (player1->Get<JumpBehaviour>()->IsStartingJump())
					{
						player1->Get<MorphAnimator>()->ActivateAnim("Jump");
					}

					//Else if the player is in the air and the jump anim has finished
					else if (player1->Get<MorphAnimator>()->GetActiveAnim() == "jump" && player1->Get<MorphAnimator>()->IsEndOfClip())
					{
						//If the player is moving, then run in the air
						if (player1->Get<PlayerControl>()->IsMoving())
							player1->Get<MorphAnimator>()->ActivateAnim("Walk");

						//Else, idle in the air
						else
							player1->Get<MorphAnimator>()->ActivateAnim("Idle");
					}

					//Else if the player is moving and isn't in the middle of jumping
					else if (player1->Get<PlayerControl>()->IsMoving() && player1->Get<MorphAnimator>()->GetActiveAnim() != "jump"
						&& (player1->Get<MorphAnimator>()->GetActiveAnim() != "attack" || player1->Get<MorphAnimator>()->IsEndOfClip()))
					{
						//If the player is pressing sprint and isn't already in the running animation
						if (player1->Get<MorphAnimator>()->GetActiveAnim() != "run" && player1->Get<PlayerControl>()->IsSprinting())
							player1->Get<MorphAnimator>()->ActivateAnim("Run");

						//If the player isn't pressing sprint and isn't already in the walking animation
						else if (player1->Get<MorphAnimator>()->GetActiveAnim() != "walk" && !player1->Get<PlayerControl>()->IsSprinting())
							player1->Get<MorphAnimator>()->ActivateAnim("Walk");
					}

					//Else if the player isn't moving and isn't jumping and isn't already idling
					else if (!player1->Get<PlayerControl>()->IsMoving() && player1->Get<MorphAnimator>()->GetActiveAnim() != "jump" &&
						(player1->Get<MorphAnimator>()->GetActiveAnim() != "attack" || player1->Get<MorphAnimator>()->IsEndOfClip())
						&& player1->Get<MorphAnimator>()->GetActiveAnim() != "idle")
					{
						player1->Get<MorphAnimator>()->ActivateAnim("Idle");
					}
				}
			}

			if (player2->Get<MorphAnimator>() != nullptr)
			{

				if (player2->Get<HealthManager>()->IsDead() && !p2Dying)
				{
					std::string enemyNum = std::to_string(player2->Get<HealthManager>()->GotHitBy());

					GameObject::Sptr enemy = _currentScene->FindObjectByName("Player " + enemyNum);

					enemy->Get<ScoreCounter>()->AddScore();

					std::cout << "Player " << enemyNum << "'s score: "
						<< _currentScene->FindObjectByName("Player " + enemyNum)->Get<ScoreCounter>()->GetScore();

					glm::vec4 enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 1.0f));

					enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 0.0f));


					player2->Get<MorphAnimator>()->ActivateAnim("Die");
					p2Dying = true;
				}


				else if (p2Dying && player2->Get<MorphAnimator>()->IsEndOfClip())
				{
					Respawn(player2);
					p2Dying = false;
				}

				else if (!p2Dying)
				{
					//If the player is pressing the throw button and is in a the appropriate state, activate the throw anim
					if (player2->Get<PlayerControl>()->GetJustThrew())
					{
						player2->Get<MorphAnimator>()->ActivateAnim("Attack");
					}

					//If the player has just jumped, activate the jump anim
					else if (player2->Get<JumpBehaviour>()->IsStartingJump())
					{
						player2->Get<MorphAnimator>()->ActivateAnim("Jump");
					}

					//Else if the player is in the air and the jump anim has finished
					else if (player2->Get<MorphAnimator>()->GetActiveAnim() == "jump" && player2->Get<MorphAnimator>()->IsEndOfClip())
					{
						//If the player is moving, then run in the air
						if (player2->Get<PlayerControl>()->IsMoving())
							player2->Get<MorphAnimator>()->ActivateAnim("Walk");

						//Else, idle in the air
						else
							player2->Get<MorphAnimator>()->ActivateAnim("Idle");
					}

					//Else if the player is moving and isn't in the middle of jumping
					else if (player2->Get<PlayerControl>()->IsMoving() && player2->Get<MorphAnimator>()->GetActiveAnim() != "jump" &&
						(player2->Get<MorphAnimator>()->GetActiveAnim() != "attack" || player2->Get<MorphAnimator>()->IsEndOfClip()))
					{
						//If the player is pressing sprint and isn't already in the running animation
						if (player2->Get<MorphAnimator>()->GetActiveAnim() != "run" && player2->Get<PlayerControl>()->IsSprinting())
							player2->Get<MorphAnimator>()->ActivateAnim("Run");

						//If the player isn't pressing sprint and isn't already in the walking animation
						else if (player2->Get<MorphAnimator>()->GetActiveAnim() != "walk" && !player2->Get<PlayerControl>()->IsSprinting())
							player2->Get<MorphAnimator>()->ActivateAnim("Walk");
					}

					//Else if the player isn't moving and isn't jumping and isn't already idling
					else if (!player2->Get<PlayerControl>()->IsMoving() && player2->Get<MorphAnimator>()->GetActiveAnim() != "jump" &&
						(player2->Get<MorphAnimator>()->GetActiveAnim() != "attack" || player2->Get<MorphAnimator>()->IsEndOfClip()) &&
						player2->Get<MorphAnimator>()->GetActiveAnim() != "idle")
					{
						player2->Get<MorphAnimator>()->ActivateAnim("Idle");
					}
				}
			}
		}
		
		//////////////////////////////////////////////////////////
		// Core update loop
		if (_currentScene != nullptr) {
			_Update();
			_LateUpdate();
			_PreRender();
			_RenderScene();
			_PostRender();
		}

		// Store timing for next loop
		lastFrame = thisFrame;

		InputEngine::EndFrame();
		ImGuiHelper::EndFrame();

		glfwSwapBuffers(_window);

	}

	// Unload all our layers
	_Unload();
}

void Application::_RegisterClasses()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	// Initialize our resource manager
	ResourceManager::Init();

	// Register all our resource types so we can load them from manifest files
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<TextureCube>();
	ResourceManager::RegisterType<ShaderProgram>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();
	ResourceManager::RegisterType<Font>();

	// Register all of our component types so we can load them from files
	ComponentManager::RegisterType<Camera>();
	ComponentManager::RegisterType<RenderComponent>();
	ComponentManager::RegisterType<RigidBody>();
	ComponentManager::RegisterType<TriggerVolume>();
	ComponentManager::RegisterType<RotatingBehaviour>();
	ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();
	ComponentManager::RegisterType<TriggerVolumeEnterBehaviour>();
	ComponentManager::RegisterType<SimpleCameraControl>();
	ComponentManager::RegisterType<RectTransform>();
	ComponentManager::RegisterType<GuiPanel>();
	ComponentManager::RegisterType<GuiText>();

	ComponentManager::RegisterType<ControllerInput>();
	ComponentManager::RegisterType<FirstPersonCamera>();
	ComponentManager::RegisterType<MovingPlatform>();
	ComponentManager::RegisterType<PlayerControl>();
	ComponentManager::RegisterType<MorphAnimator>();
	ComponentManager::RegisterType<BoomerangBehavior>();
	ComponentManager::RegisterType<HealthManager>();
	ComponentManager::RegisterType<ScoreCounter>();
	ComponentManager::RegisterType<MenuElement>();
	ComponentManager::RegisterType<PickUpBehaviour>();
}

void Application::_Load() {
	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnAppLoad)) {
			layer->OnAppLoad(_appSettings);
		}
	}

	// Pass the window to the input engine and let it initialize itself
	InputEngine::Init(_window);
	
	// Initialize our ImGui helper
	ImGuiHelper::Init(_window);

	GuiBatcher::SetWindowSize(_windowSize);
}

void Application::_Update() {
	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnUpdate)) {
			layer->OnUpdate();
		}
	}
}

void Application::_LateUpdate() {
	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnLateUpdate)) {
			layer->OnLateUpdate();
		}
	}
}

void Application::_PreRender()
{
	glm::ivec2 size ={ 0, 0 };
	glfwGetWindowSize(_window, &size.x, &size.y);
	glViewport(0, 0, size.x, size.y);
	glScissor(0, 0, size.x, size.y);

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnPreRender)) {
			layer->OnPreRender();
		}
	}
}

void Application::_RenderScene() {

	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnRender)) {
			layer->OnRender();
		}
	}
}

void Application::_PostRender() {
	// Note that we use a reverse iterator for post render
	for (auto it = _layers.crbegin(); it != _layers.crend(); it++) {
		const auto& layer = *it;
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnPostRender)) {
			layer->OnPostRender();
		}
	}
}

void Application::_Unload() {
	// Note that we use a reverse iterator for unloading
	for (auto it = _layers.crbegin(); it != _layers.crend(); it++) {
		const auto& layer = *it;
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnAppUnload)) {
			layer->OnAppUnload();
		}
	}

	// Clean up ImGui
	ImGuiHelper::Cleanup();
}

void Application::_HandleSceneChange() {
	// If we currently have a current scene, let the layers know it's being unloaded
	if (_currentScene != nullptr) {
		// Note that we use a reverse iterator, so that layers are unloaded in the opposite order that they were loaded
		for (auto it = _layers.crbegin(); it != _layers.crend(); it++) {
			const auto& layer = *it;
			if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnSceneUnload)) {
				layer->OnSceneUnload();
			}
		}
	}

	_currentScene = _targetScene;
	
	// Let the layers know that we've loaded in a new scene
	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnSceneLoad)) {
			layer->OnSceneLoad();
		}
	}

	// Wake up all game objects in the scene
	_currentScene->Awake();

	// If we are not in editor mode, scenes play by default
	if (!_isEditor) {
		_currentScene->IsPlaying = true;
	}

	_targetScene = nullptr;
}

void Application::_HandleWindowSizeChanged(const glm::ivec2& newSize) {
	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnWindowResize)) {
			layer->OnWindowResize(_windowSize, newSize);
		}
	}
	_windowSize = newSize;
	_primaryViewport = { 0, 0, newSize.x, newSize.y };
	
	if (GetLayer<Menu>()->IsActive())
	{
		GetLayer<Menu>()->RepositionUI();
	}

	else if (GetLayer<DefaultSceneLayer>()->IsActive())
	{
		GetLayer<DefaultSceneLayer>()->RepositionUI();
	}
	
}

void Application::_ConfigureSettings() {
	// Start with the defaul application settings
	_appSettings = _GetDefaultAppSettings();

	// We'll store our settings in the %APPDATA% directory, under our application name
	std::filesystem::path appdata = getenv("APPDATA");
	std::filesystem::path settingsPath = appdata / _applicationName / "app-settings.json";

	// If the settings file exists, we can load it in!
	if (std::filesystem::exists(settingsPath)) {
		// Read contents and parse into a JSON blob
		std::string content = FileHelpers::ReadFile(settingsPath.string());
		nlohmann::json blob = nlohmann::json::parse(content);

		// We use merge_patch so that we can keep our defaults if they are missing from the file!
		_appSettings.merge_patch(blob);
	}
	// If the file does not exist, save the default application settings to the path
	else {
		SaveSettings();
	}
}

nlohmann::json Application::_GetDefaultAppSettings()
{
	nlohmann::json result ={};

	for (const auto& layer : _layers) {
		if (!layer->Name.empty()) {
			result[layer->Name] = layer->GetDefaultConfig();
		}
		else {
			LOG_WARN("Unnamed layer! Injecting settings into global namespace, may conflict with other layers!");
			result.merge_patch(layer->GetDefaultConfig());
		}
	}

	result["window_width"]  = DEFAULT_WINDOW_WIDTH;
	result["window_height"] = DEFAULT_WINDOW_HEIGHT;
	return result;
}

