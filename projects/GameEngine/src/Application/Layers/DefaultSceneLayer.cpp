#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
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
#include "Graphics/Framebuffer.h"

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
#include "Gameplay/Components/Light.h"

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

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"
using namespace Gameplay::Physics;

std::vector<Gameplay::MeshResource::Sptr>LoadTargets(int numTargets, std::string path)
{
	std::vector<Gameplay::MeshResource::Sptr> tempVec;
	for (int i = 0; i < numTargets; i++)
	{
		tempVec.push_back(ResourceManager::CreateAsset<Gameplay::MeshResource>(path + std::to_string(i) + ".obj"));
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

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	//_CreateScene();
}

void DefaultSceneLayer::BeginLayer()
{
	_CreateScene();
}

Gameplay::Scene::Sptr DefaultSceneLayer::GetScene()
{
	return _scene[currentSceneNum];
}

/// <summary>
/// handles creating or loading the scene
/// </summary>
void DefaultSceneLayer::_CreateScene() {

	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	app.SetPrimaryViewport(glm::uvec4( 0, 0, app.GetWindowSize().x / 2, app.GetWindowSize().y / 2 ));
	//app.SetSecondaryViewport(glm::uvec4(app.GetWindowSize().x / 2, app.GetWindowSize().y / 2, app.GetWindowSize().x, app.GetWindowSize().y));
	
	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	}
	else {
		
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		/*ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");*/

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		basicShader->SetDebugName("Deferred - GBuffer Generation");

		// This shader handles our basic materials without reflections (cause they expensive)
		/*ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});*/

		ShaderProgram::Sptr animShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/morphAnim.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});

		
		ShaderProgram::Sptr animShaderDepleted = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/morphAnim.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/depleteditem_vertlighting_frag.glsl" }
		});

		///////////////////// NEW SHADERS ////////////////////////////////////////////
		
		// This shader handles our foliage vertex shader example
		/*ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");*/

		// This shader handles our cel shading example
		/*ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});*/

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
		MeshResource::Sptr boiMesh = ResourceManager::CreateAsset<MeshResource>("boi-tpose.obj");
		MeshResource::Sptr catcusMesh = ResourceManager::CreateAsset<MeshResource>("CatcusAnims/Catcus_Idle_001.obj");
		MeshResource::Sptr mainCharMesh = ResourceManager::CreateAsset<MeshResource>("mainChar.obj");
		MeshResource::Sptr mainCharMesh2 = ResourceManager::CreateAsset<MeshResource>("mainChar.obj");
		MeshResource::Sptr boomerangMesh = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");
		MeshResource::Sptr boomerangMesh2 = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");

		MeshResource::Sptr displayBoomerangMesh = ResourceManager::CreateAsset<MeshResource>("BoomerangAnims/Boomerang_Active_000.obj");

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

		MeshResource::Sptr rampHitboxMesh = ResourceManager::CreateAsset<MeshResource>("stageObjs/ramps3.obj");

		MeshResource::Sptr rampHitboxMesh1 = ResourceManager::CreateAsset<MeshResource>("stageObjs/ramp1.obj");
		MeshResource::Sptr rampHitboxMesh2 = ResourceManager::CreateAsset<MeshResource>("stageObjs/ramp1.obj");

		//Assets

		MeshResource::Sptr barrelMesh = ResourceManager::CreateAsset<MeshResource>("barrel.obj");

		MeshResource::Sptr cactusMesh = ResourceManager::CreateAsset<MeshResource>("cactus.obj");
		MeshResource::Sptr roundCactusMesh = ResourceManager::CreateAsset<MeshResource>("cactus2.obj");
		MeshResource::Sptr grassMesh = ResourceManager::CreateAsset<MeshResource>("grass.obj");


		//MeshResource::Sptr wiltedTreeMesh = ResourceManager::CreateAsset<MeshResource>("tree_straight.obj");

		//MeshResource::Sptr wiltedTree2Mesh = ResourceManager::CreateAsset<MeshResource>("tree_slanted.obj");


		MeshResource::Sptr tumbleweedMesh = ResourceManager::CreateAsset<MeshResource>("tumbleweed2.obj");
		MeshResource::Sptr smallRocksMesh = ResourceManager::CreateAsset<MeshResource>("small_rocks.obj");

		MeshResource::Sptr bigRocksMesh = ResourceManager::CreateAsset<MeshResource>("big_rocks.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");

		//Texture2D::Sptr    boxSpec = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");

		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");

		Texture2D::Sptr    leafTex = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Unknown);
		leafTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   catcusTex = ResourceManager::CreateAsset<Texture2D>("textures/cattusGood.png");
		catcusTex->SetMinFilter(MinFilter::Unknown);
		catcusTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   mainCharTex = ResourceManager::CreateAsset<Texture2D>("textures/Char.png");
		mainCharTex->SetMinFilter(MinFilter::Unknown);
		mainCharTex->SetMagFilter(MagFilter::Nearest);

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		//Stage Textures

		Texture2D::Sptr    sandTexture = ResourceManager::CreateAsset<Texture2D>("textures/sandFloor.png");
		sandTexture->SetMinFilter(MinFilter::Unknown);
		sandTexture->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    rockFloorTexture = ResourceManager::CreateAsset<Texture2D>("textures/rockyFloor.png");
		rockFloorTexture->SetMinFilter(MinFilter::Unknown);
		rockFloorTexture->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    rockFormationTexture = ResourceManager::CreateAsset<Texture2D>("textures/bigRock.png");
		rockFormationTexture->SetMinFilter(MinFilter::Unknown);
		rockFormationTexture->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    bridgeTexture = ResourceManager::CreateAsset<Texture2D>("textures/woodBridge.png");
		bridgeTexture->SetMinFilter(MinFilter::Unknown);
		bridgeTexture->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    rockWallsTexture = ResourceManager::CreateAsset<Texture2D>("textures/walls.png");
		rockWallsTexture->SetMinFilter(MinFilter::Unknown);
		rockWallsTexture->SetMagFilter(MagFilter::Nearest);

		//asset textures
		
		Texture2D::Sptr    barrelTex = ResourceManager::CreateAsset<Texture2D>("textures/barrelTex.png");
		barrelTex->SetMinFilter(MinFilter::Unknown);
		barrelTex->SetMagFilter(MagFilter::Nearest);
		
		Texture2D::Sptr	   healthPackTex = ResourceManager::CreateAsset<Texture2D>("textures/vegemiteTex.png");
		healthPackTex->SetMinFilter(MinFilter::Unknown);
		healthPackTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   torchTex = ResourceManager::CreateAsset<Texture2D>("textures/Torch.png");
		torchTex->SetMinFilter(MinFilter::Unknown);
		torchTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr	   boomerangTex = ResourceManager::CreateAsset<Texture2D>("textures/boomerwang.png");
		boomerangTex->SetMinFilter(MinFilter::Unknown);
		boomerangTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    cactusTex = ResourceManager::CreateAsset<Texture2D>("textures/cactusTexture.png");
		cactusTex->SetMinFilter(MinFilter::Unknown);
		cactusTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    grassTex = ResourceManager::CreateAsset<Texture2D>("textures/grassTex.png");
		grassTex->SetMinFilter(MinFilter::Unknown);
		grassTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    greyTreeTex = ResourceManager::CreateAsset<Texture2D>("textures/greyTreeTex.png");
		greyTreeTex->SetMinFilter(MinFilter::Unknown);
		greyTreeTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    beigeTreeTex = ResourceManager::CreateAsset<Texture2D>("textures/beigeTreeTex.png");
		beigeTreeTex->SetMinFilter(MinFilter::Unknown);
		beigeTreeTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    rockTex = ResourceManager::CreateAsset<Texture2D>("textures/rockTex.png");
		rockTex->SetMinFilter(MinFilter::Unknown);
		rockTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    tumbleweedTex = ResourceManager::CreateAsset<Texture2D>("textures/tumbleweedTex.png");
		tumbleweedTex->SetMinFilter(MinFilter::Unknown);
		tumbleweedTex->SetMagFilter(MagFilter::Nearest);


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

		//std::vector<MeshResource::Sptr> boomerangSpin = LoadTargets(4, "BoomerangAnims/Boomerang_Active_00");

		std::vector<MeshResource::Sptr> torchIdle = LoadTargets(6, "TorchAnims/Torch_Idle_00");

		std::vector<MeshResource::Sptr> healthPackIdle = LoadTargets(7, "HealthPackAnims/healthPack_idle_00");

#pragma region Basic Texture Creation
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

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/sky/sky.jpg");// Please for the love of god
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

		scene->SetAmbientLight(glm::vec3(0.2f));
		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap);
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/Coolish.CUBE");  
		//Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");  

		// Configure the color correction LUT
		scene->SetColorLUT(lut);
		//
		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr movingPlatMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			movingPlatMaterial->Name = "movingPlatMaterial";
			movingPlatMaterial->Set("u_Material.AlbedoMap", rockTex);
			movingPlatMaterial->Set("u_Material.Shininess", 0.1f);
			movingPlatMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr torchMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			torchMaterial->Name = "torchMaterial";
			torchMaterial->Set("u_Material.AlbedoMap", torchTex);
			torchMaterial->Set("u_Material.Shininess", 0.1f);
			torchMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr catcusMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			catcusMaterial->Name = "catcusMaterial";
			catcusMaterial->Set("u_Material.AlbedoMap", catcusTex);
			catcusMaterial->Set("u_Material.Shininess", 0.1f);
			catcusMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr healthPackMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			healthPackMaterial->Name = "healthPackMaterial";
			healthPackMaterial->Set("u_Material.AlbedoMap", healthPackTex);
			healthPackMaterial->Set("u_Material.Shininess", 0.1f);
			healthPackMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}
		Material::Sptr healthPackDepletedMaterial = ResourceManager::CreateAsset<Material>(animShaderDepleted);
		{
			healthPackDepletedMaterial->Name = "healthPackDepletedMaterial";
			healthPackDepletedMaterial->Set("u_Material.AlbedoMap", healthPackTex);
			healthPackDepletedMaterial->Set("u_Material.Shininess", 0.1f);
			healthPackDepletedMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr mainCharMaterial = ResourceManager::CreateAsset<Material>(animShader);
		{
			mainCharMaterial->Name = "mainCharMaterial";
			mainCharMaterial->Set("u_Material.AlbedoMap", mainCharTex);
			mainCharMaterial->Set("u_Material.Shininess", 0.1f);
			mainCharMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr mainCharMaterial2 = ResourceManager::CreateAsset<Material>(animShader);
		{
			mainCharMaterial2->Name = "mainCharMaterial2";
			mainCharMaterial2->Set("u_Material.AlbedoMap", mainCharTex);
			mainCharMaterial2->Set("u_Material.Shininess", 0.1f);
			mainCharMaterial2->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr boomerangMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boomerangMaterial->Name = "boomerangMaterial";
			boomerangMaterial->Set("u_Material.AlbedoMap", boomerangTex);
			boomerangMaterial->Set("u_Material.Shininess", 0.1f);
			boomerangMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}
		/*
		Material::Sptr boomerangMaterial2 = ResourceManager::CreateAsset<Material>(animShader);
		{
			boomerangMaterial2->Name = "boomerangMaterial2";
			boomerangMaterial2->Set("u_Material.AlbedoMap", boomerangTex);
			boomerangMaterial2->Set("u_Material.Shininess", 0.1f);
			boomerangMaterial2->Set("u_Material.NormalMap", normalMapDefault);
		}
		*/

		Material::Sptr displayBoomerangMaterial1 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			displayBoomerangMaterial1->Name = "displayBoomerangMaterial1";
			displayBoomerangMaterial1->Set("u_Material.AlbedoMap", boomerangTex);
			displayBoomerangMaterial1->Set("u_Material.Shininess", 0.0f);
			displayBoomerangMaterial1->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr displayBoomerangMaterial2 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			displayBoomerangMaterial2->Name = "displayBoomerangMaterial2";
			displayBoomerangMaterial2->Set("u_Material.AlbedoMap", boomerangTex);
			displayBoomerangMaterial2->Set("u_Material.Shininess", 0.0f);
			displayBoomerangMaterial2->Set("u_Material.NormalMap", normalMapDefault);
		}
		/*
		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(reflectiveShader);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.Diffuse", monkeyTex);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}
		*/

		// This will be the reflective material, we'll make the whole thing 90% reflective
		/*Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.Diffuse", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
		}*/

		/*
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
		*/

		//// Our toon shader material
		//Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		//{
		//	toonMaterial->Name = "Toon";
		//	toonMaterial->Set("u_Material.Diffuse", boxTexture);
		//	toonMaterial->Set("u_Material.Shininess", 0.1f);
		//	toonMaterial->Set("u_Material.Steps", 8);
		//}

		/////////////Stage materials

		//sand material
		Material::Sptr sandMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			sandMaterial->Name = "sandMaterial";
			sandMaterial->Set("u_Material.AlbedoMap", sandTexture);
			sandMaterial->Set("u_Material.Shininess", 0.1f);
			sandMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		//rock floor material
		Material::Sptr rockFloorMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockFloorMaterial->Name = "rockFloorMaterial";
			rockFloorMaterial->Set("u_Material.AlbedoMap", rockFloorTexture);
			rockFloorMaterial->Set("u_Material.Shininess", 0.1f);
			rockFloorMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		//rock Pillar material
		Material::Sptr rockPillarMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockPillarMaterial->Name = "rockPillarMaterial";
			rockPillarMaterial->Set("u_Material.AlbedoMap", rockFormationTexture);
			rockPillarMaterial->Set("u_Material.Shininess", 0.1f);
			rockPillarMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		//Wall Material
		Material::Sptr rockWallMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockWallMaterial->Name = "rockWallMaterial";
			rockWallMaterial->Set("u_Material.AlbedoMap", rockWallsTexture);
			rockWallMaterial->Set("u_Material.Shininess", 0.1f);
			rockWallMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr bridgeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			bridgeMaterial->Name = "bridgeMaterial";
			bridgeMaterial->Set("u_Material.AlbedoMap", bridgeTexture);
			bridgeMaterial->Set("u_Material.Shininess", 0.1f);
			bridgeMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		/*
		Okay, I pull up, hop out at the after party
		You and all your friends, yeah, they love to get naughty
		Sippin' on that Henn', I know you love that Bacardi (Sonny Digital)
1		942, I take you back in that 'Rari
		*/


		Material::Sptr barrelMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			barrelMaterial->Name = "barrelMaterial";
			barrelMaterial->Set("u_Material.AlbedoMap", barrelTex);
			barrelMaterial->Set("u_Material.Shininess", 0.1f);
			barrelMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr cactusMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			cactusMaterial->Name = "cactusMaterial";
			cactusMaterial->Set("u_Material.AlbedoMap", cactusTex);
			cactusMaterial->Set("u_Material.Shininess", 0.1f);
			cactusMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			grassMaterial->Name = "grassMaterial";
			grassMaterial->Set("u_Material.AlbedoMap", grassTex);
			grassMaterial->Set("u_Material.Shininess", 0.1f);
			grassMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr greyTreeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			greyTreeMaterial->Name = "greyTreeMaterial";
			greyTreeMaterial->Set("u_Material.AlbedoMap", greyTreeTex);
			greyTreeMaterial->Set("u_Material.Shininess", 0.1f);
			greyTreeMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr beigeTreeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			beigeTreeMaterial->Name = "beigeTreeMaterial";
			beigeTreeMaterial->Set("u_Material.AlbedoMap", beigeTreeTex);
			beigeTreeMaterial->Set("u_Material.Shininess", 0.1f);
			beigeTreeMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			rockMaterial->Name = "rockMaterial";
			rockMaterial->Set("u_Material.AlbedoMap", rockTex);
			rockMaterial->Set("u_Material.Shininess", 0.1f);
			rockMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr tumbleweedMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			tumbleweedMaterial->Name = "tumbleweedMaterial";
			tumbleweedMaterial->Set("u_Material.AlbedoMap", tumbleweedTex);
			tumbleweedMaterial->Set("u_Material.Shininess", 0.1f);
			tumbleweedMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		GameObject::Sptr light = scene->CreateGameObject("Light"); 
		{ 
			light->SetPosition(glm::vec3(0.f, 0.f, 20.f)); 
 
			Light::Sptr lightComponent = light->Add<Light>(); 
			lightComponent->SetColor(glm::vec3(1.0f)); 
			lightComponent->SetRadius(500.f); 
			lightComponent->SetIntensity(5.f); 
		} 

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

			scene->WorldCamera = cam;
		}

		//Set up the scene's camera 
		GameObject::Sptr camera2 = scene->CreateGameObject("Main Camera 2");
		{
			camera2->SetPosition(glm::vec3(5.0f));
			camera2->LookAt(glm::vec3(0.0f));

			Camera::Sptr cam = camera2->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera! 
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

		GameObject::Sptr player1 = scene->CreateGameObject("Player 1");
		{
			ControllerInput::Sptr controller1 = player1->Add<ControllerInput>();
			controller1->SetController(GLFW_JOYSTICK_1);

			player1->SetPosition(glm::vec3(0.f, 0.f, 4.f));
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

		GameObject::Sptr player2 = scene->CreateGameObject("Player 2");
		{
			ControllerInput::Sptr controller2 = player2->Add<ControllerInput>();
			controller2->SetController(GLFW_JOYSTICK_2);

			player2->SetPosition(glm::vec3(10.f, 0.f, 4.f));

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


			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(110.0f, 50.0f, 1.0f));
			collider->SetPosition({ 0,-0.8,-1 });
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
		
		
		//Stage Mesh - side floors
		GameObject::Sptr sideGroundHB = scene->CreateGameObject("GroundSideHitbox");
		{
			// Set position in the scene
			sideGroundHB->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			sideGroundHB->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			sideGroundHB->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			int d = 35;

			TriggerVolume::Sptr volume = sideGroundHB->Add<TriggerVolume>();
			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 0.97, 7.82)))->SetPosition({ -37.86, 0.13, 19.41 })->SetRotation(glm::vec3(-5, -20, 18));
			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 1, 7.9)))->SetPosition({ -28.9, 3.46, 22.6 })->SetRotation(glm::vec3(-4, -14, 26));
			volume->AddCollider(BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17)))->SetPosition({ -20.83, 4.46, 21.86 })->SetRotation(glm::vec3(6, 36, 10));

			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 0.97, 7.82)))->SetPosition(glm::vec3((-37.86 - d) * -1, 0.13, -19.41))->SetRotation(glm::vec3(5, -20, -18));
			volume->AddCollider(BoxCollider::Create(glm::vec3(5, 1, 7.9)))->SetPosition(glm::vec3((-28.9 - d) * -1, 3.46, -22.6))->SetRotation(glm::vec3(4, -14, -26));
			volume->AddCollider(BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17)))->SetPosition(glm::vec3((-20.83 - d) * -1, 4.46, -21.86))->SetRotation(glm::vec3(-6, 36, -10));

			sideGroundHB->Add<TriggerVolumeEnterBehaviour>();
		}
		
		//Stage Mesh - side floors
		GameObject::Sptr sideGroundHB1 = scene->CreateGameObject("GroundSideHitbox1");
		{
			// Set position in the scene
			sideGroundHB1->SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
			sideGroundHB1->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			sideGroundHB1->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = sideGroundHB1->Add<RenderComponent>();
			renderer->SetMesh(rampHitboxMesh1);
			//renderer->SetMaterial(rockFloorMaterial);

			RigidBody::Sptr physics = sideGroundHB1->Add<RigidBody>();

			physics->AddCollider(ConvexMeshCollider::Create());

		}
		
		GameObject::Sptr sideGroundHB2 = scene->CreateGameObject("GroundSideHitbox2");
		{
			// Set position in the scene
			sideGroundHB2->SetPosition(glm::vec3(35.0f, 0.0f, -1.0f));
			sideGroundHB2->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			sideGroundHB2->SetRotation(glm::vec3(90.0f, 0.0f, 180.0f));

			// Create and attach a renderer
			RenderComponent::Sptr renderer = sideGroundHB2->Add<RenderComponent>();
			renderer->SetMesh(rampHitboxMesh2);
			//renderer->SetMaterial(rockFloorMaterial);

			RigidBody::Sptr physics = sideGroundHB2->Add<RigidBody>();

			physics->AddCollider(ConvexMeshCollider::Create());
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
			
			RigidBody::Sptr physics = centerWalls->Add<RigidBody>();
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

			/*
			BoxCollider::Sptr collider10 = BoxCollider::Create(glm::vec3(5, 0.97, 7.82));
			collider10->SetPosition(glm::vec3(-35.990, 0.280, 19.290));
			collider10->SetRotation(glm::vec3(-7, -19, 17));
			
			BoxCollider::Sptr collider11 = BoxCollider::Create(glm::vec3(7.17, 1, 7.9));
			collider11->SetPosition(glm::vec3(-30.70, 2.55, 21.3));
			collider11->SetRotation(glm::vec3(-4, 4, 22));
			
			BoxCollider::Sptr collider12 = BoxCollider::Create(glm::vec3(3.20, 1.0, 5.17));
			collider12->SetPosition(glm::vec3(-19.450, 6.05, 21.54));
			collider12->SetRotation(glm::vec3(4.5, 33, 9));
			*/

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
			/*
			BoxCollider::Sptr collider29 = BoxCollider::Create(glm::vec3(5, 0.97, 7.82));
			collider29->SetPosition(glm::vec3((-37.86 - d) * -1, 0.13, -19.41));
			collider29->SetRotation(glm::vec3(5, -20, -18));//// Here!  
			
			BoxCollider::Sptr collider30 = BoxCollider::Create(glm::vec3(5, 1, 7.9));
			collider30->SetPosition(glm::vec3((-28.9 - d) * -1, 3.46, -22.6));
			collider30->SetRotation(glm::vec3(4, -14, -26));//// Here!

			BoxCollider::Sptr collider31 = BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17));
			collider31->SetPosition(glm::vec3((-21.420 - d) * -1, 3.56, -24.51));
			collider31->SetRotation(glm::vec3(0, -15, -1));//// Here!
			*/

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

			/*
			BoxCollider::Sptr collider38 = BoxCollider::Create(glm::vec3(1.8, 1, 5.17));
			collider38->SetPosition(glm::vec3(-22.54, 5.64, 23.51));
			collider38->SetRotation(glm::vec3(-4, 3, 3));

			BoxCollider::Sptr collider39 = BoxCollider::Create(glm::vec3(3.64, 2.67, 5.17));
			collider39->SetPosition(glm::vec3((-20.830 - d) * -1, 4.1, -23.51));
			collider39->SetRotation(glm::vec3(0, -15, 1));
			*/
			/// <summary>
			/// 
			/// </summary>

			RigidBody::Sptr physics = sideWalls->Add<RigidBody>();
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
			//physics->AddCollider(collider10);
			//physics->AddCollider(collider11);
			//physics->AddCollider(collider12);
			physics->AddCollider(collider13);
			physics->AddCollider(collider14);
			physics->AddCollider(collider15);
			physics->AddCollider(collider16);
			physics->AddCollider(collider17);
			physics->AddCollider(collider18);
			//physics->AddCollider(collider38);

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
			//physics->AddCollider(collider29);
			//physics->AddCollider(collider30);
			//physics->AddCollider(collider31);
			physics->AddCollider(collider32);
			physics->AddCollider(collider33);
			physics->AddCollider(collider34);
			physics->AddCollider(collider35);
			physics->AddCollider(collider36);
			physics->AddCollider(collider37);
			//physics->AddCollider(collider39);
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

			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(39.8, 0.5, 2.12));
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
			//grass->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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
			//grass2->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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
			//grass3->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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
			//grass4->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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
			//grass5->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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
			//cactus->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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

		GameObject::Sptr roundCactus = scene->CreateGameObject("Cactus Round ");
		{
			// Set position in the scene
			roundCactus->SetPosition(glm::vec3(52.82, 10, -1));
			//roundCactus->SetScale(glm::vec3(30.0f, 30.0f, 30.0f));
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
			boomerang->SetRotation(glm::vec3(0.f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = boomerang->Add<RenderComponent>();
			renderer->SetMesh(boomerangMesh);
			renderer->SetMaterial(boomerangMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));
			//collider->SetExtents(glm::vec3(0.8f, 0.8f, 0.8f));

			RigidBody::Sptr physics = boomerang->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(collider);
			physics->SetAngularFactor(glm::vec3(1.f, 1.f, 0.f));

			boomerang->Add<BoomerangBehavior>();
			BoxCollider::Sptr colliderTrigger = BoxCollider::Create();
			colliderTrigger->SetScale(glm::vec3(0.4f, 0.4f, 0.4f));

			TriggerVolume::Sptr volume = boomerang->Add<TriggerVolume>();
			boomerang->Add<TriggerVolumeEnterBehaviour>();
			volume->AddCollider(colliderTrigger);

			/*
			boomerang->Add<MorphAnimator>();
			boomerang->Get<MorphAnimator>()->AddClip(boomerangSpin, 0.1, "Spin");

			boomerang->Get<MorphAnimator>()->ActivateAnim("spin");
			*/

		}

		GameObject::Sptr boomerang2 = scene->CreateGameObject("Boomerang 2");
		{
			// Set position in the scene
			boomerang2->SetPosition(glm::vec3(0.0f, 0.0f, -100.0f));
			boomerang2->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));
			boomerang2->SetRotation(glm::vec3(0.f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = boomerang2->Add<RenderComponent>();
			renderer->SetMesh(boomerangMesh2);
			renderer->SetMaterial(boomerangMaterial);

			BoxCollider::Sptr collider = BoxCollider::Create();
			collider->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

			RigidBody::Sptr physics = boomerang2->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(collider);
			physics->SetAngularFactor(glm::vec3(1.f, 1.f, 0.f));

			boomerang2->Add<BoomerangBehavior>();
			BoxCollider::Sptr colliderTrigger = BoxCollider::Create();
			colliderTrigger->SetScale(glm::vec3(0.4f, 0.4f, 0.4f));

			TriggerVolume::Sptr volume = boomerang2->Add<TriggerVolume>();
			boomerang2->Add<TriggerVolumeEnterBehaviour>();
			volume->AddCollider(colliderTrigger);


			/*
			boomerang2->Add<MorphAnimator>();
			boomerang2->Get<MorphAnimator>()->AddClip(boomerangSpin, 0.1, "Spin");

			boomerang2->Get<MorphAnimator>()->ActivateAnim("spin");
			*/
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

			BoxCollider::Sptr colliderTrigger = BoxCollider::Create();
			colliderTrigger->SetScale(glm::vec3(0.4f, 0.4f, 0.2f));
			TriggerVolume::Sptr volume = healthPack->Add<TriggerVolume>();
			volume->AddCollider(colliderTrigger);
			PickUpBehaviour::Sptr pickUp = healthPack->Add<PickUpBehaviour>();
			pickUp->DefaultMaterial = healthPackMaterial;
			pickUp->DepletedMaterial = healthPackDepletedMaterial;

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
				transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y});

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
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/volumeSelect.png"));

			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr screenSplitter = scene->CreateGameObject("Screen Splitter");
		{
			screenSplitter->SetRenderFlag(5);
			RectTransform::Sptr transform = screenSplitter->Add<RectTransform>();
			transform->SetMin({ 0, app.GetWindowSize().y / 2 - 3 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y / 2 + 3 });

			GuiPanel::Sptr canPanel = screenSplitter->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/screenSplitter.png"));

			canPanel->SetTransparency(1.0f);
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		//scene->Save("scene.json");

		_scene.push_back(scene);
		currentSceneNum++;
	}
}

void DefaultSceneLayer::SetActive(bool active)
{
	_active = active;
}

bool DefaultSceneLayer::IsActive()
{
	return _active;
}

//Function to be used when the screen is resized
void DefaultSceneLayer::RepositionUI()
{
	Application& app = Application::Get();

	//Grab all the UI elements
	Gameplay::GameObject::Sptr crosshair = app.CurrentScene()->FindObjectByName("Crosshairs");
	Gameplay::GameObject::Sptr crosshair2 = app.CurrentScene()->FindObjectByName("Crosshairs 2");
	Gameplay::GameObject::Sptr killUI = app.CurrentScene()->FindObjectByName("Score Counter 1");
	Gameplay::GameObject::Sptr killUI2 = app.CurrentScene()->FindObjectByName("Score Counter 2");

	Gameplay::GameObject::Sptr screenSplitter = app.CurrentScene()->FindObjectByName("Screen Splitter");

	//Reposition the elements
	crosshair->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
	crosshair->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });
	crosshair2->Get<RectTransform>()->SetMin({ app.GetWindowSize().x / 2 - 50, app.GetWindowSize().y / 2 - 50 });
	crosshair2->Get<RectTransform>()->SetMax({ app.GetWindowSize().x / 2 + 50, app.GetWindowSize().y / 2 + 50 });
	killUI->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 195 });
	killUI->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y });
	killUI2->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y - 195 });
	killUI2->Get<RectTransform>()->SetMax({ 200, app.GetWindowSize().y });

	screenSplitter->Get<RectTransform>()->SetMin({ 0, app.GetWindowSize().y / 2 - 3 });
	screenSplitter->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y / 2 + 3 });

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
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMin({30, app.GetWindowSize().y - 130 });
		app.CurrentScene()->FindObjectByName("1-" + std::to_string(i))->Get<RectTransform>()->SetMax({160, app.GetWindowSize().y });

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
	pauseText->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y});

	pauseBG->Get<RectTransform>()->SetMin({ 0, 0 });
	pauseBG->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y});
}