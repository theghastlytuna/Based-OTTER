#include "ParticleLayer.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Application/Application.h"
#include "RenderLayer.h"

ParticleLayer::ParticleLayer() :
	ApplicationLayer()
{
	Name = "Particles";
	Overrides = AppLayerFunctions::OnUpdate | AppLayerFunctions::OnPostRender;
}

ParticleLayer::~ParticleLayer()
{ }

void ParticleLayer::OnUpdate()
{
	Application& app = Application::Get();

	RenderLayer::Sptr renderer = app.GetLayer<RenderLayer>();
	UniformBuffer<RenderLayer::FrameLevelUniforms>::Sptr frameUn = renderer->GetFrameUniforms();

	auto& projection = app.CurrentScene()->MainCamera->GetProjection();
	auto& view = app.CurrentScene()->MainCamera->GetView();
	auto& viewProj = app.CurrentScene()->MainCamera->GetViewProjection();

	auto& frameData = frameUn->GetData();
	frameData.u_Projection = projection;
	frameData.u_View = view;
	frameData.u_ViewProjection = viewProj;
	frameData.u_CameraPos = view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	frameUn->Update();

	// Only update the particle systems when the game is playing, so we can edit them in
	// the inspector
	glm::uvec4 viewport = app.GetPrimaryViewport();
	glViewport(viewport.x, viewport.w / 2.0f, viewport.z, viewport.w / 2.0f);
	if (app.CurrentScene()->IsPlaying) {
		app.CurrentScene()->Components().Each<ParticleSystem>([](const ParticleSystem::Sptr& system) {
			if (system->IsEnabled && system->GetFlag() == 1) {
				system->Update();
			}
		});
	}

	auto& projection2 = app.CurrentScene()->MainCamera2->GetProjection();
	auto& view2 = app.CurrentScene()->MainCamera2->GetView();
	auto& viewProj2 = app.CurrentScene()->MainCamera2->GetViewProjection();

	auto& frameData2 = frameUn->GetData();
	frameData2.u_Projection = projection2;
	frameData2.u_View = view2;
	frameData2.u_ViewProjection = viewProj2;
	frameData2.u_CameraPos = view2 * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	frameUn->Update();
	
	glViewport(viewport.x, viewport.y, viewport.z, viewport.w / 2.0f);
	if (app.CurrentScene()->IsPlaying) {
		app.CurrentScene()->Components().Each<ParticleSystem>([](const ParticleSystem::Sptr& system) {
			if (system->IsEnabled && system->GetFlag() == 2) {
				system->Update();
			}
			});
	}
}

void ParticleLayer::OnPostRender()
{
	Application& app = Application::Get();
	const glm::uvec4& viewport = app.GetPrimaryViewport();

	// Restore viewport to game viewport

	RenderLayer::Sptr renderer = app.GetLayer<RenderLayer>();
	UniformBuffer<RenderLayer::InstanceLevelUniforms>::Sptr instUn = renderer->GetInstanceUniforms();
	UniformBuffer<RenderLayer::FrameLevelUniforms>::Sptr frameUn = renderer->GetFrameUniforms();
	const Framebuffer::Sptr renderOutput = renderer->GetRenderOutput();
	renderOutput->Bind();
	glViewport(0, 0, renderOutput->GetWidth(), renderOutput->GetHeight());

	auto& projection = app.CurrentScene()->MainCamera->GetProjection();
	auto& view = app.CurrentScene()->MainCamera->GetView();
	auto& viewProj = app.CurrentScene()->MainCamera->GetViewProjection();

	auto& frameData = frameUn->GetData();
	frameData.u_Projection = projection;
	frameData.u_View = view;
	frameData.u_ViewProjection = viewProj;
	frameData.u_CameraPos = view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	frameUn->Update();

	glViewport(viewport.x, viewport.w / 2.0f, viewport.z, viewport.w / 2.0f);
	Application::Get().CurrentScene()->Components().Each<ParticleSystem>([](const ParticleSystem::Sptr& system) {
		if (system->IsEnabled && system->GetFlag() == 1) {
			system->Render(); 
		}
	});
	
	auto& projection2 = app.CurrentScene()->MainCamera2->GetProjection();
	auto& view2 = app.CurrentScene()->MainCamera2->GetView();
	auto& viewProj2 = app.CurrentScene()->MainCamera2->GetViewProjection();

	auto& frameData2 = frameUn->GetData();
	frameData2.u_Projection = projection2;
	frameData2.u_View = view2;
	frameData2.u_ViewProjection = viewProj2;
	frameData2.u_CameraPos = view2 * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	frameUn->Update();

	glViewport(viewport.x, viewport.y, viewport.z, viewport.w / 2.0f);
	Application::Get().CurrentScene()->Components().Each<ParticleSystem>([](const ParticleSystem::Sptr& system) {
		if (system->IsEnabled && system->GetFlag() == 2) {
			system->Render();
		}
	});
	
	renderer->GetRenderOutput()->Unbind();
}
