#include "PixelizationEffect.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Graphics/Framebuffer.h"
#include "Application/Application.h"

#include <GLM/glm.hpp>

PixelizationEffect::PixelizationEffect() :
	PostProcessingLayer::Effect()
{
	Name = "Pixelization";
	_format = RenderTargetType::ColorRgb8;

	//memset(Pixels, 0, sizeof(float) * 25);
	PixelsX = 640;
	PixelsY = 480;
	Power = 1;
	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/pixelization.glsl" }
	});

	Enabled = true;
}

PixelizationEffect::~PixelizationEffect() = default;

void PixelizationEffect::Apply(const Framebuffer::Sptr & gBuffer)
{
	//Pixels = Application::Get().GetWindowSize().x ;
	_shader->Bind();
	_shader->SetUniform("u_Pixels_X", PixelsX);
	_shader->SetUniform("u_Pixels_Y", PixelsY);
	_shader->SetUniform("u_Power", Power);

	
	//_shader->SetUniform("u_PixelSize", glm::vec2(1.0f) / (glm::vec2)gBuffer->GetSize());
}

void PixelizationEffect::RenderImGui()
{
	//ImGui::PushID(this);
	//ImGui::SliderFloat("Pixelization Power", &Power, 0.0f, 1.0f);
	LABEL_LEFT(ImGui::SliderFloat, "Strength", &Power, 0, 1);


}

PixelizationEffect::Sptr PixelizationEffect::FromJson(const nlohmann::json& data)
{
	//PixelizationEffect::Sptr result = std::make_shared<PixelizationEffect>(false);

	return PixelizationEffect::Sptr();
}



nlohmann::json PixelizationEffect::ToJson() const
{
	return nlohmann::json();

}