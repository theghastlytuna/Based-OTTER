#include "SecondMap.h"

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
/*
std::vector<Gameplay::MeshResource::Sptr> LoadTargets(int numTargets, std::string path)
{
	std::vector<Gameplay::MeshResource::Sptr> tempVec;
	for (int i = 0; i < numTargets; i++)
	{
		tempVec.push_back(ResourceManager::CreateAsset<Gameplay::MeshResource>(path + std::to_string(i) + ".obj"));
	}

	return tempVec;
}
*/

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

#pragma endregion
#pragma region loadMeshes

		MeshResource::Sptr groundMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Grass.obj");
		MeshResource::Sptr wallMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Canyon_Walls.obj");
		MeshResource::Sptr healthBaseMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Health_Pick_Up.obj");
		MeshResource::Sptr treeBaseMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Trees.obj");
		MeshResource::Sptr raisedPlatMesh = ResourceManager::CreateAsset<MeshResource>("stage2Objs/Raised_Platorm.obj");

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

#pragma endregion
#pragma region createLights

		// Create some lights for our scene
		scene->Lights.resize(4);

		/*scene->Lights[0].Position = glm::vec3(10.0f, 10.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 40.0f;*/

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
			scene->WorldCamera->ResizeWindow(1920, 500);
		}

		//Set up the scene's camera 
		GameObject::Sptr camera2 = scene->CreateGameObject("Main Camera 2");
		{
			camera2->SetPosition(glm::vec3(5.0f));
			camera2->LookAt(glm::vec3(0.0f));

			Camera::Sptr cam = camera2->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera! 

			cam->ResizeWindow(1920, 500);
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
			scene->MainCamera->ResizeWindow(1920, 500);
			scene->PlayerCamera->ResizeWindow(1920, 500);
			scene->MainCamera->SetFovDegrees(90);
			scene->PlayerCamera->SetFovDegrees(90);
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
			scene->MainCamera2->ResizeWindow(1920, 500);
			scene->PlayerCamera2->ResizeWindow(1920, 500);
			scene->MainCamera2->SetFovDegrees(90);
			scene->PlayerCamera2->SetFovDegrees(90);
		}

#pragma endregion
		
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

}