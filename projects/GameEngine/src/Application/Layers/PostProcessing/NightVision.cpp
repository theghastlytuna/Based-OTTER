#include "NightVision.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

NightVision::NightVision() :
	PostProcessingLayer::Effect(),
	_noise(nullptr),
	_shader(nullptr)
{
	Name = "Night Vision";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/NightVision.glsl" }
	});

	_noise = ResourceManager::CreateAsset<Texture2D>("textures/noise_texture_0001.png");

	Enabled = false;
}

NightVision::~NightVision() = default;

void NightVision::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();
	_noise->Bind(3);
	gBuffer->BindAttachment(RenderTargetAttachment::Depth, 1);
	gBuffer->BindAttachment(RenderTargetAttachment::Color1, 2); // The normal buffer
}

void NightVision::RenderImGui()
{
	/*
	LABEL_LEFT(ImGui::ColorEdit4, "Color", &_outlineColor.x);
	LABEL_LEFT(ImGui::SliderFloat, "Scale", &_scale, 0.1f, 10.0f);
	LABEL_LEFT(ImGui::SliderFloat, "Depth Threshold", &_depthThreshold, 0.0f, 1.0f);
	LABEL_LEFT(ImGui::SliderFloat, "Norm. Threshold", &_normalThreshold, 0.0f, 1.0f);
	LABEL_LEFT(ImGui::SliderFloat, "Depth Norm. Threshold", &_depthNormalThreshold, 0.0f, 1.0f);
	LABEL_LEFT(ImGui::SliderFloat, "Depth Norm. Threshold Scale", &_depthNormalThresholdScale, 0.0f, 10.0f);
	*/
}

NightVision::Sptr NightVision::FromJson(const nlohmann::json & data)
{
	NightVision::Sptr result = std::make_shared<NightVision>();
	result->Enabled = JsonGet(data, "enabled", true);
	/*
	result->_outlineColor = JsonGet(data, "color", result->_outlineColor);
	result->_scale = JsonGet(data, "scale", result->_scale);
	result->_depthThreshold = JsonGet(data, "depth_threshold", result->_depthThreshold);
	result->_normalThreshold = JsonGet(data, "normal_threshold", result->_normalThreshold);
	result->_depthNormalThreshold = JsonGet(data, "depth_normal_threshold", result->_depthNormalThreshold);
	result->_depthNormalThresholdScale = JsonGet(data, "depth_normal_threshold_scale", result->_depthNormalThresholdScale);
	*/
	return result;
}

nlohmann::json NightVision::ToJson() const
{
	return {
		{ "enabled", Enabled },
		/*
		{ "color", _outlineColor },
		{ "scale", _scale },
		{ "depth_threshold", _depthThreshold },
		{ "normal_threshold", _normalThreshold },
		{ "depth_normal_threshold", _depthNormalThreshold },
		{ "depth_normal_threshold_scale", _depthNormalThresholdScale },
		*/
	};
}
