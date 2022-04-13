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
#include "Utils/ImGuiHelper.h"

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture1D.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

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
#include "Gameplay/Components/ParticleSystem.h"
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
#include "Gameplay/Components/Light.h"
#include "Gameplay/Components/ShadowCamera.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/Components/ComponentManager.h"

// Layers
#include "Layers/RenderLayer.h"
#include "Layers/InterfaceLayer.h"
#include "Layers/DefaultSceneLayer.h"
#include "Layers/LogicUpdateLayer.h"
#include "Layers/ImGuiDebugLayer.h"
#include "Utils/ImGuiHelper.h"
#include "Layers/ParticleLayer.h"
#include "Layers/Menu.h"
#include "Layers/SecondMap.h"
#include "Layers/EndScreen.h"

#include "SoundManaging.h"
#include "Layers/PostProcessingLayer.h" 

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
	_layers.push_back(std::make_shared<LogicUpdateLayer>());
	_layers.push_back(std::make_shared<RenderLayer>());
	_layers.push_back(std::make_shared<ParticleLayer>());
	_layers.push_back(std::make_shared<PostProcessingLayer>());
	_layers.push_back(std::make_shared<InterfaceLayer>());

	// If we're in editor mode, we add all the editor layers
	if (_isEditor) {
		_layers.push_back(std::make_shared<ImGuiDebugLayer>());
	}

	_layers.push_back(std::make_shared<Menu>());
	_layers.push_back(std::make_shared<DefaultSceneLayer>());
	_layers.push_back(std::make_shared<SecondMap>());
	_layers.push_back(std::make_shared<EndScreen>());

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

	bool firstFrame = true;
	bool started = false;
	bool paused = false;
	bool loading = false;
	bool desert = true;
	bool options = false;
	bool exitting = false;
	float exitTime = 0;
	bool FirstBlood = false;
	bool LastKill = false;

	//bool started = false;

	//GetLayer<DefaultSceneLayer>()->BeginLayer();

	float selectTime = 0.5f;
	float barSelectTime = 0.2f;

	MenuElement::Sptr currentElement;
	std::vector<MenuElement::Sptr> menuItems;
	int currentItemInd = 0;

	std::vector<MenuElement::Sptr> optionItems;
	
	float currentSensitivity = 2.0f; //This is just used to pass sensitivity from one scene to another; use ControllerInput::SetSensitivity to change a controller's sensitivity

	///////Grab the sound manager and load all the sounds we want///////////
	SoundManaging& soundManaging = SoundManaging::_singleton;

	struct soundInfo {
		float minVol = 0.0f;
		float maxVol = 1.0f;
		float currentVol = 0.5f;
	};

	soundInfo thisSoundInfo;

	soundManaging.LoadBank("fmod/Banks/Master.bank");
	soundManaging.LoadBank("fmod/Banks/Sounds.bank");
	soundManaging.LoadBank("fmod/Banks/Dialogue.bank");
	soundManaging.LoadStringBank("fmod/Banks/Master.strings.bank");

	// Dialogue
	soundManaging.SetEvent("event:/Dialogue/ExitGame", "Exit");
	soundManaging.SetEvent("event:/Dialogue/FirstBlood", "FirstBlood");
	soundManaging.SetEvent("event:/Dialogue/GameBootup", "Bootup");
	soundManaging.SetEvent("event:/Dialogue/MatchBegin", "BeginMatch");
	soundManaging.SetEvent("event:/Dialogue/Options", "Options");
	soundManaging.SetEvent("event:/Dialogue/Player1_LastKill", "P1_LastKill");
	soundManaging.SetEvent("event:/Dialogue/Player1_Leading", "P1_Winning");
	soundManaging.SetEvent("event:/Dialogue/Player1_Win", "P1_Win");
	soundManaging.SetEvent("event:/Dialogue/Player2_LastKill", "P2_LastKill");
	soundManaging.SetEvent("event:/Dialogue/Player2_Leading", "P2_Winning");
	soundManaging.SetEvent("event:/Dialogue/Player2_Win", "P2_Win");
	soundManaging.SetEvent("event:/Dialogue/PlayGame", "Play");
	soundManaging.SetEvent("event:/Dialogue/VolumeAdjust", "VolumeAdjust");

	// Sound Effects
	soundManaging.SetEvent("event:/MenuSFX/pop", "Pop");
	soundManaging.SetEvent("event:/MenuSFX/CD_Drive", "LoadScene");
	soundManaging.SetEvent("event:/GameSFX/Cartoon_Boing", "Boing"); //Not in the game anymore
	soundManaging.SetEvent("event:/GameSFX/Jump", "Jump");
	soundManaging.SetEvent("event:/GameSFX/Death", "Death");
	soundManaging.SetEvent("event:/GameSFX/NormalHit", "NormalHit");
	soundManaging.SetEvent("event:/GameSFX/CriticalHit", "CriticalHit");
	soundManaging.SetEvent("event:/GameSFX/Throw", "Throw");
	soundManaging.SetEvent("event:/GameSFX/WalkSand", "Walk_Sand");
	soundManaging.SetEvent("event:/GameSFX/WalkGrass", "Walk_Grass");
	soundManaging.SetEvent("event:/Music/BackgroundMusic", "Map1Music"); //Not in the game anymore
	soundManaging.SetEvent("event:/Music/FinalKillMusic", "FinalKillMusic");
	soundManaging.SetEvent("event:/Music/GameMusic", "GameMusic");

	std::vector<MenuElement::Sptr> p1OptionItems;
	std::vector<MenuElement::Sptr> p2OptionItems;
	int p1ItemInd = 0;
	int p2ItemInd = 0;
	MenuElement::Sptr p1CurrentElement;
	MenuElement::Sptr p2CurrentElement;

	float selectTimes[2];
	float barSelectTimes[2];

	/*
	soundManaging.LoadSound("Sounds/CD_Drive.wav", "Scene Startup");
	soundManaging.LoadSound("Sounds/Cartoon_Boing.wav", "Jump");
	soundManaging.LoadSound("Sounds/pop.wav", "Pop");
	*/

	//int numEvents1 = soundManaging.GetNumEvents1();
	//int numEvents2 = soundManaging.GetNumEvents2();

	GetLayer<Menu>()->BeginLayer();

	LoadScene(GetLayer<Menu>()->GetScene());

	GetLayer<Menu>()->SetActive(true);

	GetLayer<Menu>()->GetScene()->IsPlaying = true;

	////////////////////////////////////////////////////////////////////////

		// Infinite loop as long as the application is running
	while (_isRunning) {
		// Handle scene switching
		if (_targetScene != nullptr) {
			_HandleSceneChange();
		}

		// Receive events like input and window position/size changes from GLFW
		glfwPollEvents();

		//std::cout << "Non-string: " << numEvents1 << "\nString: " << numEvents2 << '\n';

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


		if (InputEngine::GetKeyState(GLFW_KEY_1) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(1);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_2) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(2);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_3) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(3);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_4) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(4);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_5) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(5);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_6) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(6);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_7) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->ToggleRenderFlag(7);
		}

		else if (InputEngine::GetKeyState(GLFW_KEY_ENTER) == ButtonState::Pressed)
		{
			GetLayer<RenderLayer>()->SetRenderFlags(RenderFlags::None);
		}
		
		if (InputEngine::GetKeyState(GLFW_KEY_GRAVE_ACCENT) == ButtonState::Pressed && GetLayer<DefaultSceneLayer>()->IsActive())
		{
			CurrentScene()->FindObjectByName("Lights")->Get<Light>()->IsEnabled =
				!CurrentScene()->FindObjectByName("Lights")->Get<Light>()->IsEnabled;

			CurrentScene()->FindObjectByName("Light 2")->Get<Light>()->IsEnabled =
				!CurrentScene()->FindObjectByName("Light 2")->Get<Light>()->IsEnabled;

			CurrentScene()->FindObjectByName("Shadow Light")->Get<ShadowCamera>()->IsEnabled =
				!CurrentScene()->FindObjectByName("Shadow Light")->Get<ShadowCamera>()->IsEnabled;
		}

		if (InputEngine::GetKeyState(GLFW_KEY_TAB) == ButtonState::Pressed && GetLayer<DefaultSceneLayer>()->IsActive())
		{
			ShadowCamera::Sptr shadowCam = CurrentScene()->FindObjectByName("Shadow Light")->Get<ShadowCamera>();

			if (shadowCam->NormalBias == 0.0001f)
			{
				shadowCam->NormalBias = 0.0012f;
			}

			else
			{
				shadowCam->NormalBias = 0.0001f;
			}
		}


		//Update the durations of all sounds (to be used to see if a sound has fully been played)
		soundManaging.UpdateSounds(dt);

		ImGuiHelper::StartFrame();

		//If we are on the first frame, then get some references to menu elements
		if (firstFrame)
		{
			soundManaging.PlayEvent("Bootup");
			menuItems.clear();
			optionItems.clear();

			GetLayer<Menu>()->RepositionUI();
			currentElement = _currentScene->FindObjectByName("Play Desert")->Get<MenuElement>();
			currentElement->GrowElement();

			menuItems.push_back(_currentScene->FindObjectByName("Play Desert")->Get<MenuElement>());
			menuItems.push_back(_currentScene->FindObjectByName("Play Jungle")->Get<MenuElement>());
			menuItems.push_back(_currentScene->FindObjectByName("Options Button")->Get<MenuElement>());
			menuItems.push_back(_currentScene->FindObjectByName("Exit Button")->Get<MenuElement>());

			optionItems.push_back(_currentScene->FindObjectByName("Volume Text")->Get<MenuElement>());
			optionItems.push_back(_currentScene->FindObjectByName("Sensitivity Text")->Get<MenuElement>());

			soundManaging.SetListenerObjects(_currentScene->FindObjectByName("MainCam1"), _currentScene->FindObjectByName("MainCam2"));

			firstFrame = false;


			//soundManaging.SetEvent("event:/Footsteps", "footsteps");
		}

		//else 		std::cout << soundManaging.GetStatus() << '\n';

		//If menu screen layer is active
		if (GetLayer<Menu>()->IsActive())
		{
			bool downSelect;
			bool upSelect;
			bool leftSelect;
			bool rightSelect;
			bool confirm;
			bool scoreboard;
			bool back;


			ControllerInput::Sptr menuControl = _currentScene->FindObjectByName("Menu Control")->Get<ControllerInput>();
			if (menuControl->IsValid())
			{
				downSelect = menuControl->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) > 0.2f;
				upSelect = menuControl->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) < -0.2f;
				leftSelect = menuControl->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X) < -0.2f;
				rightSelect = menuControl->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X) > 0.2f;
				confirm = menuControl->GetButtonDown(GLFW_GAMEPAD_BUTTON_A);
				scoreboard = menuControl->GetButtonDown(GLFW_GAMEPAD_BUTTON_BACK);
				back = menuControl->GetButtonDown(GLFW_GAMEPAD_BUTTON_B);

			}

			else
			{
				downSelect = glfwGetKey(_window, GLFW_KEY_DOWN);
				upSelect = glfwGetKey(_window, GLFW_KEY_UP);
				leftSelect = glfwGetKey(_window, GLFW_KEY_LEFT);
				rightSelect = glfwGetKey(_window, GLFW_KEY_RIGHT);
				confirm = glfwGetKey(_window, GLFW_KEY_ENTER);
				scoreboard = glfwGetKey(_window, GLFW_KEY_TAB);
				back = glfwGetKey(_window, GLFW_KEY_ESCAPE);

			}

			if (loading)
			{

				GetLayer<Menu>()->SetActive(false);
				GetLayer<EndScreen>()->SetActive(false);

				if (desert)
				{
					GetLayer<DefaultSceneLayer>()->BeginLayer();

					LoadScene(GetLayer<DefaultSceneLayer>()->GetScene());

					GetLayer<DefaultSceneLayer>()->SetActive(true);

					GetLayer<DefaultSceneLayer>()->GetScene()->IsPlaying = true;
				}
				else
				{
					GetLayer<Menu>()->SetActive(false);
					//Begin the second map (create the scene)
					GetLayer<SecondMap>()->BeginLayer();
					//Load the scene
					LoadScene(GetLayer<SecondMap>()->GetScene());
					//Set the current layer to active
					GetLayer<SecondMap>()->SetActive(true);
					//Start playing
					GetLayer<SecondMap>()->GetScene()->IsPlaying = true;
				}
				soundManaging.StopSounds();

				started = true;
				loading = false;
			}

			else if (options)
			{

				ControllerInput::Sptr thisController = CurrentScene()->FindObjectByName("Menu Control")->Get<ControllerInput>();
				
				if (downSelect && selectTime >= 0.3f)
				{
					currentElement->ShrinkElement();
					currentItemInd++;

					if (currentItemInd >= optionItems.size()) currentItemInd = 0;
					currentElement = optionItems[currentItemInd];

					currentElement->GrowElement();
					selectTime = 0.0f;
				}

				else if (upSelect && selectTime >= 0.3f)
				{
					currentElement->ShrinkElement();
					currentItemInd--;
					if (currentItemInd < 0) currentItemInd = optionItems.size() - 1;

					currentElement = optionItems[currentItemInd];

					currentElement->GrowElement();
					selectTime = 0.0f;
				}

				else selectTime += dt;

				if (back)
				{
					currentElement->ShrinkElement();

					CurrentScene()->FindObjectByName("Play Desert")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Play Jungle")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Options Button")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Exit Button")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Logo")->Get<GuiPanel>()->SetTransparency(1.0f);

					CurrentScene()->FindObjectByName("Volume Text")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Volume Bar")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Volume Selector")->Get<GuiPanel>()->SetTransparency(0.0f);

					CurrentScene()->FindObjectByName("Sensitivity Text")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Bar")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Selector")->Get<GuiPanel>()->SetTransparency(0.0f);

					currentElement = _currentScene->FindObjectByName("Play Desert")->Get<MenuElement>();
					currentElement->GrowElement();
					currentItemInd = 0;

					options = false;
				}

				else if (currentElement == _currentScene->FindObjectByName("Volume Text")->Get<MenuElement>())
				{

					if (leftSelect && barSelectTime >= 0.2f)
					{
						//If sound isn't at 0 yet, reduce it
						//Note: something is weird with computations in the back-end, causing currentVol to have a miniscule decimal added to the end.
						//Therefore, use > 0.00001 instead of > 0
						if (thisSoundInfo.currentVol > 0.00001f)
						{
							thisSoundInfo.currentVol -= 0.1f;

							soundManaging.SetVolume(thisSoundInfo.currentVol);

							soundManaging.PlayEvent("VolumeAdjust");
						}

						barSelectTime = 0.0f;
					}

					else if (rightSelect && barSelectTime >= 0.2f)
					{
						if (thisSoundInfo.currentVol < 1.0f)
						{
							thisSoundInfo.currentVol += 0.1f;

							soundManaging.SetVolume(thisSoundInfo.currentVol);

							soundManaging.PlayEvent("VolumeAdjust");
						}

						barSelectTime = 0.0f;
					}

					else barSelectTime += dt;
				}

				else if (currentElement == _currentScene->FindObjectByName("Sensitivity Text")->Get<MenuElement>())
				{

					if (leftSelect && barSelectTime >= 0.2f)
					{
						//If sound isn't at 0 yet, reduce it
						//Note: something is weird with computations in the back-end, causing currentVol to have a miniscule decimal added to the end.
						//Therefore, use > 0.00001 instead of > 0
						if (thisController->GetSensitivity() > thisController->GetMinSensitivity())
						{
							thisController->SetSensitivity(thisController->GetSensitivity() - 0.2f);

							soundManaging.PlayEvent("Pop");

							currentSensitivity = thisController->GetSensitivity();
						}

						barSelectTime = 0.0f;
					}

					else if (rightSelect && barSelectTime >= 0.2f)
					{
						if (thisController->GetSensitivity() < thisController->GetMaxSensitivity())
						{
							thisController->SetSensitivity(thisController->GetSensitivity() + 0.2f);

							soundManaging.PlayEvent("Pop");

							currentSensitivity = thisController->GetSensitivity();
						}

						barSelectTime = 0.0f;
					}

					else barSelectTime += dt;
				}

				//Lerp between the two ends of the volume bar, with the current volume being the lerp parameter
				float currentLoc = glm::lerp(CurrentScene()->FindObjectByName("Volume Bar")->Get<RectTransform>()->GetMin().x + 10, 
					CurrentScene()->FindObjectByName("Volume Bar")->Get<RectTransform>()->GetMax().x - 10, 
					thisSoundInfo.currentVol);
			
				CurrentScene()->FindObjectByName("Volume Selector")->Get<RectTransform>()->SetMin({ currentLoc - 10, GetWindowSize().y / 8 });
				CurrentScene()->FindObjectByName("Volume Selector")->Get<RectTransform>()->SetMax({ currentLoc + 10, GetWindowSize().y / 4 });

				//Lerp between the two ends of the sensitivity bar, with the current sensitivity being the lerp parameter
				currentLoc = glm::lerp(CurrentScene()->FindObjectByName("Sensitivity Bar")->Get<RectTransform>()->GetMin().x + 10,
					CurrentScene()->FindObjectByName("Sensitivity Bar")->Get<RectTransform>()->GetMax().x - 10,
					(thisController->GetSensitivity() - thisController->GetMinSensitivity()) / (thisController->GetMaxSensitivity() - thisController->GetMinSensitivity()));

				CurrentScene()->FindObjectByName("Sensitivity Selector")->Get<RectTransform>()->SetMin({ currentLoc - 10, 3 * GetWindowSize().y / 8 });
				CurrentScene()->FindObjectByName("Sensitivity Selector")->Get<RectTransform>()->SetMax({ currentLoc + 10, GetWindowSize().y / 2 });
			}

			else if (scoreboard)
			{
				auto scoreCard = scoreFunc.readScores();

				std::cout << scoreCard[0].name << "|" << scoreCard[0].time << std::endl;
				std::cout << scoreCard[1].name << "|" << scoreCard[1].time << std::endl;
				std::cout << scoreCard[2].name << "|" << scoreCard[2].time << std::endl;

				std::string bsp;
				std::cin >> bsp;
			}
			
			else
			{
			
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

				else if (currentElement == _currentScene->FindObjectByName("Play Desert")->Get<MenuElement>() && confirm)
				{

					CurrentScene()->FindObjectByName("Menu BG")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Play Desert")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Play Jungle")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Options Button")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Exit Button")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Logo")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Loading Screen")->Get<GuiPanel>()->SetTransparency(1.0f);

					soundManaging.PlayEvent("Play");
					loading = true;
					desert = true;
					soundManaging.PlayEvent("LoadScene");
				}

				else if (currentElement == _currentScene->FindObjectByName("Play Jungle")->Get<MenuElement>() && confirm)
				{

					CurrentScene()->FindObjectByName("Menu BG")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Play Desert")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Play Jungle")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Options Button")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Exit Button")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Logo")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Loading Screen")->Get<GuiPanel>()->SetTransparency(1.0f);

					soundManaging.PlayEvent("Play");
					loading = true;
					desert = false;
					soundManaging.PlayEvent("LoadScene");
				}

				else if (currentElement == _currentScene->FindObjectByName("Options Button")->Get<MenuElement>() && confirm)
				{
					soundManaging.PlayEvent("Options");
					options = true;

					currentElement->ShrinkElement();

					CurrentScene()->FindObjectByName("Play Desert")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Play Jungle")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Options Button")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Exit Button")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Logo")->Get<GuiPanel>()->SetTransparency(0.0f);

					CurrentScene()->FindObjectByName("Volume Text")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Volume Bar")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Volume Selector")->Get<GuiPanel>()->SetTransparency(1.0f);

					CurrentScene()->FindObjectByName("Sensitivity Text")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Bar")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Selector")->Get<GuiPanel>()->SetTransparency(1.0f);

					currentElement = _currentScene->FindObjectByName("Volume Text")->Get<MenuElement>();
					currentElement->GrowElement();
					currentItemInd = 0;
				}

				else if (exitting)
				{
					exitTime += dt;
					if (exitTime >= 1.5)
						exit(0);
				}

				else if (currentElement == _currentScene->FindObjectByName("Exit Button")->Get<MenuElement>() && confirm)
				{
					soundManaging.PlayEvent("Exit");
					exitting = true;
				}
			
				else selectTime += dt;
			}
		}

		//If end screen layer is active
		else if (GetLayer<EndScreen>()->IsActive())
		{
			if (loading)
			{
				GetLayer<EndScreen>()->SetActive(false);

				GetLayer<Menu>()->BeginLayer();

				LoadScene(GetLayer<Menu>()->GetScene());

				GetLayer<Menu>()->SetActive(true);

				GetLayer<Menu>()->GetScene()->IsPlaying = true;

				soundManaging.StopSounds();

				started = true;
				loading = false;
				firstFrame = true;
			}

			else if (CurrentScene()->FindObjectByName("Menu Control")->Get<ControllerInput>()->IsValid())
			{
				if (CurrentScene()->FindObjectByName("Menu Control")->Get<ControllerInput>()->GetButtonPressed(GLFW_GAMEPAD_BUTTON_BACK))
				{
					std::cin >> playerName;
					scoreFunc.addScore(playerName, timing.TimeSinceSceneLoad());

				}

				else if (CurrentScene()->FindObjectByName("Menu Control")->Get<ControllerInput>()->GetButtonPressed(GLFW_GAMEPAD_BUTTON_START))
				{
					loading = true;
					//soundManaging.PlayEvent("LoadScene");
				}
			}

			else
			{
				if (InputEngine::GetKeyState(GLFW_KEY_ENTER) == ButtonState::Pressed)
				{
					loading = true;
					//soundManaging.PlayEvent("LoadScene");
				}
			}
			
			
		}
		
		else
		{
			GameObject::Sptr player1 = _currentScene->FindObjectByName("Player 1");
			GameObject::Sptr player2 = _currentScene->FindObjectByName("Player 2");
			GameObject::Sptr boomerang1 = _currentScene->FindObjectByName("Boomerang 1");
			GameObject::Sptr boomerang2 = _currentScene->FindObjectByName("Boomerang 2");

			//GameObject::Sptr detachedCam = _currentScene->FindObjectByName("Detached Camera");

			if (started)
			{
				started = false;
				p1OptionItems.clear();
				p2OptionItems.clear();

				//GetLayer<DefaultSceneLayer>()->RepositionUI();
				player1->Get<ControllerInput>()->SetSensitivity(currentSensitivity);
				player2->Get<ControllerInput>()->SetSensitivity(currentSensitivity);

				//InputEngine::SetSensitivity(currentSensitivity);

				p1CurrentElement = _currentScene->FindObjectByName("Sensitivity Text1")->Get<MenuElement>();
				p2CurrentElement = _currentScene->FindObjectByName("Sensitivity Text2")->Get<MenuElement>();

				p1OptionItems.push_back(p1CurrentElement);
				p2OptionItems.push_back(p2CurrentElement);

				p1CurrentElement->GrowElement();
				p2CurrentElement->GrowElement();

				p1ItemInd = 0;
				p2ItemInd = 0;

				selectTimes[0] = 0.3f;
				selectTimes[1] = 0.3f;
				barSelectTimes[0] = 0.2f;
				barSelectTimes[1] = 0.2f;

				soundManaging.PlayEvent("GameMusic");

				soundManaging.SetListenerObjects(player1, player2);

				player1->Get<HealthManager>()->ResetHealth();
				player2->Get<HealthManager>()->ResetHealth();

				player1->Get<ScoreCounter>()->ResetScore();
				player2->Get<ScoreCounter>()->ResetScore();

				soundManaging.PlayEvent("BeginMatch");

			}

			bool pressedPause = false;

			if (player1->Get<ControllerInput>()->IsValid())
			{
				if (player1->Get<ControllerInput>()->GetButtonPressed(GLFW_GAMEPAD_BUTTON_START)) pressedPause = true;
			}

			if (player2->Get<ControllerInput>()->IsValid())
			{
				if (player2->Get<ControllerInput>()->GetButtonPressed(GLFW_GAMEPAD_BUTTON_START)) pressedPause = true;
			}

			if (InputEngine::GetKeyState(GLFW_KEY_ENTER) == ButtonState::Pressed) pressedPause = true;

			if (pressedPause)
			{
				if (paused)
				{
					player1->Get<ControllerInput>()->SetEnabled(true);
					player2->Get<ControllerInput>()->SetEnabled(true);
					InputEngine::SetEnabled(true);
					timing.SetTimeScale(1.0f);
					paused = false;

					//CurrentScene()->FindObjectByName("PauseText")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("PauseBackground")->Get<GuiPanel>()->SetTransparency(0.0f);

					CurrentScene()->FindObjectByName("Sensitivity Text1")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Selector1")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Text2")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Bar2")->Get<GuiPanel>()->SetTransparency(0.0f);
					CurrentScene()->FindObjectByName("Sensitivity Selector2")->Get<GuiPanel>()->SetTransparency(0.0f);
				}

				else
				{
					player1->Get<ControllerInput>()->SetEnabled(false);
					player2->Get<ControllerInput>()->SetEnabled(false);
					InputEngine::SetEnabled(false);
					timing.SetTimeScale(0.0f);
					paused = true;

					//CurrentScene()->FindObjectByName("PauseText")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("PauseBackground")->Get<GuiPanel>()->SetTransparency(0.1f);

					CurrentScene()->FindObjectByName("Sensitivity Text1")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Selector1")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Text2")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Bar2")->Get<GuiPanel>()->SetTransparency(1.0f);
					CurrentScene()->FindObjectByName("Sensitivity Selector2")->Get<GuiPanel>()->SetTransparency(1.0f);
				}
			}

			if (paused)
			{
				bool downSelect[2];
				bool upSelect[2];
				bool leftSelect[2];
				bool rightSelect[2];
				bool confirm[2];

				//Grab p1's controls and find the corresponding values
				ControllerInput::Sptr p1Control = _currentScene->FindObjectByName("Player 1")->Get<ControllerInput>();
				if (p1Control->IsValid())
				{
					downSelect[0] = p1Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) > 0.2f;
					upSelect[0] = p1Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) < -0.2f;
					leftSelect[0] = p1Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X) < -0.2f;
					rightSelect[0] = p1Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X) > 0.2f;
					confirm[0] = p1Control->GetButtonDown(GLFW_GAMEPAD_BUTTON_A);
				}

				else
				{
					downSelect[0] = glfwGetKey(_window, GLFW_KEY_DOWN);
					upSelect[0] = glfwGetKey(_window, GLFW_KEY_UP);
					leftSelect[0] = glfwGetKey(_window, GLFW_KEY_LEFT);
					rightSelect[0] = glfwGetKey(_window, GLFW_KEY_RIGHT);
					confirm[0] = glfwGetKey(_window, GLFW_KEY_ENTER);
				}

				//Grab p2's controls and find the corresponding values
				ControllerInput::Sptr p2Control = _currentScene->FindObjectByName("Player 2")->Get<ControllerInput>();
				if (p2Control->IsValid())
				{
					downSelect[1] = p2Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) > 0.2f;
					upSelect[1] = p2Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y) < -0.2f;
					leftSelect[1] = p2Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X) < -0.2f;
					rightSelect[1] = p2Control->GetAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X) > 0.2f;
					confirm[1] = p2Control->GetButtonDown(GLFW_GAMEPAD_BUTTON_A);
				}

				else
				{
					downSelect[1] = glfwGetKey(_window, GLFW_KEY_DOWN);
					upSelect[1] = glfwGetKey(_window, GLFW_KEY_UP);
					confirm[1] = glfwGetKey(_window, GLFW_KEY_ENTER);
					leftSelect[1] = glfwGetKey(_window, GLFW_KEY_LEFT);
					rightSelect[1] = glfwGetKey(_window, GLFW_KEY_RIGHT);
				}

				//Player 1 pause menu control
				if (downSelect[0] && selectTimes[0] >= 0.3f)
				{
					p1CurrentElement->ShrinkElement();
					p1ItemInd++;

					if (p1ItemInd >= p1OptionItems.size()) p1ItemInd = 0;
					p1CurrentElement = p1OptionItems[p1ItemInd];

					p1CurrentElement->GrowElement();
					selectTimes[0] = 0.0f;
				}

				else if (upSelect[0] && selectTimes[0] >= 0.3f)
				{
					p1CurrentElement->ShrinkElement();
					p1ItemInd--;
					if (p1ItemInd < 0) p1ItemInd = p1OptionItems.size() - 1;

					p1CurrentElement = p1OptionItems[p1ItemInd];

					p1CurrentElement->GrowElement();
					selectTimes[0] = 0.0f;
				}

				else selectTimes[0]++;

				if (p1CurrentElement == _currentScene->FindObjectByName("Sensitivity Text1")->Get<MenuElement>())
				{

					if (leftSelect[0] && barSelectTimes[0] >= 0.2f)
					{
						//If sound isn't at 0 yet, reduce it
						//Note: something is weird with computations in the back-end, causing currentVol to have a miniscule decimal added to the end.
						//Therefore, use > 0.00001 instead of > 0
						if (p1Control->IsValid() && p1Control->GetSensitivity() > p1Control->GetMinSensitivity())
						{
							if (p1Control->IsValid())
							{
								p1Control->SetSensitivity(p1Control->GetSensitivity() - 0.2f);
								currentSensitivity = p1Control->GetSensitivity();
							}

							soundManaging.PlayEvent("Pop");
						}

						else if (!p1Control->IsValid() && InputEngine::GetSensitivity() > InputEngine::GetMinSensitivity())
						{
							InputEngine::SetSensitivity(InputEngine::GetSensitivity() - 0.2f);
							currentSensitivity = InputEngine::GetSensitivity();

							soundManaging.PlayEvent("Pop");
						}

						barSelectTimes[0] = 0.0f;
					}

					else if (rightSelect[0] && barSelectTimes[0] >= 0.2f)
					{
						if (p1Control->IsValid() && p1Control->GetSensitivity() < p1Control->GetMaxSensitivity())
						{
							p1Control->SetSensitivity(p1Control->GetSensitivity() + 0.2f);
							currentSensitivity = p1Control->GetSensitivity();

							soundManaging.PlayEvent("Pop");
						}

						else if (!p1Control->IsValid() && InputEngine::GetSensitivity() < InputEngine::GetMaxSensitivity())
						{
							InputEngine::SetSensitivity(InputEngine::GetSensitivity() + 0.2f);
							currentSensitivity = InputEngine::GetSensitivity();

							soundManaging.PlayEvent("Pop");
						}

						barSelectTimes[0] = 0.0f;
					}

					else barSelectTimes[0] += dt;
				}

				float currentLoc;

				if (p1Control->IsValid())
					//Lerp between the two ends of the volume bar, with the current volume being the lerp parameter
					currentLoc = glm::lerp(CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMin().x + 10,
						CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMax().x - 10,
						(p1Control->GetSensitivity() - p1Control->GetMinSensitivity()) / (p1Control->GetMaxSensitivity() - p1Control->GetMinSensitivity()));

				else
					//Lerp between the two ends of the volume bar, with the current volume being the lerp parameter
					currentLoc = glm::lerp(CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMin().x + 10,
						CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMax().x - 10,
						(InputEngine::GetSensitivity() - InputEngine::GetMinSensitivity()) / (InputEngine::GetMaxSensitivity() - InputEngine::GetMinSensitivity()));

				CurrentScene()->FindObjectByName("Sensitivity Selector1")->Get<RectTransform>()->SetMin({ currentLoc - 10, GetWindowSize().y / 4 });
				CurrentScene()->FindObjectByName("Sensitivity Selector1")->Get<RectTransform>()->SetMax({ currentLoc + 10, GetWindowSize().y / 2 });

				//Player 2 pause menu control
				if (downSelect[1] && selectTimes[1] >= 0.3f)
				{
					p2CurrentElement->ShrinkElement();
					p2ItemInd++;

					if (p2ItemInd >= p2OptionItems.size()) p2ItemInd = 0;
					p2CurrentElement = p2OptionItems[p2ItemInd];

					p2CurrentElement->GrowElement();
					selectTimes[1] = 0.0f;
				}

				else if (upSelect[1] && selectTimes[1] >= 0.3f)
				{
					p2CurrentElement->ShrinkElement();
					p2ItemInd--;
					if (p2ItemInd < 0) p2ItemInd = p2OptionItems.size() - 1;

					p2CurrentElement = p2OptionItems[p1ItemInd];

					p2CurrentElement->GrowElement();
					selectTimes[1] = 0.0f;
				}

				else selectTimes[1]++;

				if (p2CurrentElement == _currentScene->FindObjectByName("Sensitivity Text2")->Get<MenuElement>())
				{

					if (leftSelect[1] && barSelectTimes[1] >= 0.2f)
					{
						//If sound isn't at 0 yet, reduce it
						//Note: something is weird with computations in the back-end, causing currentVol to have a miniscule decimal added to the end.
						//Therefore, use > 0.00001 instead of > 0
						if (p2Control->IsValid() && p2Control->GetSensitivity() > p2Control->GetMinSensitivity())
						{
							if (p2Control->IsValid())
							{
								p2Control->SetSensitivity(p2Control->GetSensitivity() - 0.2f);
								currentSensitivity = p2Control->GetSensitivity();
							}

							soundManaging.PlayEvent("Pop");
						}

						else if (!p2Control->IsValid() && InputEngine::GetSensitivity() > InputEngine::GetMinSensitivity())
						{
							InputEngine::SetSensitivity(InputEngine::GetSensitivity() - 0.2f);
							currentSensitivity = InputEngine::GetSensitivity();

							soundManaging.PlayEvent("Pop");
						}

						barSelectTimes[1] = 0.0f;
					}

					else if (rightSelect[1] && barSelectTimes[1] >= 0.2f)
					{
						if (p2Control->IsValid() && p2Control->GetSensitivity() < p2Control->GetMaxSensitivity())
						{
							p2Control->SetSensitivity(p2Control->GetSensitivity() + 0.2f);
							currentSensitivity = p2Control->GetSensitivity();
						}

						else if (!p2Control->IsValid() && InputEngine::GetSensitivity() < InputEngine::GetMaxSensitivity())
						{
							InputEngine::SetSensitivity(InputEngine::GetSensitivity() + 0.2f);
							currentSensitivity = InputEngine::GetSensitivity();

							soundManaging.PlayEvent("Pop");
						}

						barSelectTimes[1] = 0.0f;
					}

					else barSelectTimes[1] += dt;
				}

				//Lerp between the two ends of the volume bar, with the current volume being the lerp parameter
				if (p2Control->IsValid())
					//Lerp between the two ends of the volume bar, with the current volume being the lerp parameter
					currentLoc = glm::lerp(CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMin().x + 10,
						CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMax().x - 10,
						(p2Control->GetSensitivity() - p2Control->GetMinSensitivity()) / (p2Control->GetMaxSensitivity() - p2Control->GetMinSensitivity()));

				else
					//Lerp between the two ends of the volume bar, with the current volume being the lerp parameter
					currentLoc = glm::lerp(CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMin().x + 10,
						CurrentScene()->FindObjectByName("Sensitivity Bar1")->Get<RectTransform>()->GetMax().x - 10,
						(InputEngine::GetSensitivity() - InputEngine::GetMinSensitivity()) / (InputEngine::GetMaxSensitivity() - InputEngine::GetMinSensitivity()));

				CurrentScene()->FindObjectByName("Sensitivity Selector2")->Get<RectTransform>()->SetMin({ currentLoc - 10, GetWindowSize().y / 4 });
				CurrentScene()->FindObjectByName("Sensitivity Selector2")->Get<RectTransform>()->SetMax({ currentLoc + 10, GetWindowSize().y / 2 });

			}

			//Boomerang Visual indicator!!!
			GameObject::Sptr dBoom1 = _currentScene->FindObjectByName("Display Boomerang 1");
			//dBoom1->SetPosition(glm::vec3(0.6f, -0.1f, -0.7f));
			
			GameObject::Sptr dBoom2 = _currentScene->FindObjectByName("Display Boomerang 2");
			//dBoom2->SetPosition(glm::vec3(0.6f, -0.1f, -0.7f));
			////////////////////Handle some UI stuff/////////////////////////////////
			if (boomerang1->Get<BoomerangBehavior>()->isInactive() == false)
			{
				dBoom1->SetRenderFlag(6);
			}
			else
			{
				dBoom1->SetRenderFlag(0);
			}

			if (boomerang2->Get<BoomerangBehavior>()->isInactive() == false)
			{
				dBoom2->SetRenderFlag(6);
			}
			else
			{
				dBoom2->SetRenderFlag(0);
			}


			GameObject::Sptr p1Health = _currentScene->FindObjectByName("Player1Health");
			GameObject::Sptr p2Health = _currentScene->FindObjectByName("Player2Health");

			GameObject::Sptr p1DamageScreen = _currentScene->FindObjectByName("DamageFlash1");
			GameObject::Sptr p2DamageScreen = _currentScene->FindObjectByName("DamageFlash2");

			//Find the current health of the player, divide it by the maximum health, lerp between the minimum and maximum health bar values using this as the interp. parameter
			p1Health->Get<RectTransform>()->SetMax({ glm::lerp(5.0f, 195.0f,
				player1->Get<HealthManager>()->GetHealth() / player1->Get<HealthManager>()->GetMaxHealth()), GetWindowSize().y - 5.0f });

			p1Health->Get<GuiPanel>()->SetColor(glm::lerp(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
				player1->Get<HealthManager>()->GetHealth() / player1->Get<HealthManager>()->GetMaxHealth()));

			p1DamageScreen->Get<GuiPanel>()->SetColor(glm::vec4(
				p1DamageScreen->Get<GuiPanel>()->GetColor().x,
				p1DamageScreen->Get<GuiPanel>()->GetColor().y,
				p1DamageScreen->Get<GuiPanel>()->GetColor().x,
				player1->Get<HealthManager>()->GetDamageOpacity()));

			p2Health->Get<RectTransform>()->SetMax({ glm::lerp(5.0f, 195.0f,
				player2->Get<HealthManager>()->GetHealth() / player2->Get<HealthManager>()->GetMaxHealth()),  GetWindowSize().y - 5.0f });

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
						<< _currentScene->FindObjectByName("Player " + enemyNum)->Get<ScoreCounter>()->GetScore() << std::endl;

					glm::vec4 enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 1.0f));

					enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 0.0f));

					player1->Get<MorphAnimator>()->ActivateAnim("Die");
					soundManaging.PlayEvent("Death");
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
						<< _currentScene->FindObjectByName("Player " + enemyNum)->Get<ScoreCounter>()->GetScore() << std::endl;

					glm::vec4 enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore()))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 1.0f));

					enemyUIColour = _currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->GetColor();

					_currentScene->FindObjectByName(enemyNum + "-" + std::to_string(enemy->Get<ScoreCounter>()->GetScore() - 1))->Get<GuiPanel>()->SetColor(glm::vec4(
						enemyUIColour.x, enemyUIColour.y, enemyUIColour.z, 0.0f));


					player2->Get<MorphAnimator>()->ActivateAnim("Die");
					soundManaging.PlayEvent("Death");
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

			///////////Activate Last Kill Music///////

			if (LastKill == false)
			{
				if (player1->Get<ScoreCounter>()->GetScore() == 4 || player2->Get<ScoreCounter>()->GetScore() == 4)
				{
					soundManaging.StopSounds();
					soundManaging.PlayEvent("FinalKillMusic");

					if (player1->Get<ScoreCounter>()->GetScore() == 4)
					{
						soundManaging.PlayEvent("P1_LastKill");
					}

					else if (player2->Get<ScoreCounter>()->GetScore() == 4)
					{
						soundManaging.PlayEvent("P2_LastKill");
					}

					LastKill = true;
				}
			}

			///////////Player gets First Blood////////
			
			if (FirstBlood == false)
			{
				if (player1->Get<ScoreCounter>()->GetScore() == 1 || player2->Get<ScoreCounter>()->GetScore() == 1)
				{
					soundManaging.PlayEvent("FirstBlood");
					FirstBlood = true;
				}
			}

			///////////Player overtakes other player//

			if (player1->Get<ScoreCounter>()->GetScore() > player2->Get<ScoreCounter>()->GetScore() && player2->Get<ScoreCounter>()->GetScore() >= 1
				&& player1->Get<ScoreCounter>()->GetScore() < 4)
			{
				if (player1->Get<ScoreCounter>()->GetLead() == false)
				{
					soundManaging.PlayEvent("P1_Winning");
					player1->Get<ScoreCounter>()->SetLead();
					
					if (player2->Get<ScoreCounter>()->GetLead() == true)
					{
						player2->Get<ScoreCounter>()->SetLead();
					}
				}
			}
			if (player2->Get<ScoreCounter>()->GetScore() > player1->Get<ScoreCounter>()->GetScore() && player1->Get<ScoreCounter>()->GetScore() >= 1
				&& player2->Get<ScoreCounter>()->GetScore() < 4)
			{
				if (player2->Get<ScoreCounter>()->GetLead() == false)
				{
					soundManaging.PlayEvent("P2_Winning");
					player2->Get<ScoreCounter>()->SetLead();

					if (player1->Get<ScoreCounter>()->GetLead() == true)
					{
						player1->Get<ScoreCounter>()->SetLead();
					}
				}
			}

			///////////Look for a winner//////////////

			if (player1->Get<ScoreCounter>()->ReachedMaxScore())
			{
				GetLayer<DefaultSceneLayer>()->SetActive(false);

				GetLayer<EndScreen>()->BeginLayer();

				LoadScene(GetLayer<EndScreen>()->GetScene());

				GetLayer<EndScreen>()->SetActive(true);

				GetLayer<EndScreen>()->GetScene()->IsPlaying = true;

				GetLayer<EndScreen>()->GetScene()->FindObjectByName("P1 Wins Text")->Get<GuiPanel>()->SetTransparency(1.0f);

				LastKill = false;
				FirstBlood = false;

				soundManaging.StopSounds();
				soundManaging.PlayEvent("P1_Win");

				//GetLayer<DefaultSceneLayer>()->~DefaultSceneLayer();
			}

			else if (player2->Get<ScoreCounter>()->ReachedMaxScore())
			{
				GetLayer<DefaultSceneLayer>()->SetActive(false);

				GetLayer<EndScreen>()->BeginLayer();

				LoadScene(GetLayer<EndScreen>()->GetScene());

				GetLayer<EndScreen>()->SetActive(true);

				GetLayer<EndScreen>()->GetScene()->IsPlaying = true;

				GetLayer<EndScreen>()->GetScene()->FindObjectByName("P2 Wins Text")->Get<GuiPanel>()->SetTransparency(1.0f);

				LastKill = false;
				FirstBlood = false;

				soundManaging.StopSounds();
				soundManaging.PlayEvent("P2_Win");

				//GetLayer<DefaultSceneLayer>()->~DefaultSceneLayer();
			}

			/////////////////////////////////////////
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
	ResourceManager::RegisterType<Texture1D>();
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<Texture3D>();
	ResourceManager::RegisterType<TextureCube>();
	ResourceManager::RegisterType<ShaderProgram>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();
	ResourceManager::RegisterType<Font>();
	ResourceManager::RegisterType<Framebuffer>();

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

	ComponentManager::RegisterType<ParticleSystem>();
	ComponentManager::RegisterType<Light>();
	ComponentManager::RegisterType<ShadowCamera>();
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

	Framebuffer::Sptr result = nullptr;
	for (const auto& layer : _layers) {
		if (layer->Enabled && *(layer->Overrides & AppLayerFunctions::OnRender)) {
			layer->OnRender(result);
		}
	}
}

void Application::_PostRender() {
	// Note that we use a reverse iterator for post render
	for (auto it = _layers.begin(); it != _layers.end(); it++) {
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

	else if (GetLayer<SecondMap>()->IsActive())
	{
		GetLayer<SecondMap>()->RepositionUI();
	}

	else if (GetLayer<EndScreen>()->IsActive())
	{
		GetLayer<EndScreen>()->RepositionUI();
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

