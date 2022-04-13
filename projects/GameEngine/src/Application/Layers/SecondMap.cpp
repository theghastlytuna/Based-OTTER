#include "SecondMap.h"

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
#include "Graphics/Textures/Texture1D.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/Texture3D.h"
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
#include "Gameplay/Components/Light.h"

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
#include "Gameplay/Components/ParticleSystem.h"

std::vector<Gameplay::MeshResource::Sptr>LoadTargets2(int numTargets, std::string path)
{
	std::vector<Gameplay::MeshResource::Sptr> tempVec;
	for (int i = 0; i < numTargets; i++)
	{
		tempVec.push_back(ResourceManager::CreateAsset<Gameplay::MeshResource>(path + std::to_string(i) + ".obj"));
	}

	return tempVec;
}

SecondMap::SecondMap() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

SecondMap::~SecondMap() = default;

void SecondMap::OnAppLoad(const nlohmann::json& config) {
	//_CreateScene();
}

void SecondMap::BeginLayer()
{
	_CreateScene();
}

void SecondMap::_CreateScene() {

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

#pragma region shaderCompilation

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});

		ShaderProgram::Sptr animShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/morphAnim.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});

#pragma endregion
#pragma region loadMeshes

#pragma region EnvironmentalMeshes

		MeshResource::Sptr northWestMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/North West.obj");
		MeshResource::Sptr nWestDirtMesh = ResourceManager::CreateAsset<MeshResource>("Jungle/North West Dirt.obj");
		MeshResource::Sptr southMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/South.obj");
		MeshResource::Sptr eastMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/East.obj");
		MeshResource::Sptr redMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/Red.obj");
		MeshResource::Sptr blueMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/Blue.obj");
		MeshResource::Sptr groundMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/Ground.obj");

		MeshResource::Sptr leavesMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/leaves.obj");
		MeshResource::Sptr redTreeMesh = ResourceManager::CreateAsset<MeshResource>("Jungle/Red_Tree.obj");
		MeshResource::Sptr blueTreeMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/Blue_Tree.obj");
		MeshResource::Sptr treePlatsMesh = ResourceManager::CreateAsset<MeshResource>("Jungle/Tree Platforms.obj");

		MeshResource::Sptr raisedPlatMesh = ResourceManager::CreateAsset<MeshResource>("Jungle/Raised Platform.obj");

#pragma endregion
#pragma region CharacterMeshes
		MeshResource::Sptr mainCharMesh = ResourceManager::CreateAsset<MeshResource>("mainChar.obj");
		MeshResource::Sptr mainCharMesh2 = ResourceManager::CreateAsset<MeshResource>("mainChar.obj");
		MeshResource::Sptr boomerangMesh = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");
		MeshResource::Sptr displayBoomerangMesh = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");

		std::vector<MeshResource::Sptr> mainIdle = LoadTargets2(3, "MainCharacterAnims/Idle/Char_Idle_00");

		std::vector<MeshResource::Sptr> mainWalk = LoadTargets2(5, "MainCharacterAnims/Walk/Char_Walk_00");

		std::vector<MeshResource::Sptr> mainRun = LoadTargets2(5, "MainCharacterAnims/Run/Char_Run_00");

		std::vector<MeshResource::Sptr> mainJump = LoadTargets2(3, "MainCharacterAnims/Jump/Char_Jump_00");

		std::vector<MeshResource::Sptr> mainDeath = LoadTargets2(4, "MainCharacterAnims/Death/Char_Death_00");

		std::vector<MeshResource::Sptr> mainAttack = LoadTargets2(5, "MainCharacterAnims/Attack/Char_Throw_00");

		//std::vector<MeshResource::Sptr> boomerangSpin = LoadTargets2(4, "BoomerangAnims/Boomerang_Active_00");
#pragma endregion

#pragma endregion
#pragma region loadTextures

		Texture2D::Sptr    grassTex = ResourceManager::CreateAsset<Texture2D>("Jungle/Grass.png");
		grassTex->SetMinFilter(MinFilter::Unknown);
		grassTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    dirtTex = ResourceManager::CreateAsset<Texture2D>("Jungle/Dirt.png");
		dirtTex->SetMinFilter(MinFilter::Unknown);
		dirtTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    platformTex = ResourceManager::CreateAsset<Texture2D>("Jungle/Platform.png");
		platformTex->SetMinFilter(MinFilter::Unknown);
		platformTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    wallTex = ResourceManager::CreateAsset<Texture2D>("Jungle/Walls.png");
		wallTex->SetMinFilter(MinFilter::Unknown);
		wallTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    treeTex = ResourceManager::CreateAsset<Texture2D>("Jungle/Tree.png");
		treeTex->SetMinFilter(MinFilter::Unknown);
		treeTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   mainCharTex = ResourceManager::CreateAsset<Texture2D>("textures/Char.png");
		mainCharTex->SetMinFilter(MinFilter::Unknown);
		mainCharTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   boomerangTex = ResourceManager::CreateAsset<Texture2D>("textures/boomerwang.png");
		boomerangTex->SetMinFilter(MinFilter::Unknown);
		boomerangTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   leafTex = ResourceManager::CreateAsset<Texture2D>("Jungle/Leaf2.png");
		leafTex->SetMinFilter(MinFilter::Unknown);
		leafTex->SetMagFilter(MagFilter::Nearest);

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion
#pragma region Skybox

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap);
		scene->SetSkyboxShader(skyboxShader);
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

#pragma endregion
#pragma region createMaterials

		Material::Sptr platformMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			platformMat->Name = "platformMat";
			platformMat->Set("u_Material.AlbedoMap", platformTex);
			platformMat->Set("u_Material.Shininess", 0.1f);
			platformMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr dirtMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			dirtMat->Name = "platformMat";
			dirtMat->Set("u_Material.AlbedoMap", dirtTex);
			dirtMat->Set("u_Material.Shininess", 0.1f);
			dirtMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr grassMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			grassMat->Name = "grassMat";
			grassMat->Set("u_Material.AlbedoMap", grassTex);
			grassMat->Set("u_Material.Shininess", 0.1f);
			grassMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr leafMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			leafMat->Name = "leafMat";
			leafMat->Set("u_Material.AlbedoMap", leafTex);
			leafMat->Set("u_Material.Shininess", 0.1f);
			leafMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr wallMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			wallMat->Name = "wallMat";
			wallMat->Set("u_Material.AlbedoMap", wallTex);
			wallMat->Set("u_Material.Shininess", 0.1f);
			wallMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr treeMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			treeMat->Name = "treeMat";
			treeMat->Set("u_Material.AlbedoMap", treeTex);
			treeMat->Set("u_Material.Shininess", 0.1f);
			treeMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr mainCharMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			mainCharMaterial->Name = "mainCharMaterial";
			mainCharMaterial->Set("u_Material.AlbedoMap", mainCharTex);
			mainCharMaterial->Set("u_Material.Shininess", 0.f);
			mainCharMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr mainCharMaterial2 = ResourceManager::CreateAsset<Material>(animShader);
		{
			mainCharMaterial2->Name = "mainCharMaterial2";
			mainCharMaterial2->Set("u_Material.AlbedoMap", mainCharTex);
			mainCharMaterial2->Set("u_Material.Shininess", 0.f);
			mainCharMaterial2->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr boomerangMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boomerangMaterial->Name = "boomerangMaterial";
			boomerangMaterial->Set("u_Material.AlbedoMap", boomerangTex);
			boomerangMaterial->Set("u_Material.Shininess", 0.1f);
			boomerangMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr boomerangMaterial2 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boomerangMaterial2->Name = "boomerangMaterial2";
			boomerangMaterial2->Set("u_Material.AlbedoMap", boomerangTex);
			boomerangMaterial2->Set("u_Material.Shininess", 0.1f);
			boomerangMaterial2->Set("u_Material.NormalMap", normalMapDefault);
		}
		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particles4.png", 2, 2);

		/*
		Material::Sptr displayBoomerangMaterial1 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			displayBoomerangMaterial1->Name = "Display Boomerang1";
			displayBoomerangMaterial1->Set("u_Material.Diffuse", boomerangTex);
			displayBoomerangMaterial1->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr displayBoomerangMaterial2 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			displayBoomerangMaterial2->Name = "Display Boomerang1";
			displayBoomerangMaterial2->Set("u_Material.Diffuse", boomerangTex);
			displayBoomerangMaterial2->Set("u_Material.Shininess", 0.1f);
		}
		*/

#pragma endregion
#pragma region createGameObjects
#pragma region Cameras

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPosition(glm::vec3(5.0f));
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();
			
			Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!

			scene->WorldCamera = cam;
			//scene->WorldCamera->ResizeWindow(1920, 500);
		}

		//Set up the scene's camera 
		GameObject::Sptr camera2 = scene->CreateGameObject("Main Camera 2");
		{
			camera2->SetPosition(glm::vec3(5.0f));
			camera2->LookAt(glm::vec3(0.0f));

			Camera::Sptr cam = camera2->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera! 

			//cam->ResizeWindow(1920, 500);
		}

		GameObject::Sptr detachedCam = scene->CreateGameObject("Detached Camera");
		{
			ControllerInput::Sptr controller1 = detachedCam->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);

			detachedCam->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam->Add<Camera>();
			scene->PlayerCamera = cam;
			scene->MainCamera = cam;
			scene->MainCamera->ResizeWindow(32, 9);
			scene->PlayerCamera->ResizeWindow(32, 9);
			scene->MainCamera->SetFovDegrees(75);
			scene->PlayerCamera->SetFovDegrees(75);
		}

		GameObject::Sptr detachedCam2 = scene->CreateGameObject("Detached Camera 2");
		{
			ControllerInput::Sptr controller2 = detachedCam2->Add<ControllerInput>();
			controller2->SetController(GLFW_JOYSTICK_2);

			detachedCam2->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam2->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam2->Add<Camera>();
			scene->PlayerCamera2 = cam;
			scene->MainCamera2 = cam;
			scene->MainCamera2->ResizeWindow(32, 9);
			scene->PlayerCamera2->ResizeWindow(32, 9);
			scene->MainCamera2->SetFovDegrees(75);
			scene->PlayerCamera2->SetFovDegrees(75);
		}

#pragma endregion
#pragma region Lights
		GameObject::Sptr light = scene->CreateGameObject("Lights");
		{
			light->SetPosition(glm::vec3(0.f, 0.f, 20.f));

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetColor(glm::vec3(1.0f));
			lightComponent->SetRadius(500.f);
			lightComponent->SetIntensity(5.f);
		}

		GameObject::Sptr light2 = scene->CreateGameObject("Light 2");
		{
			light2->SetPosition(glm::vec3(50.f, 0.f, 20.f));

			Light::Sptr lightComponent = light2->Add<Light>();
			lightComponent->SetColor(glm::vec3(1.0f));
			lightComponent->SetRadius(500.f);
			lightComponent->SetIntensity(5.f);
		}
#pragma endregion

#pragma region environmentalObjects
#pragma region groundObjs
		GameObject::Sptr MapDaddy = scene->CreateGameObject("Map Daddy"); 

		GameObject::Sptr groundBlueObj = scene->CreateGameObject("Ground");
		{
			// Set position in the scene
			groundBlueObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			groundBlueObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			groundBlueObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = groundBlueObj->Add<RenderComponent>();
			renderer->SetMesh(groundMesh);
			renderer->SetMaterial(grassMat);

			RigidBody::Sptr physics = groundBlueObj->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0.5f,0 });
			collider->SetScale({ 100,1,100 });
			physics->AddCollider(collider);
			physics->SetMass(0);
			MapDaddy->AddChild(groundBlueObj);

			TriggerVolume::Sptr volume = groundBlueObj->Add<TriggerVolume>();
			BoxCollider::Sptr collider2 = BoxCollider::Create();
			collider2->SetPosition({ 0,0.5f,0 });
			collider2->SetScale({ 100,1,100 });
			volume->AddCollider(collider2);
			groundBlueObj->Add<TriggerVolumeEnterBehaviour>();
		}
#pragma endregion
#pragma region wallObjs
		GameObject::Sptr northWallObj = scene->CreateGameObject("Ground");
		{
			// Set position in the scene
			northWallObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			northWallObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			northWallObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = northWallObj->Add<RenderComponent>();
			renderer->SetMesh(northWestMesh);
			renderer->SetMaterial(wallMat);

			RigidBody::Sptr physics = northWallObj->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ -15, 5, -15 });
			collider->SetScale({ 16, 6, 16 });
			physics->AddCollider(collider);
			BoxCollider::Sptr collider2 = BoxCollider::Create();
			collider2->SetPosition({ -14, 20, -14 });
			collider2->SetRotation({ 0,45,0 });
			collider2->SetScale({ 20, 10, 1 });
			physics->AddCollider(collider2);
			SphereCollider::Sptr collider3 = SphereCollider::Create();
			collider3->SetPosition({ -22,17,-10 });
			collider3->SetRadius(6);
			physics->SetMass(0);

			TriggerVolume::Sptr volume = northWallObj->Add<TriggerVolume>();
			BoxCollider::Sptr acollider = BoxCollider::Create();
			acollider->SetPosition({ -15, 5, -15 });
			acollider->SetScale({ 16, 6, 16 });
			volume->AddCollider(acollider);
			northWallObj->Add<TriggerVolumeEnterBehaviour>();
			
			MapDaddy->AddChild(northWallObj);
		}
		GameObject::Sptr southWallObj = scene->CreateGameObject("South Wall");
		{
			// Set position in the scene
			southWallObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			southWallObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			southWallObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = southWallObj->Add<RenderComponent>();
			renderer->SetMesh(southMesh);
			renderer->SetMaterial(wallMat);

			RigidBody::Sptr physics = southWallObj->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,20,29.89f });
			collider->SetScale({ 30,17,1 });
			SphereCollider::Sptr collider2 = SphereCollider::Create();
			collider2->SetPosition({ -22,3,29.89 });
			collider2->SetRadius(6);
			physics->AddCollider(collider);
			physics->AddCollider(collider2);
			physics->SetMass(0);
			MapDaddy->AddChild(southWallObj);
		}
		GameObject::Sptr eastWallObj = scene->CreateGameObject("East Wall");
		{
			// Set position in the scene
			eastWallObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			eastWallObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			eastWallObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = eastWallObj->Add<RenderComponent>();
			renderer->SetMesh(eastMesh);
			renderer->SetMaterial(wallMat);

			RigidBody::Sptr physics = eastWallObj->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 28, 20, 0 });
			collider->SetRotation({ 0,5,0 });
			collider->SetScale({ 1, 17, 30 });
			physics->AddCollider(collider);
			physics->SetMass(0);
			MapDaddy->AddChild(eastWallObj);
		}
		GameObject::Sptr blueWallObj = scene->CreateGameObject("Blue Team Wall");
		{
			// Set position in the scene
			blueWallObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			blueWallObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			blueWallObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = blueWallObj->Add<RenderComponent>();
			renderer->SetMesh(blueMesh);
			renderer->SetMaterial(wallMat);

			RigidBody::Sptr physics = blueWallObj->Add<RigidBody>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			physics->AddCollider(collider);
			physics->SetMass(0);
			MapDaddy->AddChild(blueWallObj);
		}
		GameObject::Sptr redWallObj = scene->CreateGameObject("Red Team Wall");
		{
			// Set position in the scene
			redWallObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			redWallObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			redWallObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = redWallObj->Add<RenderComponent>();
			renderer->SetMesh(redMesh);
			renderer->SetMaterial(wallMat);

			RigidBody::Sptr physics = redWallObj->Add<RigidBody>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			physics->AddCollider(collider);
			physics->SetMass(0);
			MapDaddy->AddChild(redWallObj);
		}
#pragma endregion
#pragma region Obstacles

		GameObject::Sptr blueTreeObj = scene->CreateGameObject("Blue Tree");
		{
			// Set position in the scene
			blueTreeObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			blueTreeObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			blueTreeObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = blueTreeObj->Add<RenderComponent>();
			renderer->SetMesh(blueTreeMesh);
			renderer->SetMaterial(treeMat);

			MapDaddy->AddChild(blueTreeObj);

			RigidBody::Sptr physics = blueTreeObj->Add<RigidBody>();
			physics->SetMass(0);
			CylinderCollider::Sptr collider = CylinderCollider::Create();
			collider->SetPosition({ 13, 12, -12 });
			collider->SetRotation({ -90, 0,0 });
			collider->SetScale({ 8,1,10 });
			physics->AddCollider(collider);
		}

		GameObject::Sptr redTreeObj = scene->CreateGameObject("Red Tree");
		{
			// Set position in the scene
			redTreeObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			redTreeObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			redTreeObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = redTreeObj->Add<RenderComponent>();
			renderer->SetMesh(redTreeMesh);
			renderer->SetMaterial(treeMat);

			MapDaddy->AddChild(redTreeObj);

			RigidBody::Sptr physics = redTreeObj->Add<RigidBody>();
			physics->SetMass(0);
			CylinderCollider::Sptr collider = CylinderCollider::Create();
			collider->SetPosition({ -17, 12, 14 });
			collider->SetRotation({ -90, 0,0 });
			collider->SetScale({ 8,1,10 });
			physics->AddCollider(collider);
		}

		GameObject::Sptr leaves = scene->CreateGameObject("Leaves");
		{
			leaves->SetPosition({ 0,0,-2 });
			leaves->SetScale({ 1,1,1 });
			leaves->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = leaves->Add<RenderComponent>();
			renderer->SetMesh(leavesMesh);
			renderer->SetMaterial(leafMat);

			MapDaddy->AddChild(leaves);

			RigidBody::Sptr physics = leaves->Add<RigidBody>();
		}

		GameObject::Sptr treePlats = scene->CreateGameObject("Ground");
		{
			treePlats->SetPosition({ 7,13,8 });
			treePlats->SetScale({ 1,1,1 });
			treePlats->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = treePlats->Add<RenderComponent>();
			renderer->SetMesh(treePlatsMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(treePlats);

			RigidBody::Sptr physics = treePlats->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0,0 });
			collider->SetScale({ 2.4,0.3,2.4 });
			physics->AddCollider(collider);

			BoxCollider::Sptr vcollider = BoxCollider::Create();
			vcollider->SetPosition({ 0,0,0 });
			vcollider->SetScale({ 2.4,0.3,2.4 });
			TriggerVolume::Sptr volume = treePlats->Add<TriggerVolume>();
			volume->AddCollider(vcollider);
			treePlats->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr treePlats2 = scene->CreateGameObject("Ground");
		{
			treePlats2->SetPosition({ 13,19,6 });
			treePlats2->SetScale({ 1,1,1 });
			treePlats2->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = treePlats2->Add<RenderComponent>();
			renderer->SetMesh(treePlatsMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(treePlats2);

			RigidBody::Sptr physics = treePlats2->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0,0 });
			collider->SetScale({ 2.4,0.3,2.4 });
			physics->AddCollider(collider);

			BoxCollider::Sptr vcollider = BoxCollider::Create();
			vcollider->SetPosition({ 0,0,0 });
			vcollider->SetScale({ 2.4,0.3,2.4 });
			TriggerVolume::Sptr volume = treePlats2->Add<TriggerVolume>();
			volume->AddCollider(vcollider);
			treePlats2->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr treePlats3 = scene->CreateGameObject("Ground");
		{
			treePlats3->SetPosition({ 19,13,4 });
			treePlats3->SetScale({ 1,1,1 });
			treePlats3->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = treePlats3->Add<RenderComponent>();
			renderer->SetMesh(treePlatsMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(treePlats3);

			RigidBody::Sptr physics = treePlats3->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0,0 });
			collider->SetScale({ 2.4,0.3,2.4 });
			physics->AddCollider(collider);

			BoxCollider::Sptr vcollider = BoxCollider::Create();
			vcollider->SetPosition({ 0,0,0 });
			vcollider->SetScale({ 2.4,0.3,2.4 });
			TriggerVolume::Sptr volume = treePlats3->Add<TriggerVolume>();
			volume->AddCollider(vcollider);
			treePlats3->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr treePlats4 = scene->CreateGameObject("Ground");
		{
			treePlats4->SetPosition({ -17,-9,8 });
			treePlats4->SetScale({ 1,1,1 });
			treePlats4->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = treePlats4->Add<RenderComponent>();
			renderer->SetMesh(treePlatsMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(treePlats4);

			RigidBody::Sptr physics = treePlats4->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0,0 });
			collider->SetScale({ 2.4,0.3,2.4 });
			physics->AddCollider(collider);

			BoxCollider::Sptr vcollider = BoxCollider::Create();
			vcollider->SetPosition({ 0,0,0 });
			vcollider->SetScale({ 2.4,0.3,2.4 });
			TriggerVolume::Sptr volume = treePlats4->Add<TriggerVolume>();
			volume->AddCollider(vcollider);
			treePlats4->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr treePlats5 = scene->CreateGameObject("Ground");
		{
			treePlats5->SetPosition({ -23,-14,6 });
			treePlats5->SetScale({ 1,1,1 });
			treePlats5->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = treePlats5->Add<RenderComponent>();
			renderer->SetMesh(treePlatsMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(treePlats5);

			RigidBody::Sptr physics = treePlats5->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0,0 });
			collider->SetScale({ 2.4,0.3,2.4 });
			physics->AddCollider(collider);

			BoxCollider::Sptr vcollider = BoxCollider::Create();
			vcollider->SetPosition({ 0,0,0 });
			vcollider->SetScale({ 2.4,0.3,2.4 });
			TriggerVolume::Sptr volume = treePlats5->Add<TriggerVolume>();
			volume->AddCollider(vcollider);
			treePlats5->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr treePlats6 = scene->CreateGameObject("Ground");
		{
			treePlats6->SetPosition({ -18,-20,4 });
			treePlats6->SetScale({ 1,1,1 });
			treePlats6->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = treePlats6->Add<RenderComponent>();
			renderer->SetMesh(treePlatsMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(treePlats6);

			RigidBody::Sptr physics = treePlats6->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 0,0,0 });
			collider->SetScale({ 2.4,0.3,2.4 });
			physics->AddCollider(collider);

			BoxCollider::Sptr vcollider = BoxCollider::Create();
			vcollider->SetPosition({ 0,0,0 });
			vcollider->SetScale({ 2.4,0.3,2.4 });
			TriggerVolume::Sptr volume = treePlats6->Add<TriggerVolume>();
			volume->AddCollider(vcollider);
			treePlats6->Add<TriggerVolumeEnterBehaviour>();
		}
		/*GameObject::Sptr raisedPlat = scene->CreateGameObject("Ground");
		{
			raisedPlat->SetPosition({ 0,0,0 });
			raisedPlat->SetScale({ 1,1,1 });
			raisedPlat->SetRotation({ 90,0,0 });

			RenderComponent::Sptr renderer = raisedPlat->Add<RenderComponent>();
			renderer->SetMesh(raisedPlatMesh);
			renderer->SetMaterial(platformMat);

			MapDaddy->AddChild(raisedPlat);

			RigidBody::Sptr physics = raisedPlat->Add<RigidBody>();
			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetPosition({ 16,13,20 });
			collider->SetScale({ 11,0.3,11 });
			physics->AddCollider(collider);
			TriggerVolume::Sptr volume = raisedPlat->Add<TriggerVolume>();
			BoxCollider::Sptr collider4 = BoxCollider::Create();
			collider4->SetPosition({ 16,13,20 });
			collider4->SetScale({ 11,0.3,11 });
			volume->AddCollider(collider4);
			raisedPlat->Add<TriggerVolumeEnterBehaviour>();
		}*/

#pragma endregion
#pragma region Entities
		GameObject::Sptr player1 = scene->CreateGameObject("Player 1");
		{
			ControllerInput::Sptr controller1 = player1->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);

			player1->SetPosition(glm::vec3(2.f, -2.f, 4.f));
			player1->SetRotation(glm::vec3(0.f, 90.f, 0.f));

			RenderComponent::Sptr renderer = player1->Add<RenderComponent>();
			renderer->SetMesh(mainCharMesh);
			renderer->SetMaterial(mainCharMaterial);
			player1->SetRenderFlag(2);

			player1->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			RigidBody::Sptr physics = player1->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(BoxCollider::Create(glm::vec3(0.4f, 1.2f, 0.4f)))->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			physics->SetAngularFactor(glm::vec3(0.f));
			physics->SetLinearDamping(0.6f);
			physics->SetMass(1.f);

			PlayerControl::Sptr controller = player1->Add<PlayerControl>();

			controller->Map2 = true;

			JumpBehaviour::Sptr jumping = player1->Add<JumpBehaviour>();

			player1->AddChild(detachedCam);

			// Only add an animator when you have a clip to add.
			MorphAnimator::Sptr animator = player1->Add<MorphAnimator>();

			//Add the clips
			animator->AddClip(mainIdle, 0.8f, "Idle");
			animator->AddClip(mainWalk, 0.4f, "Walk");
			animator->AddClip(mainRun, 0.25f, "Run");
			animator->AddClip(mainAttack, 0.1f, "Attack");
			animator->AddClip(mainDeath, 0.5f, "Die");
			animator->AddClip(mainJump, 0.1f, "Jump");

			//Make sure to always activate an animation at the time of creation (usually idle)
			animator->ActivateAnim("Idle");

			player1->Add<HealthManager>();

			player1->Add<ScoreCounter>();

			player1->SetRenderFlag(2);
		}

		GameObject::Sptr player2 = scene->CreateGameObject("Player 2");
		{
			ControllerInput::Sptr controller2 = player2->Add<ControllerInput>();
			controller2->SetController(GLFW_JOYSTICK_2);

			player2->SetPosition(glm::vec3(10.f, 5.f, 4.f));

			RenderComponent::Sptr renderer = player2->Add<RenderComponent>();
			renderer->SetMesh(mainCharMesh2);
			renderer->SetMaterial(mainCharMaterial2);
			player2->SetRenderFlag(1);

			player2->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			RigidBody::Sptr physics = player2->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(BoxCollider::Create(glm::vec3(0.4f, 1.2f, 0.4f)))->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			physics->SetAngularFactor(glm::vec3(0.f));
			physics->SetLinearDamping(0.6f);
			physics->SetMass(1.f);

			PlayerControl::Sptr controller = player2->Add<PlayerControl>();

			controller->Map2 = true;

			JumpBehaviour::Sptr jumping = player2->Add<JumpBehaviour>();

			player2->AddChild(detachedCam2);

			//Only add an animator when you have a clip to add.
			MorphAnimator::Sptr animator = player2->Add<MorphAnimator>();

			//Add the clips
			animator->AddClip(mainIdle, 0.8f, "Idle");
			animator->AddClip(mainWalk, 0.4f, "Walk");
			animator->AddClip(mainRun, 0.25f, "Run");
			animator->AddClip(mainAttack, 0.1f, "Attack");
			animator->AddClip(mainDeath, 0.5f, "Die");
			animator->AddClip(mainJump, 0.1f, "Jump");

			//Make sure to always activate an animation at the time of creation (usually idle)
			animator->ActivateAnim("Idle");

			player2->Add<HealthManager>();

			player2->Add<ScoreCounter>();

			player2->SetRenderFlag(1);
		}

		GameObject::Sptr boomerang = scene->CreateGameObject("Boomerang 1");
		{
			// Set position in the scene
			boomerang->SetPosition(glm::vec3(0.0f, 0.0f, -100.0f));
			boomerang->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = boomerang->Add<RenderComponent>();
			renderer->SetMesh(boomerangMesh);
			renderer->SetMaterial(boomerangMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(0.3f, 0.3f, 0.1f));
			//collider->SetExtents(glm::vec3(0.8f, 0.8f, 0.8f));

			BoxCollider::Sptr colliderTrigger = BoxCollider::Create();
			colliderTrigger->SetScale(glm::vec3(0.4f, 0.4f, 0.2f));

			TriggerVolume::Sptr volume = boomerang->Add<TriggerVolume>();
			boomerang->Add<TriggerVolumeEnterBehaviour>();
			volume->AddCollider(colliderTrigger);

			RigidBody::Sptr physics = boomerang->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(collider);

			boomerang->Add<BoomerangBehavior>();

		}

		GameObject::Sptr boomerang2 = scene->CreateGameObject("Boomerang 2");
		{
			// Set position in the scene
			boomerang2->SetPosition(glm::vec3(0.0f, 0.0f, -100.0f));
			boomerang2->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = boomerang2->Add<RenderComponent>();
			renderer->SetMesh(boomerangMesh);
			renderer->SetMaterial(boomerangMaterial2);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(0.3f, 0.3f, 0.1f));

			BoxCollider::Sptr colliderTrigger = BoxCollider::Create();
			colliderTrigger->SetScale(glm::vec3(0.4f, 0.4f, 0.2f));

			TriggerVolume::Sptr volume = boomerang2->Add<TriggerVolume>();
			boomerang2->Add<TriggerVolumeEnterBehaviour>();
			volume->AddCollider(colliderTrigger);

			RigidBody::Sptr physics = boomerang2->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(collider);

			boomerang2->Add<BoomerangBehavior>();
		}

		GameObject::Sptr displayBoomerang1 = scene->CreateGameObject("Display Boomerang 1");
		{
			glm::vec3 displacement = (glm::vec3(0.6f, -0.1f, -0.7f));
			// Set position in the scene
			displayBoomerang1->SetPosition(glm::vec3(player1->GetPosition()));
			displayBoomerang1->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));
			displayBoomerang1->SetRotation(glm::vec3(-178.0f, -15.0f, -110.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = displayBoomerang1->Add<RenderComponent>();
			renderer->SetMesh(displayBoomerangMesh);
			renderer->SetMaterial(boomerangMaterial);

			detachedCam->AddChild(displayBoomerang1);
			displayBoomerang1->SetPosition(displacement);
		}

		GameObject::Sptr displayBoomerang2 = scene->CreateGameObject("Display Boomerang 2");
		{


			glm::vec3 displacement = (glm::vec3(0.6f, -0.1f, -0.7f));
			// Set position in the scene

			displayBoomerang2->SetPosition(glm::vec3(player2->GetPosition()));
			displayBoomerang2->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));
			displayBoomerang2->SetRotation(glm::vec3(-178.0f, -15.0f, -110.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = displayBoomerang2->Add<RenderComponent>();
			renderer->SetMesh(displayBoomerangMesh);
			renderer->SetMaterial(boomerangMaterial2);

			detachedCam2->AddChild(displayBoomerang2);
			displayBoomerang2->SetPosition(displacement);
		}

		GameObject::Sptr tBoomSmoke1 = scene->CreateGameObject("tBoomSmoke1");
		{
			tBoomSmoke1->SetPosition(glm::vec3(0.f, 0.f, 0.f));
			//particles->SetRenderFlag(1);

			ParticleSystem::Sptr particleManager = tBoomSmoke1->Add<ParticleSystem>();
			particleManager->AddFlag(1);
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 4;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.9f, 0.8f, 0.8f, 1.0f);

			emitter.SphereEmitterData.Timer = 1.0f / 100.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.SphereEmitterData.Radius = 1.0f;
			emitter.SphereEmitterData.SizeRange = { 0.2f, 0.7f };

			emitter.ConeEmitterData.Velocity = glm::vec3(0, 0, 2.f);
			emitter.ConeEmitterData.Angle = 0.f;
			emitter.ConeEmitterData.Timer = 1.0f / 100.0f;
			emitter.ConeEmitterData.SizeRange = { 0.5, 1.5 };
			emitter.ConeEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.ConeEmitterData.LifeRange = { 0.5f, 2.0f };


			particleManager->AddEmitter(emitter);

			boomerang->AddChild(tBoomSmoke1);
		}

		GameObject::Sptr bBoomSmoke1 = scene->CreateGameObject("bBoomSmoke1");
		{
			tBoomSmoke1->SetPosition(glm::vec3(0.f, 0.f, 0.f));
			//particles->SetRenderFlag(1);

			ParticleSystem::Sptr particleManager = bBoomSmoke1->Add<ParticleSystem>();
			particleManager->AddFlag(2);
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 4;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.9f, 0.8f, 0.8f, 1.0f);

			emitter.SphereEmitterData.Timer = 1.0f / 100.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.SphereEmitterData.Radius = 1.0f;
			emitter.SphereEmitterData.SizeRange = { 0.2f, 0.7f };


			emitter.ConeEmitterData.Velocity = glm::vec3(0, 0, 2.f);
			emitter.ConeEmitterData.Angle = 0.f;
			emitter.ConeEmitterData.Timer = 1.0f / 100.0f;
			emitter.ConeEmitterData.SizeRange = { 0.5, 1.5 };
			emitter.ConeEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.ConeEmitterData.LifeRange = { 0.5f, 2.0f };;


			particleManager->AddEmitter(emitter);

			boomerang->AddChild(bBoomSmoke1);
		}

		GameObject::Sptr tBoomSmoke2 = scene->CreateGameObject("tBoomSmoke2");
		{
			tBoomSmoke2->SetPosition(glm::vec3(0.f, 0.f, 0.f));
			//particles->SetRenderFlag(1);

			ParticleSystem::Sptr particleManager = tBoomSmoke2->Add<ParticleSystem>();
			particleManager->AddFlag(1);
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.48f, 0.46f, 0.72f, 1.0f);

			emitter.SphereEmitterData.Timer = 1.0f / 100.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.SphereEmitterData.Radius = 1.0f;
			emitter.SphereEmitterData.SizeRange = { 0.2f, 0.7f };


			emitter.ConeEmitterData.Velocity = glm::vec3(0, 0, 2.f);
			emitter.ConeEmitterData.Angle = 0.f;
			emitter.ConeEmitterData.Timer = 1.0f / 100.0f;
			emitter.ConeEmitterData.SizeRange = { 0.5, 1.5 };
			emitter.ConeEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.ConeEmitterData.LifeRange = { 0.5f, 2.0f };


			particleManager->AddEmitter(emitter);

			boomerang2->AddChild(tBoomSmoke2);
		}

		GameObject::Sptr bBoomSmoke2 = scene->CreateGameObject("bBoomSmoke2");
		{
			bBoomSmoke2->SetPosition(glm::vec3(0.f, 0.f, 0.f));
			//particles->SetRenderFlag(1);

			ParticleSystem::Sptr particleManager = bBoomSmoke2->Add<ParticleSystem>();
			particleManager->AddFlag(2);
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.48f, 0.46f, 0.72f, 1.0f);

			emitter.SphereEmitterData.Timer = 1.0f / 100.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.SphereEmitterData.Radius = 1.0f;
			emitter.SphereEmitterData.SizeRange = { 0.2f, 0.7f };


			emitter.ConeEmitterData.Velocity = glm::vec3(0, 0, 2.f);
			emitter.ConeEmitterData.Angle = 0.f;
			emitter.ConeEmitterData.Timer = 1.0f / 100.0f;
			emitter.ConeEmitterData.SizeRange = { 0.5, 1.5 };
			emitter.ConeEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.ConeEmitterData.LifeRange = { 0.5f, 2.0f };


			particleManager->AddEmitter(emitter);

			boomerang2->AddChild(bBoomSmoke2);
		}

#pragma endregion
#pragma region UI
		GameObject::Sptr healthbar1 = scene->CreateGameObject("HealthBackPanel1");
		{
			healthbar1->SetRenderFlag(1);

			RectTransform::Sptr transform = healthbar1->Add<RectTransform>();
			transform->SetMin({ 0, app.GetWindowSize().y - 50 });
			transform->SetMax({ 200, app.GetWindowSize().y });

			GuiPanel::Sptr canPanel = healthbar1->Add<GuiPanel>();
			canPanel->SetColor(glm::vec4(0.467f, 0.498f, 0.549f, 1.0f));

			GameObject::Sptr subPanel1 = scene->CreateGameObject("Player1Health");
			{
				subPanel1->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel1->Add<RectTransform>();
				transform->SetMin({ 5, app.GetWindowSize().y - 45 });
				transform->SetMax({ 195, app.GetWindowSize().y - 5 });

				GuiPanel::Sptr panel = subPanel1->Add<GuiPanel>();
				panel->SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}

			healthbar1->AddChild(subPanel1);
		}

		GameObject::Sptr healthbar2 = scene->CreateGameObject("HealthBackPanel2");
		{
			healthbar2->SetRenderFlag(2);

			RectTransform::Sptr transform = healthbar2->Add<RectTransform>();
			transform->SetMin({ 0, app.GetWindowSize().y - 50 });
			transform->SetMax({ 200, app.GetWindowSize().y });

			GuiPanel::Sptr canPanel = healthbar2->Add<GuiPanel>();
			canPanel->SetColor(glm::vec4(0.467f, 0.498f, 0.549f, 1.0f));

			GameObject::Sptr subPanel2 = scene->CreateGameObject("Player2Health");
			{
				subPanel2->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel2->Add<RectTransform>();
				transform->SetMin({ 5, app.GetWindowSize().y - 45 });
				transform->SetMax({ 195, app.GetWindowSize().y - 5 });

				GuiPanel::Sptr panel = subPanel2->Add<GuiPanel>();
				panel->SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}

			healthbar2->AddChild(subPanel2);
		}

		GameObject::Sptr damageFlash1 = scene->CreateGameObject("DamageFlash1");
		{
			damageFlash1->SetRenderFlag(1);

			RectTransform::Sptr transform = damageFlash1->Add<RectTransform>();
			transform->SetMin({ -10, -10 });
			transform->SetMax({ 10000, 10000 });

			GuiPanel::Sptr canPanel = damageFlash1->Add<GuiPanel>();
			canPanel->SetColor(glm::vec4(1.f, 1.f, 1.f, 0.f));
		}

		GameObject::Sptr damageFlash2 = scene->CreateGameObject("DamageFlash2");
		{
			damageFlash2->SetRenderFlag(2);

			RectTransform::Sptr transform = damageFlash2->Add<RectTransform>();
			transform->SetMin({ -10, -10 });
			transform->SetMax({ 10000, 10000 });

			GuiPanel::Sptr canPanel = damageFlash2->Add<GuiPanel>();
			canPanel->SetColor(glm::vec4(1.f, 1.f, 1.f, 0.f));
		}

		GameObject::Sptr crosshairs = scene->CreateGameObject("Crosshairs");
		{
			RectTransform::Sptr transform = crosshairs->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
			transform->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });

			crosshairs->SetRenderFlag(1);

			GuiPanel::Sptr panel = crosshairs->Add<GuiPanel>();
			panel->SetBorderRadius(4);
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/CrossHairs.png"));
		}

		GameObject::Sptr crosshairs2 = scene->CreateGameObject("Crosshairs 2");
		{
			RectTransform::Sptr transform = crosshairs2->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
			transform->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });

			crosshairs2->SetRenderFlag(2);

			GuiPanel::Sptr panel = crosshairs2->Add<GuiPanel>();
			panel->SetBorderRadius(4);
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/CrossHairs.png"));
		}

		GameObject::Sptr scoreCounter1 = scene->CreateGameObject("Score Counter 1"); /// HERE!!!!!
		{
			scoreCounter1->SetRenderFlag(1);

			RectTransform::Sptr transform = scoreCounter1->Add<RectTransform>();
			transform->SetMin({ 0, app.GetWindowSize().y - 240 });
			transform->SetMax({ 200, app.GetWindowSize().y - 45 });

			GuiPanel::Sptr panel = scoreCounter1->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/ScoreUI.png"));

			GameObject::Sptr subPanel1 = scene->CreateGameObject("1-0");
			{
				subPanel1->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel1->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel1->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/0.png"));
			}

			GameObject::Sptr subPanel2 = scene->CreateGameObject("1-1");
			{
				subPanel2->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel2->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel2->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/1.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel3 = scene->CreateGameObject("1-2");
			{
				subPanel3->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel3->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel3->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/2.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel4 = scene->CreateGameObject("1-3");
			{
				subPanel4->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel4->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel4->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/3.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel5 = scene->CreateGameObject("1-4");
			{
				subPanel5->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel5->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel5->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/4.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel6 = scene->CreateGameObject("1-5");
			{
				subPanel6->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel6->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel6->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/5.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel7 = scene->CreateGameObject("1-6");
			{
				subPanel7->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel7->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel7->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/6.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel8 = scene->CreateGameObject("1-7");
			{
				subPanel8->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel8->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel8->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/7.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel9 = scene->CreateGameObject("1-8");
			{
				subPanel9->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel9->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel9->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/8.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel10 = scene->CreateGameObject("1-9");
			{
				subPanel10->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel10->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel10->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/9.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}
		}

		GameObject::Sptr scoreCounter2 = scene->CreateGameObject("Score Counter 2");
		{
			scoreCounter2->SetRenderFlag(2);

			RectTransform::Sptr transform = scoreCounter2->Add<RectTransform>();
			transform->SetMin({ 0, app.GetWindowSize().y - 240 });
			transform->SetMax({ 200, app.GetWindowSize().y - 45 });

			GuiPanel::Sptr panel = scoreCounter2->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/ScoreUI.png"));

			GameObject::Sptr subPanel1 = scene->CreateGameObject("2-0");
			{
				subPanel1->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel1->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel1->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/0.png"));
			}

			GameObject::Sptr subPanel2 = scene->CreateGameObject("2-1");
			{
				subPanel2->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel2->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel2->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/1.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel3 = scene->CreateGameObject("2-2");
			{
				subPanel3->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel3->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel3->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/2.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel4 = scene->CreateGameObject("2-3");
			{
				subPanel4->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel4->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel4->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/3.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel5 = scene->CreateGameObject("2-4");
			{
				subPanel5->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel5->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel5->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/4.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel6 = scene->CreateGameObject("2-5");
			{
				subPanel6->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel6->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel6->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/5.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel7 = scene->CreateGameObject("2-6");
			{
				subPanel7->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel7->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel7->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/6.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel8 = scene->CreateGameObject("2-7");
			{
				subPanel8->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel8->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel8->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/7.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel9 = scene->CreateGameObject("2-8");
			{
				subPanel9->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel9->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel9->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/8.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}

			GameObject::Sptr subPanel10 = scene->CreateGameObject("2-9");
			{
				subPanel10->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel10->Add<RectTransform>();
				transform->SetMin({ 2 * app.GetWindowSize().x - 85, 10 });
				transform->SetMax({ 2 * app.GetWindowSize().x - 10, 95 });

				GuiPanel::Sptr panel = subPanel10->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/UI/9.png"));
				panel->SetColor(glm::vec4(panel->GetColor().x, panel->GetColor().y, panel->GetColor().z, 0.0f));
			}
		}

		GameObject::Sptr pauseMenu = scene->CreateGameObject("PauseBackground");
		{

			RectTransform::Sptr transform = pauseMenu->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

			GuiPanel::Sptr panel = pauseMenu->Add<GuiPanel>();
			panel->SetColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

			GameObject::Sptr subPanel2 = scene->CreateGameObject("PauseText");
			{
				RectTransform::Sptr transform = subPanel2->Add<RectTransform>();
				transform->SetMin({ 0, 0 });
				transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

				GuiPanel::Sptr panel = subPanel2->Add<GuiPanel>();
				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/pauseTextTemp.png"));

				panel->SetColor(glm::vec4(
					panel->GetColor().x,
					panel->GetColor().y,
					panel->GetColor().z,
					0.0f));

				subPanel2->SetRenderFlag(5);
			}

			pauseMenu->AddChild(subPanel2);
			pauseMenu->SetRenderFlag(5);
		}

		GameObject::Sptr sensText1 = scene->CreateGameObject("Sensitivity Text1");
		{
			sensText1->SetRenderFlag(1);

			RectTransform::Sptr transform = sensText1->Add<RectTransform>();
			transform->SetMin({ 120, app.GetWindowSize().y / 4 });
			transform->SetMax({ app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 });

			sensText1->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensText1->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/sensitivityTextFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensBar1 = scene->CreateGameObject("Sensitivity Bar1");
		{
			sensBar1->SetRenderFlag(1);

			RectTransform::Sptr transform = sensBar1->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 4 });
			transform->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 2 });

			//sensBar->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensBar1->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeBarFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensSelector1 = scene->CreateGameObject("Sensitivity Selector1");
		{
			sensSelector1->SetRenderFlag(1);

			RectTransform::Sptr transform = sensSelector1->Add<RectTransform>();
			transform->SetMin({ (sensBar1->Get<RectTransform>()->GetMin().x + sensBar1->Get<RectTransform>()->GetMax().x) / 2 - 10, 400 });
			transform->SetMax({ (sensBar1->Get<RectTransform>()->GetMin().x + sensBar1->Get<RectTransform>()->GetMax().x) / 2 + 10, 2 * app.GetWindowSize().y / 5 });

			//sensSelector->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensSelector1->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelector.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensText2 = scene->CreateGameObject("Sensitivity Text2");
		{
			sensText2->SetRenderFlag(2);

			RectTransform::Sptr transform = sensText2->Add<RectTransform>();
			transform->SetMin({ 120, app.GetWindowSize().y / 4 });
			transform->SetMax({ app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 });

			sensText2->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensText2->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/sensitivityTextFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensBar2 = scene->CreateGameObject("Sensitivity Bar2");
		{
			sensBar2->SetRenderFlag(2);

			RectTransform::Sptr transform = sensBar2->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 4 });
			transform->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 2 });

			//sensBar->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensBar2->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeBarFinal.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr sensSelector2 = scene->CreateGameObject("Sensitivity Selector2");
		{
			sensSelector2->SetRenderFlag(2);

			RectTransform::Sptr transform = sensSelector2->Add<RectTransform>();
			transform->SetMin({ (sensBar2->Get<RectTransform>()->GetMin().x + sensBar2->Get<RectTransform>()->GetMax().x) / 2 - 10, 400 });
			transform->SetMax({ (sensBar2->Get<RectTransform>()->GetMin().x + sensBar2->Get<RectTransform>()->GetMax().x) / 2 + 10, 2 * app.GetWindowSize().y / 5 });

			//sensSelector->Add<MenuElement>();

			GuiPanel::Sptr canPanel = sensSelector2->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelector.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr screenSplitter = scene->CreateGameObject("Screen Splitter");
		{
			screenSplitter->SetRenderFlag(5);
			RectTransform::Sptr transform = screenSplitter->Add<RectTransform>();
			transform->SetMin({ 0, app.GetWindowSize().y / 2 - 1 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y / 2 + 1 });

			GuiPanel::Sptr canPanel = screenSplitter->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/screenSplitter.png"));

			canPanel->SetTransparency(1.0f);
		}
#pragma endregion	
#pragma endregion

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		_scene = scene;
	}
}

Gameplay::Scene::Sptr SecondMap::GetScene()
{
	return _scene;
}

void SecondMap::SetActive(bool active)
{
	_active = active;
}

bool SecondMap::IsActive()
{
	return _active;
}

void SecondMap::RepositionUI()
{
	Application& app = Application::Get();

	//Grab all the UI elements
	Gameplay::GameObject::Sptr crosshair = app.CurrentScene()->FindObjectByName("Crosshairs");
	Gameplay::GameObject::Sptr crosshair2 = app.CurrentScene()->FindObjectByName("Crosshairs 2");
	Gameplay::GameObject::Sptr killUI = app.CurrentScene()->FindObjectByName("Score Counter 1");
	Gameplay::GameObject::Sptr killUI2 = app.CurrentScene()->FindObjectByName("Score Counter 2");

	Gameplay::GameObject::Sptr screenSplitter = app.CurrentScene()->FindObjectByName("Screen Splitter");

	Gameplay::GameObject::Sptr healthBack1 = app.CurrentScene()->FindObjectByName("HealthBackPanel1");
	Gameplay::GameObject::Sptr healthBack2 = app.CurrentScene()->FindObjectByName("HealthBackPanel2");

	Gameplay::GameObject::Sptr healthBar1 = app.CurrentScene()->FindObjectByName("Player1Health");
	Gameplay::GameObject::Sptr healthBar2 = app.CurrentScene()->FindObjectByName("Player2Health");

	//Reposition the elements
	crosshair->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
	crosshair->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });
	crosshair2->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
	crosshair2->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });
	killUI->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 240 });
	killUI->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y - 45 });
	killUI2->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 240 });
	killUI2->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y - 45 });

	screenSplitter->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y / 2 - 3 });
	screenSplitter->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y / 2 + 3 });

	healthBack1->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 50 });
	healthBack1->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y });
	healthBack2->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 50 });
	healthBack2->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y });

	healthBar1->Get<RectTransform>()->SetMin({ 5, app.GetWindowSize().y - 45 });
	healthBar2->Get<RectTransform>()->SetMin({ 5, app.GetWindowSize().y - 45 });

	//Grab pause menu elements
	Gameplay::GameObject::Sptr sensText1 = app.CurrentScene()->FindObjectByName("Sensitivity Text1");
	Gameplay::GameObject::Sptr sensBar1 = app.CurrentScene()->FindObjectByName("Sensitivity Bar1");
	Gameplay::GameObject::Sptr sensText2 = app.CurrentScene()->FindObjectByName("Sensitivity Text2");
	Gameplay::GameObject::Sptr sensBar2 = app.CurrentScene()->FindObjectByName("Sensitivity Bar2");

	sensText1->Get<RectTransform>()->SetMin({ 120, app.GetWindowSize().y / 4 });
	sensText1->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 3,  app.GetWindowSize().y / 2 });
	sensBar1->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 4 });
	sensBar1->Get<RectTransform>()->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 2 });

	sensText2->Get<RectTransform>()->SetMin({ 120, app.GetWindowSize().y / 4 });
	sensText2->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 3,  app.GetWindowSize().y / 2 });
	sensBar2->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2, app.GetWindowSize().y / 4 });
	sensBar2->Get<RectTransform>()->SetMax({ app.GetWindowSize().x - 100,  app.GetWindowSize().y / 2 });

	//Reposition the score UI elements
	for (int i = 0; i < 10; i++)
	{
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMin({ 30, app.GetWindowSize().y - 175 });
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMax({ 160, app.GetWindowSize().y - 45 });

		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMin({ 30, app.GetWindowSize().y - 175 });
		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMax({ 160, app.GetWindowSize().y - 45 });

		//keeping this jus in case
		/*
		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMin({ app.GetWindowSize().x - 85, 10 });
		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMax({ app.GetWindowSize().x - 10, 95 });
		*/
	}

	Gameplay::GameObject::Sptr pauseText = app.CurrentScene()->FindObjectByName("PauseText");
	Gameplay::GameObject::Sptr pauseBG = app.CurrentScene()->FindObjectByName("PauseBackground");

	pauseText->Get<RectTransform>()->SetMin({ 0, 0 });
	pauseText->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

	pauseBG->Get<RectTransform>()->SetMin({ 0, 0 });
	pauseBG->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });
}