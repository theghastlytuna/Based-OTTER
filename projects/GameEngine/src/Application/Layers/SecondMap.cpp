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
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});

		ShaderProgram::Sptr animShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/morphAnim.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/animFrag.glsl" }
		});

#pragma endregion
#pragma region loadMeshes

#pragma region EnvironmentalMeshes
		MeshResource::Sptr groundGreyMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/GroundGrey.obj");
		MeshResource::Sptr groundRedMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/GroundRed.obj");
		MeshResource::Sptr groundBlueMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/GroundBlue.obj");

		MeshResource::Sptr northWestMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/North West.obj");
		MeshResource::Sptr southMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/South.obj");
		MeshResource::Sptr eastMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/East.obj");
		MeshResource::Sptr redMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/Red.obj");
		MeshResource::Sptr blueMesh = ResourceManager::CreateAsset <MeshResource>("Jungle/Blue.obj");

		/*
		MeshResource::Sptr groundMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Grass.obj");
		MeshResource::Sptr wallMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Canyon_Walls.obj");
		MeshResource::Sptr healthBaseMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Health_Pick_Up.obj");
		MeshResource::Sptr treeBaseMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Trees.obj");
		MeshResource::Sptr raisedPlatMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Raised_Platorm.obj");
		*/
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

		std::vector<MeshResource::Sptr> boomerangSpin = LoadTargets2(4, "BoomerangAnims/Boomerang_Active_00");
#pragma endregion

#pragma endregion
#pragma region loadTextures

		Texture2D::Sptr    grassTex = ResourceManager::CreateAsset<Texture2D>("textures/Map2/Grass_Texture.png");
		grassTex->SetMinFilter(MinFilter::Unknown);
		grassTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    wallTex = ResourceManager::CreateAsset<Texture2D>("textures/Map2/Canyon_Walls.png");
		wallTex->SetMinFilter(MinFilter::Unknown);
		wallTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    healthTex = ResourceManager::CreateAsset<Texture2D>("textures/Map2/Health_Spot.png");
		healthTex->SetMinFilter(MinFilter::Unknown);
		healthTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    treeTex = ResourceManager::CreateAsset<Texture2D>("textures/Map2/Tree.png");
		treeTex->SetMinFilter(MinFilter::Unknown);
		treeTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   mainCharTex = ResourceManager::CreateAsset<Texture2D>("textures/Char.png");
		mainCharTex->SetMinFilter(MinFilter::Unknown);
		mainCharTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   boomerangTex = ResourceManager::CreateAsset<Texture2D>("textures/boomerwang.png");
		boomerangTex->SetMinFilter(MinFilter::Unknown);
		boomerangTex->SetMagFilter(MagFilter::Nearest);

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

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

		Material::Sptr grassMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			grassMat->Name = "Box";
			grassMat->Set("u_Material.Diffuse", grassTex);
			grassMat->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr wallMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			wallMat->Name = "Box";
			wallMat->Set("u_Material.Diffuse", wallTex);
			wallMat->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr healthMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			healthMat->Name = "Box";
			healthMat->Set("u_Material.Diffuse", healthTex);
			healthMat->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr treeMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			treeMat->Name = "Box";
			treeMat->Set("u_Material.Diffuse", treeTex);
			treeMat->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr mainCharMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			mainCharMaterial->Name = "MainCharacter";
			mainCharMaterial->Set("u_Material.Diffuse", mainCharTex);
			mainCharMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr mainCharMaterial2 = ResourceManager::CreateAsset<Material>(animShader);
		{
			mainCharMaterial2->Name = "MainCharacter2";
			mainCharMaterial2->Set("u_Material.Diffuse", mainCharTex);
			mainCharMaterial2->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr boomerangMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			boomerangMaterial->Name = "Boomerang1";
			boomerangMaterial->Set("u_Material.Diffuse", boomerangTex);
			boomerangMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr boomerangMaterial2 = ResourceManager::CreateAsset<Material>(animShader);
		{
			boomerangMaterial2->Name = "Boomerang2";
			boomerangMaterial2->Set("u_Material.Diffuse", boomerangTex);
			boomerangMaterial2->Set("u_Material.Shininess", 0.1f);
		}

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

#pragma endregion
#pragma region createLights

		/*
		// Create some lights for our scene
		scene->Lights.resize(4);

		scene->Lights[0].Position = glm::vec3(10.0f, 10.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 40.0f;

		scene->Lights[0].Position = glm::vec3(9.0f, 1.0f, 50.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 1000.0f;

		scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[2].Position = glm::vec3(9.0f, 1.0f, 50.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.57f, 0.1f);
		scene->Lights[2].Range = 200.0f;

		scene->Lights[3].Position = glm::vec3(-67.73f, 15.73f, 3.5f);
		scene->Lights[3].Color = glm::vec3(0.81f, 0.62f, 0.61f);
		scene->Lights[3].Range = 200.0f;
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
#pragma region environmentalObjects
#pragma region groundObjs
		GameObject::Sptr MapDaddy = scene->CreateGameObject("Map Daddy"); 

		GameObject::Sptr groundGreyObj = scene->CreateGameObject("Neutral Ground");
		{
			// Set position in the scene
			groundGreyObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			groundGreyObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			groundGreyObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = groundGreyObj->Add<RenderComponent>();
			renderer->SetMesh(groundGreyMesh);
			renderer->SetMaterial(grassMat);

			RigidBody::Sptr physics = groundGreyObj->Add<RigidBody>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			physics->SetMass(0);
			physics->AddCollider(collider);
			MapDaddy->AddChild(groundGreyObj);
		}
		GameObject::Sptr groundRedObj = scene->CreateGameObject("Red Team Ground");
		{
			// Set position in the scene
			groundRedObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			groundRedObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			groundRedObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = groundRedObj->Add<RenderComponent>();
			renderer->SetMesh(groundRedMesh);
			renderer->SetMaterial(grassMat);

			RigidBody::Sptr physics = groundRedObj->Add<RigidBody>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			physics->AddCollider(collider);
			physics->SetMass(0);
			MapDaddy->AddChild(groundRedObj);
		}
		GameObject::Sptr groundBlueObj = scene->CreateGameObject("Blue Team Ground");
		{
			// Set position in the scene
			groundBlueObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			groundBlueObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			groundBlueObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = groundBlueObj->Add<RenderComponent>();
			renderer->SetMesh(groundBlueMesh);
			renderer->SetMaterial(grassMat);

			RigidBody::Sptr physics = groundBlueObj->Add<RigidBody>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			physics->AddCollider(collider);
			physics->SetMass(0);
			MapDaddy->AddChild(groundBlueObj);
		}
#pragma endregion
#pragma region wallObjs
		GameObject::Sptr northWallObj = scene->CreateGameObject("North West Platform Wall");
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
			collider->SetPosition({ -10,15,-11 });
			collider->SetRotation({ 0,45,0 });
			collider->SetScale({ 14,7,1 });
			BoxCollider::Sptr collider2 = BoxCollider::Create();
			collider2->SetPosition({ -10,4,-10 });
			collider2->SetScale({ 11,4.4f,11 });
			physics->AddCollider(collider);
			physics->AddCollider(collider2);
			physics->SetMass(0);
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
			collider->SetPosition(glm::vec3(0, 3.0f, 21.0f));
			collider->SetScale({ 21, 24, 1 });
			physics->AddCollider(collider);
			SphereCollider::Sptr coolider2 = SphereCollider::Create();
			coolider2->SetPosition({ -17, 3.75, 22 });
			coolider2->SetRadius(5);
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
			collider->SetPosition({ 19,10,0 });
			collider->SetScale({ 1,20,25 });
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


	/*
		GameObject::Sptr groundObj = scene->CreateGameObject("Ground");
		{
			// Set position in the scene
			groundObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			groundObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			groundObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = groundObj->Add<RenderComponent>();
			renderer->SetMesh(groundMesh);
			renderer->SetMaterial(grassMat);

			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(110.0f, 50.0f, 1.0f));
			collider->SetPosition({ 0,-0.8,-1 });
			collider->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = groundObj->Add<RigidBody>();
			physics->AddCollider(collider);

		}

		GameObject::Sptr wallObj = scene->CreateGameObject("Walls");
		{
			// Set position in the scene
			wallObj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			wallObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			wallObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = wallObj->Add<RenderComponent>();
			renderer->SetMesh(wallMesh);
			renderer->SetMaterial(wallMat);
		}

		GameObject::Sptr healthBase = scene->CreateGameObject("Health Base");
		{
			// Set position in the scene
			healthBase->SetPosition(glm::vec3(17.0f, -7.5f, 2.6f));
			healthBase->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			healthBase->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = healthBase->Add<RenderComponent>();
			renderer->SetMesh(healthBaseMesh);
			renderer->SetMaterial(healthMat);
		}

		GameObject::Sptr treeBase1 = scene->CreateGameObject("Tree Base 1");
		{
			// Set position in the scene
			treeBase1->SetPosition(glm::vec3(-23.0f, 0.0f, 11.0f));
			treeBase1->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			treeBase1->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = treeBase1->Add<RenderComponent>();
			renderer->SetMesh(treeBaseMesh);
			renderer->SetMaterial(treeMat);
		}

		GameObject::Sptr treeBase2 = scene->CreateGameObject("Tree Base 2");
		{
			// Set position in the scene
			treeBase2->SetPosition(glm::vec3(7.0f, 28.0f, 11.0f));
			treeBase2->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			treeBase2->SetRotation(glm::vec3(90.0f, 0.0f, -90.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = treeBase2->Add<RenderComponent>();
			renderer->SetMesh(treeBaseMesh);
			renderer->SetMaterial(treeMat);
		}

		GameObject::Sptr raisedPlatObj = scene->CreateGameObject("Raised Platform");
		{
			// Set position in the scene
			raisedPlatObj->SetPosition(glm::vec3(11.0f, -7.0f, 6.0f));
			raisedPlatObj->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			raisedPlatObj->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = raisedPlatObj->Add<RenderComponent>();
			renderer->SetMesh(raisedPlatMesh);
			renderer->SetMaterial(treeMat);
		}
		*/
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

			boomerang->Add<MorphAnimator>();
			boomerang->Get<MorphAnimator>()->AddClip(boomerangSpin, 0.1, "Spin");

			boomerang->Get<MorphAnimator>()->ActivateAnim("spin");

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

			boomerang2->Add<MorphAnimator>();
			boomerang2->Get<MorphAnimator>()->AddClip(boomerangSpin, 0.1, "Spin");

			boomerang2->Get<MorphAnimator>()->ActivateAnim("spin");
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
			renderer->SetMaterial(displayBoomerangMaterial1);

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
			renderer->SetMaterial(displayBoomerangMaterial1);

			detachedCam2->AddChild(displayBoomerang2);
			displayBoomerang2->SetPosition(displacement);
		}

#pragma endregion
#pragma region UI
		GameObject::Sptr healthbar1 = scene->CreateGameObject("HealthBackPanel1");
		{
			healthbar1->SetRenderFlag(1);

			RectTransform::Sptr transform = healthbar1->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ 200, 50 });

			GuiPanel::Sptr canPanel = healthbar1->Add<GuiPanel>();
			canPanel->SetColor(glm::vec4(0.467f, 0.498f, 0.549f, 1.0f));

			GameObject::Sptr subPanel1 = scene->CreateGameObject("Player1Health");
			{
				subPanel1->SetRenderFlag(1);
				RectTransform::Sptr transform = subPanel1->Add<RectTransform>();
				transform->SetMin({ 5, 5 });
				transform->SetMax({ 195, 45 });

				GuiPanel::Sptr panel = subPanel1->Add<GuiPanel>();
				panel->SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}

			healthbar1->AddChild(subPanel1);
		}

		GameObject::Sptr healthbar2 = scene->CreateGameObject("HealthBackPanel2");
		{
			healthbar2->SetRenderFlag(2);

			RectTransform::Sptr transform = healthbar2->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ 200, 50 });

			GuiPanel::Sptr canPanel = healthbar2->Add<GuiPanel>();
			canPanel->SetColor(glm::vec4(0.467f, 0.498f, 0.549f, 1.0f));

			GameObject::Sptr subPanel2 = scene->CreateGameObject("Player2Health");
			{
				subPanel2->SetRenderFlag(2);
				RectTransform::Sptr transform = subPanel2->Add<RectTransform>();
				transform->SetMin({ 5, 5 });
				transform->SetMax({ 195, 45 });

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
			transform->SetMin({ app.GetWindowSize().x, 5 });
			transform->SetMax({ app.GetWindowSize().x - 20, 100 });

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
			transform->SetMin({ 2 * app.GetWindowSize().x - 280, 5 });
			transform->SetMax({ 2 * app.GetWindowSize().x - 100, 100 });

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/sensitivityText.png"));

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeBar.png"));

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelect.png"));

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/sensitivityText.png"));

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeBar.png"));

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelect.png"));

			canPanel->SetTransparency(0.0f);
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

	//Reposition the elements
	crosshair->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
	crosshair->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });
	crosshair2->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
	crosshair2->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });
	killUI->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 195 });
	killUI->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y });
	killUI2->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 195 });
	killUI2->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y });

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
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMin({ 30, app.GetWindowSize().y - 130 });
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMax({ 160, app.GetWindowSize().y });

		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMin({ 30, app.GetWindowSize().y - 130 });
		app.CurrentScene()->FindObjectByName("2-" + std::to_string(i))->Get<RectTransform>()->SetMax({ 160, app.GetWindowSize().y });

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