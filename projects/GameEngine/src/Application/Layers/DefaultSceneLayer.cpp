#include "DefaultSceneLayer.h"

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

std::vector<MeshResource::Sptr> LoadTargets(int numTargets, std::string path)
{
	std::vector<MeshResource::Sptr> tempVec;
	for (int i = 0; i < numTargets; i++)
	{
		tempVec.push_back(ResourceManager::CreateAsset<MeshResource>(path + std::to_string(i) + ".obj"));
	}

	return tempVec;
}

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json & config) {
	_CreateScene();
}

/// <summary>
/// handles creating or loading the scene
/// </summary>
void DefaultSceneLayer::_CreateScene() {

	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	}
	else {
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

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
		MeshResource::Sptr boiMesh = ResourceManager::CreateAsset<MeshResource>("boi-tpose.obj");
		MeshResource::Sptr catcusMesh = ResourceManager::CreateAsset<MeshResource>("CatcusAnims/Catcus_Idle_001.obj");
		MeshResource::Sptr mainCharMesh = ResourceManager::CreateAsset<MeshResource>("mainChar.obj");
		MeshResource::Sptr mainCharMesh2 = ResourceManager::CreateAsset<MeshResource>("mainChar.obj");
		MeshResource::Sptr boomerangMesh = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");
		MeshResource::Sptr boomerangMesh2 = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");
		MeshResource::Sptr movingPlatMesh = ResourceManager::CreateAsset<MeshResource>("floating_rock.obj");
		MeshResource::Sptr healthPackMesh = ResourceManager::CreateAsset<MeshResource>("HealthPackAnims/healthPack_idle_000.obj");
		MeshResource::Sptr torchMesh = ResourceManager::CreateAsset<MeshResource>("TorchAnims/Torch_Idle_000.obj");

		//Stage Meshes
			//Floors
		MeshResource::Sptr stageCenterFloorMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_center_floor.obj");
		MeshResource::Sptr stageSideFloorMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_side_floors.obj");
		//Walls
		MeshResource::Sptr stageCenterWallsMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_center_walls.obj");
		MeshResource::Sptr stageSideWallsMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_side_walls.obj");
		//Bridge
		MeshResource::Sptr stageBridgeMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_bridge.obj");
		MeshResource::Sptr stagePillarMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_pillar.obj");
		MeshResource::Sptr stagePillar2Mesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/stage_pillar2.obj");

		//Assets

		MeshResource::Sptr barrelMesh = ResourceManager::CreateAsset<MeshResource>("barrel.obj");

		MeshResource::Sptr cactusMesh = ResourceManager::CreateAsset<MeshResource>("cactus_straight.obj");
		MeshResource::Sptr roundCactusMesh = ResourceManager::CreateAsset<MeshResource>("cactus_round.obj");
		MeshResource::Sptr grassMesh = ResourceManager::CreateAsset<MeshResource>("grass.obj");


		//MeshResource::Sptr wiltedTreeMesh = ResourceManager::CreateAsset<MeshResource>("tree_straight.obj");

		//MeshResource::Sptr wiltedTree2Mesh = ResourceManager::CreateAsset<MeshResource>("tree_slanted.obj");


		MeshResource::Sptr tumbleweedMesh = ResourceManager::CreateAsset<MeshResource>("tumbleweed2.obj");
		MeshResource::Sptr smallRocksMesh = ResourceManager::CreateAsset<MeshResource>("small_rocks.obj");

		MeshResource::Sptr bigRocksMesh = ResourceManager::CreateAsset<MeshResource>("big_rocks.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		Texture2D::Sptr	   catcusTex = ResourceManager::CreateAsset<Texture2D>("textures/cattusGood.png");
		Texture2D::Sptr	   mainCharTex = ResourceManager::CreateAsset<Texture2D>("textures/Char.png");
		//Stage Textures
		Texture2D::Sptr    sandTexture = ResourceManager::CreateAsset<Texture2D>("textures/sandFloor.png");
		Texture2D::Sptr    rockFloorTexture = ResourceManager::CreateAsset<Texture2D>("textures/rockyFloor.png");
		Texture2D::Sptr    rockFormationTexture = ResourceManager::CreateAsset<Texture2D>("textures/bigRock.png");
		Texture2D::Sptr    bridgeTexture = ResourceManager::CreateAsset<Texture2D>("textures/woodBridge.png");
		Texture2D::Sptr    rockWallsTexture = ResourceManager::CreateAsset<Texture2D>("textures/walls.png");

		//asset textures

		Texture2D::Sptr    barrelTex = ResourceManager::CreateAsset<Texture2D>("textures/barrelTex.png");
		Texture2D::Sptr	   healthPackTex = ResourceManager::CreateAsset<Texture2D>("textures/vegemiteTex.png");
		Texture2D::Sptr	   torchTex = ResourceManager::CreateAsset<Texture2D>("textures/Torch.png");
		Texture2D::Sptr	   boomerangTex = ResourceManager::CreateAsset<Texture2D>("textures/boomerwang.png");

		Texture2D::Sptr    cactusTex = ResourceManager::CreateAsset<Texture2D>("textures/cactusTex.png");
		Texture2D::Sptr    grassTex = ResourceManager::CreateAsset<Texture2D>("textures/grassTex.png");

		Texture2D::Sptr    greyTreeTex = ResourceManager::CreateAsset<Texture2D>("textures/greyTreeTex.png");
		Texture2D::Sptr    beigeTreeTex = ResourceManager::CreateAsset<Texture2D>("textures/beigeTreeTex.png");

		Texture2D::Sptr    rockTex = ResourceManager::CreateAsset<Texture2D>("textures/rockTex.png");
		Texture2D::Sptr    tumbleweedTex = ResourceManager::CreateAsset<Texture2D>("textures/tumbleweedTex.png");


		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);


		sandTexture->SetMinFilter(MinFilter::Nearest);
		sandTexture->SetMagFilter(MagFilter::Nearest);

		rockFloorTexture->SetMinFilter(MinFilter::Nearest);
		rockFloorTexture->SetMagFilter(MagFilter::Nearest);

		rockFormationTexture->SetMinFilter(MinFilter::Nearest);
		rockFormationTexture->SetMagFilter(MagFilter::Nearest);

		rockWallsTexture->SetMinFilter(MinFilter::Nearest);
		rockWallsTexture->SetMagFilter(MagFilter::Nearest);

		barrelTex->SetMinFilter(MinFilter::Nearest);
		barrelTex->SetMagFilter(MagFilter::Nearest);

		//////////////Loading animation frames////////////////////////
		/*
		std::vector<MeshResource::Sptr> boiFrames;

		for (int i = 0; i < 8; i++)
		{
			boiFrames.push_back(ResourceManager::CreateAsset<MeshResource>("boi-" + std::to_string(i) + ".obj"));
		}
		*/
		std::vector<MeshResource::Sptr> catcusFrames;

		for (int i = 1; i < 9; i++)
		{
			catcusFrames.push_back(ResourceManager::CreateAsset<MeshResource>("CatcusAnims/Catcus_Idle_00" + std::to_string(i) + ".obj"));
		}
		//////////////////////////////////////////////////////////////

		std::vector<MeshResource::Sptr> mainIdle = LoadTargets(3, "MainCharacterAnims/Idle/Char_Idle_00");

		std::vector<MeshResource::Sptr> mainWalk = LoadTargets(5, "MainCharacterAnims/Walk/Char_Walk_00");

		std::vector<MeshResource::Sptr> mainRun = LoadTargets(5, "MainCharacterAnims/Run/Char_Run_00");

		std::vector<MeshResource::Sptr> mainJump = LoadTargets(3, "MainCharacterAnims/Jump/Char_Jump_00");

		std::vector<MeshResource::Sptr> mainDeath = LoadTargets(4, "MainCharacterAnims/Death/Char_Death_00");

		std::vector<MeshResource::Sptr> mainAttack = LoadTargets(5, "MainCharacterAnims/Attack/Char_Throw_00");

		std::vector<MeshResource::Sptr> boomerangSpin = LoadTargets(4, "BoomerangAnims/Boomerang_Active_00");

		std::vector<MeshResource::Sptr> torchIdle = LoadTargets(6, "TorchAnims/Torch_Idle_00");

		std::vector<MeshResource::Sptr> healthPackIdle = LoadTargets(7, "HealthPackAnims/healthPack_idle_00");

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap);
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.Diffuse", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr movingPlatMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			movingPlatMaterial->Name = "MovingPlatform";
			movingPlatMaterial->Set("u_Material.Diffuse", rockTex);
			movingPlatMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr torchMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			torchMaterial->Name = "Torch";
			torchMaterial->Set("u_Material.Diffuse", torchTex);
			torchMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr catcusMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			catcusMaterial->Name = "Catcus";
			catcusMaterial->Set("u_Material.Diffuse", catcusTex);
			catcusMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr healthPackMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			healthPackMaterial->Name = "HealthPack";
			healthPackMaterial->Set("u_Material.Diffuse", healthPackTex);
			healthPackMaterial->Set("u_Material.Shininess", 0.1f);
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

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(reflectiveShader);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.Diffuse", monkeyTex);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.Diffuse", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
		}

		// Our foliage vertex shader material
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.Diffuse", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.Threshold", 0.1f);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.Diffuse", boxTexture);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}

		/////////////Stage materials

		//sand material
		Material::Sptr sandMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			sandMaterial->Name = "Sand";
			sandMaterial->Set("u_Material.Diffuse", sandTexture);
			sandMaterial->Set("u_Material.Shininess", 0.1f);
		}

		//rock floor material
		Material::Sptr rockFloorMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockFloorMaterial->Name = "RockFloor";
			rockFloorMaterial->Set("u_Material.Diffuse", rockFloorTexture);
			rockFloorMaterial->Set("u_Material.Shininess", 0.1f);
		}

		//rock Pillar material
		Material::Sptr rockPillarMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockPillarMaterial->Name = "RockPillar";
			rockPillarMaterial->Set("u_Material.Diffuse", rockFormationTexture);
			rockPillarMaterial->Set("u_Material.Shininess", 0.1f);
		}

		//Wall Material
		Material::Sptr rockWallMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockWallMaterial->Name = "RockWall";
			rockWallMaterial->Set("u_Material.Diffuse", rockWallsTexture);
			rockWallMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr bridgeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			bridgeMaterial->Name = "Bridge";
			bridgeMaterial->Set("u_Material.Diffuse", bridgeTexture);
			bridgeMaterial->Set("u_Material.Shininess", 0.1f);
		}

		/*
		Okay, I pull up, hop out at the after party
		You and all your friends, yeah, they love to get naughty
		Sippin' on that Henn', I know you love that Bacardi (Sonny Digital)
1		942, I take you back in that 'Rari
		*/


		Material::Sptr barrelMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			barrelMaterial->Name = "Barrel";
			barrelMaterial->Set("u_Material.Diffuse", barrelTex);
			barrelMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr cactusMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			cactusMaterial->Name = "Cactus";
			cactusMaterial->Set("u_Material.Diffuse", cactusTex);
			cactusMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			grassMaterial->Name = "Grass";
			grassMaterial->Set("u_Material.Diffuse", grassTex);
			grassMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr greyTreeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			greyTreeMaterial->Name = "Tree Grey";
			greyTreeMaterial->Set("u_Material.Diffuse", greyTreeTex);
			greyTreeMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr beigeTreeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			beigeTreeMaterial->Name = "Tree beige";
			beigeTreeMaterial->Set("u_Material.Diffuse", beigeTreeTex);
			beigeTreeMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockMaterial->Name = "Rock";
			rockMaterial->Set("u_Material.Diffuse", rockTex);
			rockMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr tumbleweedMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			tumbleweedMaterial->Name = "Tumbleweed";
			tumbleweedMaterial->Set("u_Material.Diffuse", tumbleweedTex);
			tumbleweedMaterial->Set("u_Material.Shininess", 0.1f);
		}



		// Create some lights for our scene
		scene->Lights.resize(4);
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


		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
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
		GameObject::Sptr camera2 = scene->CreateGameObject("Main Camera 2");
		{
			camera2->SetPosition(glm::vec3(5.0f));
			camera2->LookAt(glm::vec3(0.0f));

			Camera::Sptr cam = camera2->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera! 
			scene->MainCamera2 = cam;
		}

		GameObject::Sptr detachedCam = scene->CreateGameObject("Detached Camera");
		{
			ControllerInput::Sptr controller1 = detachedCam->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);

			detachedCam->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam->Add<Camera>();
			scene->PlayerCamera = cam;
		}

		GameObject::Sptr player1 = scene->CreateGameObject("Player 1");
		{
			ControllerInput::Sptr controller1 = player1->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);

			player1->SetPosition(glm::vec3(0.f, 0.f, 4.f));
			player1->SetRotation(glm::vec3(0.f, 90.f, 0.f));

			RenderComponent::Sptr renderer = player1->Add<RenderComponent>();
			renderer->SetMesh(mainCharMesh);
			renderer->SetMaterial(mainCharMaterial);

			player1->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			RigidBody::Sptr physics = player1->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(BoxCollider::Create(glm::vec3(0.4f, 1.2f, 0.4f)))->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			physics->SetAngularFactor(glm::vec3(0.f));
			physics->SetLinearDamping(0.9f);

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
		}

		GameObject::Sptr detachedCam2 = scene->CreateGameObject("Detached Camera 2");
		{
			ControllerInput::Sptr controller2 = detachedCam2->Add<ControllerInput>();
			controller2->SetController(GLFW_JOYSTICK_2);

			detachedCam2->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam2->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam2->Add<Camera>();
			scene->PlayerCamera2 = cam;
		}

		GameObject::Sptr player2 = scene->CreateGameObject("Player 2");
		{
			ControllerInput::Sptr controller2 = player2->Add<ControllerInput>();
			controller2->SetController(GLFW_JOYSTICK_2);

			player2->SetPosition(glm::vec3(10.f, 0.f, 4.f));

			RenderComponent::Sptr renderer = player2->Add<RenderComponent>();
			renderer->SetMesh(mainCharMesh2);
			renderer->SetMaterial(mainCharMaterial2);

			player2->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			RigidBody::Sptr physics = player2->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(BoxCollider::Create(glm::vec3(0.4f, 1.2f, 0.4f)))->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			physics->SetAngularFactor(glm::vec3(0.f));
			physics->SetLinearDamping(0.9f);

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
		}

		//Stage Mesh - center floor
		GameObject::Sptr centerGround = scene->CreateGameObject("Center Ground");
		{
			// Set position in the scene
			centerGround->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			centerGround->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			centerGround->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = centerGround->Add<RenderComponent>();
			renderer->SetMesh(stageCenterFloorMesh);
			renderer->SetMaterial(sandMaterial);


			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(110.0f, 110.0f, 1.0f));
			collider->SetPosition({ 0,0,-1 });
			collider->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = centerGround->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider);

			TriggerVolume::Sptr volume = centerGround->Add<TriggerVolume>();
			volume->AddCollider(BoxCollider::Create(glm::vec3(110.0f, 110.0f, 1.0f)))->SetPosition({ 0,0,-1 })->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			centerGround->Add<TriggerVolumeEnterBehaviour>();

		}
		//Stage Mesh - side floors
		GameObject::Sptr sideGround = scene->CreateGameObject("Side Ground");
		{
			// Set position in the scene
			sideGround->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			sideGround->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			sideGround->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = sideGround->Add<RenderComponent>();
			renderer->SetMesh(stageSideFloorMesh);
			renderer->SetMaterial(rockFloorMaterial);

		}

		//Stage Mesh - walls
		GameObject::Sptr centerWalls = scene->CreateGameObject("Center Walls");
		{
			// Set position in the scene
			centerWalls->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			centerWalls->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			centerWalls->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = centerWalls->Add<RenderComponent>();
			renderer->SetMesh(stageCenterWallsMesh);
			renderer->SetMaterial(rockWallMaterial);

			//Collider Co-oridnates:
			BoxCollider::Sptr collider0 = BoxCollider::Create(glm::vec3(1, 23, 14));
			collider0->SetPosition(glm::vec3(-23, 19, -2));
			collider0->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider1 = BoxCollider::Create(glm::vec3(1, 23, 14));
			collider1->SetPosition(glm::vec3(-23, 19.5, -2));
			collider1->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider2 = BoxCollider::Create(glm::vec3(1, 23, 4.5));
			collider2->SetPosition(glm::vec3(-23, 19, -28));
			collider2->SetRotation(glm::vec3(0, 0, 0));


			BoxCollider::Sptr collider3 = BoxCollider::Create(glm::vec3(16, 23, 1));
			collider3->SetPosition(glm::vec3(-7, 19, -33));
			collider3->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider4 = BoxCollider::Create(glm::vec3(1, 23, 10));
			collider4->SetPosition(glm::vec3(8.5, 19, -42));
			collider4->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider5 = BoxCollider::Create(glm::vec3(15, 23, 1));
			collider5->SetPosition(glm::vec3(23, 19, -50));
			collider5->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider6 = BoxCollider::Create(glm::vec3(1, 23, 10));
			collider6->SetPosition(glm::vec3(36, 19, -42));
			collider6->SetRotation(glm::vec3(0, 0, 0));


			BoxCollider::Sptr collider7 = BoxCollider::Create(glm::vec3(1, 12, 8));
			collider7->SetPosition(glm::vec3(-21, 30, 17));
			collider7->SetRotation(glm::vec3(0, 35, 0));

			BoxCollider::Sptr collider8 = BoxCollider::Create(glm::vec3(1, 23, 9.93));
			collider8->SetPosition(glm::vec3(-7.92, 19, 28.05));
			collider8->SetRotation(glm::vec3(0, 60, 0));


			BoxCollider::Sptr collider9 = BoxCollider::Create(glm::vec3(30.74, 23, 1));
			collider9->SetPosition(glm::vec3(29, 19, 32));
			collider9->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider10 = BoxCollider::Create(glm::vec3(1, 23, 4.5));
			collider10->SetPosition(glm::vec3(58, 19, 28));
			collider10->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider11 = BoxCollider::Create(glm::vec3(1, 23, 14));
			collider11->SetPosition(glm::vec3(58, 19, 1.2));
			collider11->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider12 = BoxCollider::Create(glm::vec3(1, 12, 8));
			collider12->SetPosition(glm::vec3(54, 30, -17));
			collider12->SetRotation(glm::vec3(0, 35, 0));

			BoxCollider::Sptr collider13 = BoxCollider::Create(glm::vec3(1, 23, 9.93));
			collider13->SetPosition(glm::vec3(44.1, 19, -28.05));
			collider13->SetRotation(glm::vec3(0, 60, 0));

			RigidBody::Sptr physics = centerWalls->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider0);
			physics->AddCollider(collider1);
			physics->AddCollider(collider2);
			physics->AddCollider(collider3);
			physics->AddCollider(collider4);
			physics->AddCollider(collider5);
			physics->AddCollider(collider6);
			physics->AddCollider(collider7);
			physics->AddCollider(collider8);
			physics->AddCollider(collider9);
			physics->AddCollider(collider10);
			physics->AddCollider(collider11);
			physics->AddCollider(collider12);
			physics->AddCollider(collider13);
			//KILL ME OH MY GOD
		}

		//Stage Mesh - side walls
		GameObject::Sptr sideWalls = scene->CreateGameObject("Side Walls");
		{
			// Set position in the scene
			sideWalls->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			sideWalls->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			sideWalls->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = sideWalls->Add<RenderComponent>();
			renderer->SetMesh(stageSideWallsMesh);
			renderer->SetMaterial(rockWallMaterial);

			//oh god the amount of work i did for this
			BoxCollider::Sptr collider0 = BoxCollider::Create(glm::vec3(8, 10, 1));
			collider0->SetPosition(glm::vec3(-30, 5, -15));
			collider0->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider1 = BoxCollider::Create(glm::vec3(8, 10, 1));
			collider1->SetPosition(glm::vec3(-30, 5, -24));
			collider1->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider2 = BoxCollider::Create(glm::vec3(1, 10, 4));
			collider2->SetPosition(glm::vec3(-37, 5, -27.5));
			collider2->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider3 = BoxCollider::Create(glm::vec3(14.71, 10, 1));
			collider3->SetPosition(glm::vec3(-52, 5, -32.5));
			collider3->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider4 = BoxCollider::Create(glm::vec3(1, 10, 18));
			collider4->SetPosition(glm::vec3(-69.5, 5, -16));
			collider4->SetRotation(glm::vec3(0, -5, 0));

			BoxCollider::Sptr collider5 = BoxCollider::Create(glm::vec3(1, 10, 5));
			collider5->SetPosition(glm::vec3(-74, 5, 6));
			collider5->SetRotation(glm::vec3(0, -38, 0));

			BoxCollider::Sptr collider6 = BoxCollider::Create(glm::vec3(1, 10, 10.5));
			collider6->SetPosition(glm::vec3(-78, 5, 19));
			collider6->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider7 = BoxCollider::Create(glm::vec3(18.02, 10, 1));
			collider7->SetPosition(glm::vec3(-62, 5, 32.5));
			collider7->SetRotation(glm::vec3(0, -7, 0));

			BoxCollider::Sptr collider8 = BoxCollider::Create(glm::vec3(1, 10, 13));
			collider8->SetPosition(glm::vec3(-37, 5, -1));
			collider8->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider9 = BoxCollider::Create(glm::vec3(1, 10, 5.5));
			collider9->SetPosition(glm::vec3(-44, 5, 30));
			collider9->SetRotation(glm::vec3(0, -16, 0));

			BoxCollider::Sptr collider10 = BoxCollider::Create(glm::vec3(5, 0.97, 7.82));
			collider10->SetPosition(glm::vec3(-37.86, 0.13, 19.41));
			collider10->SetRotation(glm::vec3(-5, -20, 18));

			BoxCollider::Sptr collider11 = BoxCollider::Create(glm::vec3(5, 1, 7.9));
			collider11->SetPosition(glm::vec3(-28.9, 3.46, 22.6));
			collider11->SetRotation(glm::vec3(-4, -14, 26));

			BoxCollider::Sptr collider12 = BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17));
			collider12->SetPosition(glm::vec3(-21.420, 3.56, 24.51));
			collider12->SetRotation(glm::vec3(0, -15, 1));

			BoxCollider::Sptr collider13 = BoxCollider::Create(glm::vec3(3.76, 10, 1));
			collider13->SetPosition(glm::vec3(-34.89, 5, 14.05));
			collider13->SetRotation(glm::vec3(0, -42, 0));

			BoxCollider::Sptr collider14 = BoxCollider::Create(glm::vec3(3.46, 10, 1));
			collider14->SetPosition(glm::vec3(-27.83, 5, 18.54));
			collider14->SetRotation(glm::vec3(0, -4, 0));

			BoxCollider::Sptr collider15 = BoxCollider::Create(glm::vec3(2.72, 10, 1));
			collider15->SetPosition(glm::vec3(-23.36, 5, 17.63));
			collider15->SetRotation(glm::vec3(0, 37, 0));

			BoxCollider::Sptr collider16 = BoxCollider::Create(glm::vec3(4.29, 10, 1));
			collider16->SetPosition(glm::vec3(-39.64, 5, 26.94));
			collider16->SetRotation(glm::vec3(0, -17, 0));

			BoxCollider::Sptr collider17 = BoxCollider::Create(glm::vec3(7.44, 10, 1));
			collider17->SetPosition(glm::vec3(-31.03, 5, 28.35));
			collider17->SetRotation(glm::vec3(0, -9, 0));

			BoxCollider::Sptr collider18 = BoxCollider::Create(glm::vec3(4.91, 10, 1));
			collider18->SetPosition(glm::vec3(-19.01, 5, 25.99));
			collider18->SetRotation(glm::vec3(0, 34, 0));

			//Alright, this is uhhhhh hell :)
			//This is the other side
			/// <summary>
			/// The right side. - on the z, + 95 on the x
			/// </summary>
			int d = 35;

			BoxCollider::Sptr collider19 = BoxCollider::Create(glm::vec3(8, 10, 1));
			collider19->SetPosition(glm::vec3((-30 - d) * -1, 5, 15));
			collider19->SetRotation(glm::vec3(0, 0, 0));//

			BoxCollider::Sptr collider20 = BoxCollider::Create(glm::vec3(8, 10, 1));
			collider20->SetPosition(glm::vec3((-30 - d) * -1, 5, 24));
			collider20->SetRotation(glm::vec3(0, 0, 0));//


			BoxCollider::Sptr collider21 = BoxCollider::Create(glm::vec3(1, 10, 4));
			collider21->SetPosition(glm::vec3((-37 - d) * -1, 5, 27.5));
			collider21->SetRotation(glm::vec3(0, 0, 0));//

			BoxCollider::Sptr collider22 = BoxCollider::Create(glm::vec3(14.71, 10, 1));
			collider22->SetPosition(glm::vec3((-52 - d) * -1, 5, 32.5));
			collider22->SetRotation(glm::vec3(0, 0, 0));//

			BoxCollider::Sptr collider23 = BoxCollider::Create(glm::vec3(1, 10, 18));
			collider23->SetPosition(glm::vec3((-69.5 - d) * -1, 5, 16));
			collider23->SetRotation(glm::vec3(0, -5, 0));//

			BoxCollider::Sptr collider24 = BoxCollider::Create(glm::vec3(1, 10, 5));
			collider24->SetPosition(glm::vec3((-74 - d) * -1, 5, -6));
			collider24->SetRotation(glm::vec3(0, -38, 0));//

			BoxCollider::Sptr collider25 = BoxCollider::Create(glm::vec3(1, 10, 10.5));
			collider25->SetPosition(glm::vec3((-78 - d) * -1, 5, -19));
			collider25->SetRotation(glm::vec3(0, 0, 0));//

			BoxCollider::Sptr collider26 = BoxCollider::Create(glm::vec3(18.02, 10, 1));
			collider26->SetPosition(glm::vec3((-62 - d) * -1, 5, -32.5));
			collider26->SetRotation(glm::vec3(0, -7, 0));//

			BoxCollider::Sptr collider27 = BoxCollider::Create(glm::vec3(1, 10, 13));
			collider27->SetPosition(glm::vec3((-37 - d) * -1, 5, 1));
			collider27->SetRotation(glm::vec3(0, 0, 0));//

			BoxCollider::Sptr collider28 = BoxCollider::Create(glm::vec3(1, 10, 5.5));
			collider28->SetPosition(glm::vec3((-44 - d) * -1, 5, -30));
			collider28->SetRotation(glm::vec3(0, -16, 0));//

			BoxCollider::Sptr collider29 = BoxCollider::Create(glm::vec3(5, 0.97, 7.82));
			collider29->SetPosition(glm::vec3((-37.86 - d) * -1, 0.13, -19.41));
			collider29->SetRotation(glm::vec3(5, -20, -18));//// Here!  

			BoxCollider::Sptr collider30 = BoxCollider::Create(glm::vec3(5, 1, 7.9));
			collider30->SetPosition(glm::vec3((-28.9 - d) * -1, 3.46, -22.6));
			collider30->SetRotation(glm::vec3(4, -14, -26));//// Here!

			BoxCollider::Sptr collider31 = BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17));
			collider31->SetPosition(glm::vec3((-21.420 - d) * -1, 3.56, -24.51));
			collider31->SetRotation(glm::vec3(0, -15, -1));//// Here!

			BoxCollider::Sptr collider32 = BoxCollider::Create(glm::vec3(3.76, 10, 1));
			collider32->SetPosition(glm::vec3((-34.89 - d) * -1, 5, -14.05));
			collider32->SetRotation(glm::vec3(0, -42, 0));//

			BoxCollider::Sptr collider33 = BoxCollider::Create(glm::vec3(3.46, 10, 1));
			collider33->SetPosition(glm::vec3((-27.83 - d) * -1, 5, -18.54));
			collider33->SetRotation(glm::vec3(0, -4, 0));//

			BoxCollider::Sptr collider34 = BoxCollider::Create(glm::vec3(2.72, 10, 1));
			collider34->SetPosition(glm::vec3((-23.36 - d) * -1, 5, -17.63));
			collider34->SetRotation(glm::vec3(0, 37, 0));//

			BoxCollider::Sptr collider35 = BoxCollider::Create(glm::vec3(4.29, 10, 1));
			collider35->SetPosition(glm::vec3((-39.64 - d) * -1, 5, -26.94));
			collider35->SetRotation(glm::vec3(0, -17, 0));//

			BoxCollider::Sptr collider36 = BoxCollider::Create(glm::vec3(7.44, 10, 1));
			collider36->SetPosition(glm::vec3((-31.03 - d) * -1, 5, -28.35));
			collider36->SetRotation(glm::vec3(0, -9, 0));//

			BoxCollider::Sptr collider37 = BoxCollider::Create(glm::vec3(4.91, 10, 1));
			collider37->SetPosition(glm::vec3((-19.01 - d) * -1, 5, -25.99));
			collider37->SetRotation(glm::vec3(0, 34, 0));//

			BoxCollider::Sptr collider38 = BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17));
			collider38->SetPosition(glm::vec3(-20.830, 4.1, 23.51));
			collider38->SetRotation(glm::vec3(0, -15, 1));

			BoxCollider::Sptr collider39 = BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17));
			collider39->SetPosition(glm::vec3((-20.830 - d) * -1, 4.1, -23.51));
			collider39->SetRotation(glm::vec3(0, -15, 1));

			/// <summary>
			/// 
			/// </summary>

			RigidBody::Sptr physics = sideWalls->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider0);
			physics->AddCollider(collider1);
			physics->AddCollider(collider2);
			physics->AddCollider(collider3);
			physics->AddCollider(collider4);
			physics->AddCollider(collider5);
			physics->AddCollider(collider6);
			physics->AddCollider(collider7);
			physics->AddCollider(collider8);
			physics->AddCollider(collider9);
			physics->AddCollider(collider10);
			physics->AddCollider(collider11);
			physics->AddCollider(collider12);
			physics->AddCollider(collider13);
			physics->AddCollider(collider14);
			physics->AddCollider(collider15);
			physics->AddCollider(collider16);
			physics->AddCollider(collider17);
			physics->AddCollider(collider18);
			physics->AddCollider(collider38);

			//The other side lul

			physics->AddCollider(collider19);
			physics->AddCollider(collider20);
			physics->AddCollider(collider21);
			physics->AddCollider(collider22);
			physics->AddCollider(collider23);
			physics->AddCollider(collider24);
			physics->AddCollider(collider25);
			physics->AddCollider(collider26);
			physics->AddCollider(collider27);
			physics->AddCollider(collider28);
			physics->AddCollider(collider29);
			physics->AddCollider(collider30);
			physics->AddCollider(collider31);
			physics->AddCollider(collider32);
			physics->AddCollider(collider33);
			physics->AddCollider(collider34);
			physics->AddCollider(collider35);
			physics->AddCollider(collider36);
			physics->AddCollider(collider37);
			//physics->AddCollider(collider39);

			TriggerVolume::Sptr volume = sideWalls->Add<TriggerVolume>();
			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 0.97, 7.82)))->SetPosition({ -37.86, 0.13, 19.41 })->SetRotation(glm::vec3(-5, -20, 18));//
			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 1, 7.9)))->SetPosition({ -28.9, 3.46, 22.6 })->SetRotation(glm::vec3(-4, -14, 26));//
			volume->AddCollider(BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17)))->SetPosition({ -20.83, 4.46, 21.86 })->SetRotation(glm::vec3(6, 36, 10));//

			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 0.97, 7.82)))->SetPosition(glm::vec3((-37.86 - d) * -1, 0.13, -19.41))->SetRotation(glm::vec3(5, -20, -18));//
			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 1, 7.9)))->SetPosition(glm::vec3((-28.9 - d) * -1, 3.46, -22.6))->SetRotation(glm::vec3(4, -14, -26));
			volume->AddCollider(BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17)))->SetPosition(glm::vec3((-20.83 - d) * -1, 4.46, -21.86))->SetRotation(glm::vec3(-6, 36, -10));

			sideWalls->Add<TriggerVolumeEnterBehaviour>();
		}

		//Stage Mesh - bridge
		GameObject::Sptr bridge = scene->CreateGameObject("Bridge Ground");
		{
			// Set position in the scene
			bridge->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			bridge->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			bridge->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = bridge->Add<RenderComponent>();
			renderer->SetMesh(stageBridgeMesh);
			renderer->SetMaterial(bridgeMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(40.4, 0.5, 2.12));
			collider->SetPosition(glm::vec3(17.13, 6.97, -0.7));
			collider->SetRotation(glm::vec3(0, 29, 0));

			RigidBody::Sptr physics = bridge->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider);

			TriggerVolume::Sptr volume = bridge->Add<TriggerVolume>();
			volume->AddCollider(BoxCollider::Create(glm::vec3(40.4, 0.5, 2.12)))->SetPosition({ 17.13, 6.97, -0.7 })->SetRotation(glm::vec3(0, 29, 0));

			bridge->Add<TriggerVolumeEnterBehaviour>();
		}

		//Stage Mesh - bridge
		GameObject::Sptr pillar = scene->CreateGameObject("Pillar 1");
		{
			// Set position in the scene
			pillar->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			pillar->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			pillar->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = pillar->Add<RenderComponent>();
			renderer->SetMesh(stagePillarMesh);
			renderer->SetMaterial(rockPillarMaterial);

			BoxCollider::Sptr collider1 = BoxCollider::Create(glm::vec3(2, 3, 2));
			collider1->SetPosition(glm::vec3(10.86, 3, -11.58));
			collider1->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider2 = BoxCollider::Create(glm::vec3(4, 1.65, 4));
			collider2->SetPosition(glm::vec3(10.86, 7.72, -11.58));
			collider2->SetRotation(glm::vec3(0, 0, 0));

			//ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();

			RigidBody::Sptr physics = pillar->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider1);
			physics->AddCollider(collider2);
		}

		GameObject::Sptr pillar2 = scene->CreateGameObject("Pillar 2");
		{
			// Set position in the scene
			pillar2->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			pillar2->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			pillar2->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = pillar2->Add<RenderComponent>();
			renderer->SetMesh(stagePillar2Mesh);
			renderer->SetMaterial(rockPillarMaterial);

			BoxCollider::Sptr collider1 = BoxCollider::Create(glm::vec3(2, 3, 2));
			collider1->SetPosition(glm::vec3(23, 3, 9.8));
			collider1->SetRotation(glm::vec3(0, 0, 0));

			BoxCollider::Sptr collider2 = BoxCollider::Create(glm::vec3(4, 1.65, 4));
			collider2->SetPosition(glm::vec3(23, 7.72, 9.8));
			collider2->SetRotation(glm::vec3(0, 0, 0));

			//ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();

			RigidBody::Sptr physics = pillar2->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider1);
			physics->AddCollider(collider2);

			/*
			TriggerVolume::Sptr volume = pillar->Add<TriggerVolume>();
			volume->AddCollider(BoxCollider::Create(glm::vec3(4, 1.65, 4)))->SetPosition(glm::vec3(10.86, 7.72, -11.58))->SetRotation(glm::vec3(0, 0, 0));

			pillar->Add<TriggerVolumeEnterBehaviour>();
			*/
		}
		/*
		barrelMesh //
		cactusMesh///
		roundCactusMesh//
		grassMesh//
		wiltedTreeMesh
		wiltedTree2Mesh
		tumbleweedMesh
		smallRocksMesh//
		floatingRockMesh//
		rock2Mesh//
		*/

		GameObject::Sptr barrel1 = scene->CreateGameObject("Barrel 1");
		{
			// Set position in the scene
			barrel1->SetPosition(glm::vec3(-19.82f, 0.0f, 1.0f));
			barrel1->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			barrel1->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = barrel1->Add<RenderComponent>();
			renderer->SetMesh(barrelMesh);
			renderer->SetMaterial(barrelMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			collider->SetPosition(glm::vec3(0.0f, 1.0f, 0.0f));

			RigidBody::Sptr physics = barrel1->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(collider);

		}

		GameObject::Sptr grass = scene->CreateGameObject("Grass 1");
		{
			// Set position in the scene
			grass->SetPosition(glm::vec3(-16.75, -17.85, -1));
			grass->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			grass->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = grass->Add<RenderComponent>();
			renderer->SetMesh(grassMesh);
			renderer->SetMaterial(grassMaterial);
		}

		GameObject::Sptr grass2 = scene->CreateGameObject("Grass 2");
		{
			// Set position in the scene
			grass2->SetPosition(glm::vec3(-7.08, 12, -1));
			grass2->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			grass2->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = grass2->Add<RenderComponent>();
			renderer->SetMesh(grassMesh);
			renderer->SetMaterial(grassMaterial);
		}//

		GameObject::Sptr grass3 = scene->CreateGameObject("Grass 3");
		{
			// Set position in the scene
			grass3->SetPosition(glm::vec3(-0.26, 4, -1));
			grass3->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			grass3->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = grass3->Add<RenderComponent>();
			renderer->SetMesh(grassMesh);
			renderer->SetMaterial(grassMaterial);
		}

		GameObject::Sptr grass4 = scene->CreateGameObject("Grass 4");
		{
			// Set position in the scene
			grass4->SetPosition(glm::vec3(21.71, 8, -1));
			grass4->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			grass4->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = grass4->Add<RenderComponent>();
			renderer->SetMesh(grassMesh);
			renderer->SetMaterial(grassMaterial);
		}

		GameObject::Sptr grass5 = scene->CreateGameObject("Grass 5");
		{
			// Set position in the scene
			grass5->SetPosition(glm::vec3(51.5, 10, -1));
			grass5->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			grass5->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = grass5->Add<RenderComponent>();
			renderer->SetMesh(grassMesh);
			renderer->SetMaterial(grassMaterial);
		}



		GameObject::Sptr cactus = scene->CreateGameObject("Cactus");
		{
			// Set position in the scene
			cactus->SetPosition(glm::vec3(-17.73, -13.07, -1));
			cactus->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			cactus->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = cactus->Add<RenderComponent>();
			renderer->SetMesh(cactusMesh);
			renderer->SetMaterial(cactusMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			//collider->SetPosition(barrel1->GetPosition());
			RigidBody::Sptr physics = cactus->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider);

		}
		//SetPosition(glm::vec3(52.82,1,10));

		GameObject::Sptr roundCactus = scene->CreateGameObject("Cactus Round ");
		{
			// Set position in the scene
			roundCactus->SetPosition(glm::vec3(52.82, 10, -1));
			roundCactus->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			roundCactus->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = roundCactus->Add<RenderComponent>();
			renderer->SetMesh(roundCactusMesh);
			renderer->SetMaterial(cactusMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			//collider->SetPosition(barrel1->GetPosition());
			RigidBody::Sptr physics = roundCactus->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider);

		}
		/*
		GameObject::Sptr tree = scene->CreateGameObject("Tree 1");
		{
			// Set position in the scene
			tree->SetPosition(glm::vec3(0,-24.24, -1));
			tree->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			tree->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = tree->Add<RenderComponent>();
			renderer->SetMesh(wiltedTree2Mesh);
			renderer->SetMaterial(beigeTreeMaterial);
		}


		GameObject::Sptr tree2 = scene->CreateGameObject("Tree 2");
		{
			// Set position in the scene
			tree2->SetPosition(glm::vec3(21.71, 15, -1));
			tree2->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
			tree2->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = tree2->Add<RenderComponent>();
			renderer->SetMesh(wiltedTree2Mesh);
			renderer->SetMaterial(greyTreeMaterial);
		}
		*/
		//collider->SetPosition(glm::vec3(15.14,1,-32.57));
		//SetPosition(glm::vec3(39.99,1,0.15));

		//collider->SetPosition(glm::vec3(13.57,1,22.73))

		GameObject::Sptr smallRocks = scene->CreateGameObject("Small Rocks");
		{
			// Set position in the scene
			smallRocks->SetPosition(glm::vec3(14.14, -23.57, -1));
			smallRocks->SetScale(glm::vec3(2.0f, 2.0f, 2.0f));
			smallRocks->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = smallRocks->Add<RenderComponent>();
			renderer->SetMesh(smallRocksMesh);
			renderer->SetMaterial(rockMaterial);

		}

		GameObject::Sptr bigRocks = scene->CreateGameObject("Big Rocks");
		{
			// Set position in the scene
			bigRocks->SetPosition(glm::vec3(39.99, 0.15, -1));
			bigRocks->SetScale(glm::vec3(2.0f, 2.0f, 2.0f));
			bigRocks->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = bigRocks->Add<RenderComponent>();
			renderer->SetMesh(bigRocksMesh);
			renderer->SetMaterial(rockMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(3, 2.84, 4.87));
			collider->SetPosition(glm::vec3(-0.9, 3.31, -1));
			collider->SetRotation(glm::vec3(0, 0, 0));

			RigidBody::Sptr physics = bigRocks->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(collider);
		}

		GameObject::Sptr tumbleWeed = scene->CreateGameObject("TumbleWeed");
		{
			// Set position in the scene
			tumbleWeed->SetPosition(glm::vec3(0, 0, -1));
			tumbleWeed->SetScale(glm::vec3(5.0f, 5.0f, 5.0f));
			tumbleWeed->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = tumbleWeed->Add<RenderComponent>();
			renderer->SetMesh(tumbleweedMesh);
			renderer->SetMaterial(tumbleweedMaterial);

		}

		//LERP platform
		GameObject::Sptr movingPlat = scene->CreateGameObject("GroundMoving");
		{
			// Set position in the scene
			movingPlat->SetPosition(glm::vec3(10.0f, 0.0f, 5.0f));

			movingPlat->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			// Scale down the plane
			movingPlat->SetScale(glm::vec3(1.0f, 1.0f, 0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = movingPlat->Add<RenderComponent>();
			renderer->SetMesh(movingPlatMesh);
			renderer->SetMaterial(rockMaterial);

			TriggerVolume::Sptr volume = movingPlat->Add<TriggerVolume>();

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(2.0f, 0.5f, 2.0f));

			RigidBody::Sptr physics = movingPlat->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(collider);
			volume->AddCollider(collider);

			movingPlat->Add<TriggerVolumeEnterBehaviour>();

			movingPlat->Add<MovingPlatform>();

			std::vector<glm::vec3> nodes = { glm::vec3(10, 0, 5), glm::vec3(7, 0, 7), glm::vec3(4, 3, 5), glm::vec3(6, 2, 2) };

			movingPlat->Get<MovingPlatform>()->SetMode(MovingPlatform::MovementMode::LERP);
			movingPlat->Get<MovingPlatform>()->SetNodes(nodes, 3.0f);

		}

		//Bezier platform
		GameObject::Sptr movingPlat2 = scene->CreateGameObject("GroundMoving2");
		{
			// Set position in the scene
			movingPlat2->SetPosition(glm::vec3(-8.5f, -7.0f, 5.0f));
			movingPlat2->SetRotation(glm::vec3(0.0f, 0.0f, 40.0f));
			// Scale down the plane
			movingPlat2->SetScale(glm::vec3(1.0f, 1.0f, 0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = movingPlat2->Add<RenderComponent>();
			renderer->SetMesh(movingPlatMesh);
			renderer->SetMaterial(rockMaterial);

			TriggerVolume::Sptr volume = movingPlat2->Add<TriggerVolume>();

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(2.0f, 2.0f, 0.5f));

			RigidBody::Sptr physics = movingPlat2->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(collider);
			volume->AddCollider(collider);

			movingPlat2->Add<TriggerVolumeEnterBehaviour>();

			movingPlat2->Add<MovingPlatform>();

			std::vector<glm::vec3> nodes = { glm::vec3(-8.5f, -3.0f, -50.0f), glm::vec3(-8.5f, -7.0f, 5.0f), glm::vec3(-4.5f, -20.0f, 5.0f), glm::vec3(-4.5, -24.0f, -50.0f) };

			movingPlat2->Get<MovingPlatform>()->SetMode(MovingPlatform::MovementMode::BEZIER);
			movingPlat2->Get<MovingPlatform>()->SetNodes(nodes, 6.0f);
		}

		//Catmull-Rom platform
		GameObject::Sptr movingPlat3 = scene->CreateGameObject("GroundMoving3");
		{
			// Set position in the scene
			movingPlat3->SetPosition(glm::vec3(50.0f, -10.0f, 1.5f));
			movingPlat3->SetRotation(glm::vec3(0.0f, 0.0f, -85.0f));
			// Scale down the plane
			movingPlat3->SetScale(glm::vec3(1.0f, 1.0f, 0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = movingPlat3->Add<RenderComponent>();
			renderer->SetMesh(movingPlatMesh);
			renderer->SetMaterial(rockMaterial);

			TriggerVolume::Sptr volume = movingPlat3->Add<TriggerVolume>();

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(2.0f, 2.0f, 0.5f));

			RigidBody::Sptr physics = movingPlat3->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(collider);
			volume->AddCollider(collider);

			movingPlat3->Add<TriggerVolumeEnterBehaviour>();

			movingPlat3->Add<MovingPlatform>();

			std::vector<glm::vec3> nodes = { glm::vec3(50.0f, -10.0f, 1.5f), glm::vec3(50.0f, -1.5f, 6.0f), glm::vec3(50.0f, 7.0f, 12.0f), glm::vec3(47.0f, 15.0f, 7.5f) };

			movingPlat3->Get<MovingPlatform>()->SetMode(MovingPlatform::MovementMode::CATMULL);
			movingPlat3->Get<MovingPlatform>()->SetNodes(nodes, 5.0f);
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
			renderer->SetMesh(boomerangMesh2);
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

		GameObject::Sptr catcus = scene->CreateGameObject("Catcus Base");
		{
			// Set position in the scene
			catcus->SetPosition(glm::vec3(20.0f, 0.0f, 0.0f));
			catcus->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			catcus->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = catcus->Add<RenderComponent>();
			renderer->SetMesh(catcusMesh);
			renderer->SetMaterial(catcusMaterial);


			//Only add an animator when you have a clip to add.
			MorphAnimator::Sptr animator = catcus->Add<MorphAnimator>();

			//Add the walking clip
			animator->AddClip(catcusFrames, 0.7f, "Idle");

			//Make sure to always activate an animation at the time of creation (usually idle)
			animator->ActivateAnim("Idle");
		}

		GameObject::Sptr torch = scene->CreateGameObject("Torch");
		{
			// Set position in the scene
			torch->SetPosition(glm::vec3(-68.29f, 14.35f, 2.09f));
			torch->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
			torch->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = torch->Add<RenderComponent>();
			renderer->SetMesh(torchMesh);
			renderer->SetMaterial(torchMaterial);


			//Only add an animator when you have a clip to add.
			MorphAnimator::Sptr animator = torch->Add<MorphAnimator>();

			//Add the walking clip
			animator->AddClip(torchIdle, 0.5f, "Idle");

			//Make sure to always activate an animation at the time of creation (usually idle)
			animator->ActivateAnim("Idle");
		}

		GameObject::Sptr healthPack = scene->CreateGameObject("Health Pack");
		{
			// Set position in the scene
			healthPack->SetPosition(glm::vec3(0.0f, -8.5f, 7.5f));
			healthPack->SetScale(glm::vec3(0.15f, 0.15f, 0.15f));
			healthPack->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = healthPack->Add<RenderComponent>();
			renderer->SetMesh(healthPackMesh);
			renderer->SetMaterial(healthPackMaterial);


			//Only add an animator when you have a clip to add.
			MorphAnimator::Sptr animator = healthPack->Add<MorphAnimator>();

			//Add the walking clip
			animator->AddClip(healthPackIdle, 0.5f, "Idle");

			//Make sure to always activate an animation at the time of creation (usually idle)
			animator->ActivateAnim("Idle");
		}

		/////////////////////////// UI //////////////////////////////
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
			//crosshairs->SetRenderFlag(1);//this is how you would set this ui object to ONLY render for player 1

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
			//crosshairs->SetRenderFlag(1);//this is how you would set this ui object to ONLY render for player 1

			RectTransform::Sptr transform = crosshairs2->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
			transform->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });

			crosshairs->SetRenderFlag(2);

			GuiPanel::Sptr panel = crosshairs2->Add<GuiPanel>();
			panel->SetBorderRadius(4);
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/CrossHairs.png"));
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}