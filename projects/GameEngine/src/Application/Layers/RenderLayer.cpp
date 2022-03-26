#include "RenderLayer.h"
#include "../Application.h"
#include "Graphics/GuiBatcher.h"
#include "Gameplay/Components/Camera.h"
#include "Graphics/DebugDraw.h"
#include "Graphics/Textures/TextureCube.h"
#include "../Timing.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/Light.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)
#include "Gameplay/Components/ShadowCamera.h"


RenderLayer::RenderLayer() :
	ApplicationLayer(),
	_primaryFBO(nullptr),
	_blitFbo(true),
	_frameUniforms(nullptr),
	_instanceUniforms(nullptr),
	_renderFlags(RenderFlags::None),
	_clearColor({ 0.1f, 0.1f, 0.1f, 1.0f })
{
	Name = "Rendering";
	Overrides = 
		AppLayerFunctions::OnAppLoad | 
		AppLayerFunctions::OnPreRender | AppLayerFunctions::OnRender | AppLayerFunctions::OnPostRender | 
		AppLayerFunctions::OnWindowResize;
}

RenderLayer::~RenderLayer() = default;

void RenderLayer::OnRender(const Framebuffer::Sptr& prevLayer)
{
	using namespace Gameplay;

	Application& app = Application::Get();
	glm::uvec4 viewport = app.GetPrimaryViewport();

	glViewport(viewport.x, viewport.y, viewport.z, viewport.w / 2.0f);
	//glViewport(0, 240, 427, 240);
	// We bind our framebuffer so we can render to it
	_primaryFBO->Bind();

	// Clear the color and depth buffers
	glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Grab shorthands to the camera and shader from the scene
	Camera::Sptr camera = app.CurrentScene()->MainCamera;

	// Cache the camera's viewprojection
	glm::mat4 viewProj = camera->GetViewProjection();
	DebugDrawer::Get().SetViewProjection(viewProj);

	// Make sure depth testing and culling are re-enabled
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// The current material that is bound for rendering
	Material::Sptr currentMat = nullptr;
	ShaderProgram::Sptr shader = nullptr;

	// Bind the skybox texture to a reserved texture slot
	// See Material.h and Material.cpp for how we're reserving texture slots
	TextureCube::Sptr environment = app.CurrentScene()->GetSkyboxTexture();
	if (environment) {
		environment->Bind(15);
	}

	// Binding the color correction LUT
	Texture3D::Sptr colorLUT = app.CurrentScene()->GetColorLUT();
	if (colorLUT) {
		colorLUT->Bind(14);
	}

	// Here we'll bind all the UBOs to their corresponding slots
	app.CurrentScene()->PreRender();
	_frameUniforms->Bind(FRAME_UBO_BINDING);
	_instanceUniforms->Bind(INSTANCE_UBO_BINDING);

	// Draw physics debug
	app.CurrentScene()->DrawPhysicsDebug();

	// Upload frame level uniforms
	auto& frameData = _frameUniforms->GetData();
	frameData.u_Projection = camera->GetProjection();
	frameData.u_View = camera->GetView();
	frameData.u_ViewProjection = camera->GetViewProjection();
	frameData.u_CameraPos = glm::vec4(camera->GetGameObject()->GetPosition(), 1.0f);
	frameData.u_Time = static_cast<float>(Timing::Current().TimeSinceSceneLoad());
	frameData.u_DeltaTime = Timing::Current().DeltaTime();
	frameData.u_RenderFlags = _renderFlags;
	_frameUniforms->Update();

	Material::Sptr defaultMat = app.CurrentScene()->DefaultMaterial;

	// Render all our objects
	app.CurrentScene()->Components().Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {
		if (renderable->GetGameObject()->GetRenderFlag() == 0 ||
			renderable->GetGameObject()->GetRenderFlag() == 1)
		{


			// Early bail if mesh not set
			if (renderable->GetMesh() == nullptr) {
				return;
			}

			// If we don't have a material, try getting the scene's fallback material
			// If none exists, do not draw anything
			if (renderable->GetMaterial() == nullptr) {
				if (defaultMat != nullptr) {
					renderable->SetMaterial(defaultMat);
				}
				else {
					return;
				}
			}

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat) {
				currentMat = renderable->GetMaterial();
				shader = currentMat->GetShader();

				shader->Bind();
				currentMat->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Use our uniform buffer for our instance level uniforms
			auto& instanceData = _instanceUniforms->GetData();
			instanceData.u_Model = object->GetTransform();
			instanceData.u_ModelViewProjection = viewProj * object->GetTransform();
			instanceData.u_NormalMatrix = glm::mat3(glm::transpose(glm::inverse(object->GetTransform())));
			_instanceUniforms->Update();

			// Draw the object
			renderable->GetMesh()->Draw();
		}
	});

	// Use our cubemap to draw our skybox
	app.CurrentScene()->DrawSkybox(app.CurrentScene()->MainCamera);

	// Unbind our primary framebuffer so subsequent draw calls do not modify it
	//_primaryFBO->Unbind();

	VertexArrayObject::Unbind();

	//////////////////////////////////////////////////Render camera 2

	glViewport(viewport.x, viewport.w / 2.0f, viewport.z, viewport.w / 2.0f);
	//glViewport(0, 0, 427, 240);

	// Grab shorthands to the camera and shader from the scene
	Camera::Sptr camera2 = app.CurrentScene()->MainCamera2;

	// Cache the camera's viewprojection
	glm::mat4 viewProj2 = camera2->GetViewProjection();
	DebugDrawer::Get().SetViewProjection(viewProj2);

	// Make sure depth testing and culling are re-enabled
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// The current material that is bound for rendering
	Material::Sptr currentMat2 = nullptr;
	ShaderProgram::Sptr shader2 = nullptr;

	// Bind the skybox texture to a reserved texture slot
	// See Material.h and Material.cpp for how we're reserving texture slots
	TextureCube::Sptr environment2 = app.CurrentScene()->GetSkyboxTexture();
	if (environment2) environment2->Bind(0);

	// Here we'll bind all the UBOs to their corresponding slots
	app.CurrentScene()->PreRender();
	_frameUniforms->Bind(FRAME_UBO_BINDING);
	_instanceUniforms->Bind(INSTANCE_UBO_BINDING);

	// Draw physics debug
	app.CurrentScene()->DrawPhysicsDebug();

	// Upload frame level uniforms
	auto& frameData2 = _frameUniforms->GetData();
	frameData2.u_Projection = camera2->GetProjection();
	frameData2.u_View = camera2->GetView();
	frameData2.u_ViewProjection = camera2->GetViewProjection();
	frameData2.u_CameraPos = glm::vec4(camera2->GetGameObject()->GetPosition(), 1.0f);
	frameData2.u_Time = static_cast<float>(Timing::Current().TimeSinceSceneLoad());
	_frameUniforms->Update();

	Material::Sptr defaultMat2 = app.CurrentScene()->DefaultMaterial;

	// Render all our objects
	app.CurrentScene()->Components().Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {
		if (renderable->GetGameObject()->GetRenderFlag() == 0 ||
			renderable->GetGameObject()->GetRenderFlag() == 2)
		{

			// Early bail if mesh not set
			if (renderable->GetMesh() == nullptr) {
				return;
			}

			// If we don't have a material, try getting the scene's fallback material
			// If none exists, do not draw anything
			if (renderable->GetMaterial() == nullptr) {
				if (defaultMat2 != nullptr) {
					renderable->SetMaterial(defaultMat2);
				}
				else {
					return;
				}
			}

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat2) {
				currentMat2 = renderable->GetMaterial();
				shader2 = currentMat2->GetShader();

				shader2->Bind();
				currentMat2->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Use our uniform buffer for our instance level uniforms
			auto& instanceData = _instanceUniforms->GetData();
			instanceData.u_Model = object->GetTransform();
			instanceData.u_ModelViewProjection = viewProj2 * object->GetTransform();
			instanceData.u_NormalMatrix = glm::mat3(glm::transpose(glm::inverse(object->GetTransform())));
			_instanceUniforms->Update();

			// Draw the object
			renderable->GetMesh()->Draw();
		}
		});

	// Use our cubemap to draw our skybox
	app.CurrentScene()->DrawSkybox(camera2);

	// Unbind our primary framebuffer so subsequent draw calls do not modify it
	//_primaryFBO->Unbind();

	VertexArrayObject::Unbind();
}

void RenderLayer::OnWindowResize(const glm::ivec2& oldSize, const glm::ivec2& newSize)
{
	if (newSize.x * newSize.y == 0) return;

	// Set viewport and resize our primary FBO and light accumulation FBO
	_primaryFBO->Resize(newSize);
	_lightingFBO->Resize(newSize);
	_outputBuffer->Resize(newSize);

	// Update the main camera's projection
	Application& app = Application::Get();
	//app.CurrentScene()->MainCamera->ResizeWindow(newSize.x, newSize.y);
}

void RenderLayer::OnAppLoad(const nlohmann::json& config)
{
	Application& app = Application::Get();

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Create a new descriptor for our FBO
	FramebufferDescriptor fboDescriptor;
	fboDescriptor.Width = app.GetWindowSize().x;
	fboDescriptor.Height = app.GetWindowSize().y;

	// We want to use a 32 bit depth buffer, we'll ignore the stencil buffer for now
	fboDescriptor.RenderTargets[RenderTargetAttachment::Depth] = RenderTargetDescriptor(RenderTargetType::Depth32);
	// Color layer 0 (albedo, specular)
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color0] = RenderTargetDescriptor(RenderTargetType::ColorRgba8);
	// Color layer 1 (normals, metallic)
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color1] = RenderTargetDescriptor(RenderTargetType::ColorRgba8);
	// Color layer 2 (emissive)  
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color2] = RenderTargetDescriptor(RenderTargetType::ColorRgba8);
	// Color layer 3 (view space position)  
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color3] = RenderTargetDescriptor(RenderTargetType::ColorRgba16F);
	 
	// Create the primary FBO
	_primaryFBO = std::make_shared<Framebuffer>(fboDescriptor);

	fboDescriptor.RenderTargets.clear();
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color0] = RenderTargetDescriptor(RenderTargetType::ColorRgba8); // Diffuse
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color1] = RenderTargetDescriptor(RenderTargetType::ColorRgba8); // Specular

	_lightingFBO = std::make_shared<Framebuffer>(fboDescriptor);

	// Create an FBO to store final output
	fboDescriptor.RenderTargets.clear();
	fboDescriptor.RenderTargets[RenderTargetAttachment::Depth] = RenderTargetDescriptor(RenderTargetType::Depth32);
	fboDescriptor.RenderTargets[RenderTargetAttachment::Color0] = RenderTargetDescriptor(RenderTargetType::ColorRgba8);

	_outputBuffer = std::make_shared<Framebuffer>(fboDescriptor);

	// We'll use one shader for light accumulation for now
	_lightAccumulationShader = ShaderProgram::Create();
	_lightAccumulationShader->LoadShaderPartFromFile("shaders/vertex_shaders/fullscreen_quad.glsl", ShaderPartType::Vertex);
	_lightAccumulationShader->LoadShaderPartFromFile("shaders/fragment_shaders/light_accumulation.glsl", ShaderPartType::Fragment);
	_lightAccumulationShader->Link();

	_compositingShader = ShaderProgram::Create();
	_compositingShader->LoadShaderPartFromFile("shaders/vertex_shaders/fullscreen_quad.glsl", ShaderPartType::Vertex);
	_compositingShader->LoadShaderPartFromFile("shaders/fragment_shaders/deferred_composite.glsl", ShaderPartType::Fragment);
	_compositingShader->Link();

	_clearShader = ShaderProgram::Create();
	_clearShader->LoadShaderPartFromFile("shaders/vertex_shaders/fullscreen_quad.glsl", ShaderPartType::Vertex);
	_clearShader->LoadShaderPartFromFile("shaders/fragment_shaders/clear.glsl", ShaderPartType::Fragment);
	_clearShader->Link();

	_shadowShader = ShaderProgram::Create();
	_shadowShader->LoadShaderPartFromFile("shaders/vertex_shaders/fullscreen_quad.glsl", ShaderPartType::Vertex);
	_shadowShader->LoadShaderPartFromFile("shaders/fragment_shaders/shadow_composite.glsl", ShaderPartType::Fragment);
	_shadowShader->Link();

	// We need a mesh for drawing fullscreen quads

	glm::vec2 positions[6] = {
		{ -1.0f,  1.0f }, { -1.0f, -1.0f }, { 1.0f, 1.0f },
		{ -1.0f, -1.0f }, {  1.0f, -1.0f }, { 1.0f, 1.0f }
	};

	VertexBuffer::Sptr vbo = std::make_shared<VertexBuffer>();
	vbo->LoadData(positions, 6);

	_fullscreenQuad = VertexArrayObject::Create();
	_fullscreenQuad->AddVertexBuffer(vbo, {
		BufferAttribute(0, 2, AttributeType::Float, sizeof(glm::vec2), 0, AttribUsage::Position)
	});

	// Create our common uniform buffers
	_frameUniforms = std::make_shared<UniformBuffer<FrameLevelUniforms>>(BufferUsage::DynamicDraw);
	_instanceUniforms = std::make_shared<UniformBuffer<InstanceLevelUniforms>>(BufferUsage::DynamicDraw);
	_lightingUbo = std::make_shared<UniformBuffer<LightingUboStruct>>(BufferUsage::DynamicDraw);
}

const Framebuffer::Sptr& RenderLayer::GetPrimaryFBO() const {
	return _primaryFBO;
}

bool RenderLayer::IsBlitEnabled() const {
	return false;
}

void RenderLayer::SetBlitEnabled(bool value) {
	_blitFbo = value;
}

const Framebuffer::Sptr& RenderLayer::GetRenderOutput() const {
	return _outputBuffer;
}

const glm::vec4& RenderLayer::GetClearColor() const {
	return _clearColor;
}

void RenderLayer::SetClearColor(const glm::vec4 & value) {
	_clearColor = value;
}

void RenderLayer::SetRenderFlags(RenderFlags value) {
	_renderFlags = value;
}

RenderFlags RenderLayer::GetRenderFlags() const {
	return _renderFlags;
}

const Framebuffer::Sptr& RenderLayer::GetLightingBuffer() const {
	return _lightingFBO;
}

const Framebuffer::Sptr& RenderLayer::GetGBuffer() const
{
	return _primaryFBO;
}

void RenderLayer::_InitFrameUniforms()
{
	using namespace Gameplay;

	Application& app = Application::Get();

	// Grab shorthands to the camera and shader from the scene
	Camera::Sptr camera = app.CurrentScene()->MainCamera;

	// Cache the camera's viewprojection
	glm::mat4 viewProj = camera->GetViewProjection();
	glm::mat4 view = camera->GetView();

	// Upload frame level uniforms
	auto& frameData = _frameUniforms->GetData();
	frameData.u_Projection = camera->GetProjection();
	frameData.u_View = camera->GetView();
	frameData.u_ViewProjection = camera->GetViewProjection();
	frameData.u_CameraPos = glm::vec4(camera->GetGameObject()->GetPosition(), 1.0f);
	frameData.u_Time = static_cast<float>(Timing::Current().TimeSinceSceneLoad());
	frameData.u_DeltaTime = Timing::Current().DeltaTime();
	frameData.u_RenderFlags = _renderFlags;
	frameData.u_ZNear = camera->GetNearPlane();
	frameData.u_ZFar = camera->GetFarPlane();
	_frameUniforms->Update();
}

void RenderLayer::_RenderScene(const glm::mat4& view, const glm::mat4& projection)
{
	using namespace Gameplay;

	Application& app = Application::Get();

	glm::mat4 viewProj = projection * view;

	// The current material that is bound for rendering
	Material::Sptr currentMat = nullptr;
	ShaderProgram::Sptr shader = nullptr;

	Material::Sptr defaultMat = app.CurrentScene()->DefaultMaterial;

	auto& frameData = _frameUniforms->GetData();
	frameData.u_Projection = projection;
	frameData.u_View = view;
	frameData.u_ViewProjection = viewProj;
	frameData.u_CameraPos = view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	_frameUniforms->Update();

	// Render all our objects
	app.CurrentScene()->Components().Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {
		// Early bail if mesh not set
		if (renderable->GetMesh() == nullptr) {
			return;
		}

		// If we don't have a material, try getting the scene's fallback material
		// If none exists, do not draw anything
		if (renderable->GetMaterial() == nullptr) {
			if (defaultMat != nullptr) {
				renderable->SetMaterial(defaultMat);
			}
			else {
				return;
			}
		}

		// If the material has changed, we need to bind the new shader and set up our material and frame data
		// Note: This is a good reason why we should be sorting the render components in ComponentManager
		if (renderable->GetMaterial() != currentMat) {
			currentMat = renderable->GetMaterial();
			shader = currentMat->GetShader();

			shader->Bind();
			currentMat->Apply();
		}

		// Grab the game object so we can do some stuff with it
		GameObject* object = renderable->GetGameObject();

		// Use our uniform buffer for our instance level uniforms
		auto& instanceData = _instanceUniforms->GetData();
		instanceData.u_Model = object->GetTransform();
		instanceData.u_ModelViewProjection = viewProj * object->GetTransform();
		instanceData.u_ModelView = view * object->GetTransform();
		instanceData.u_NormalMatrix = glm::mat3(glm::transpose(glm::inverse(object->GetTransform())));
		_instanceUniforms->Update();

		// Draw the object
		renderable->GetMesh()->Draw();

	});

}

