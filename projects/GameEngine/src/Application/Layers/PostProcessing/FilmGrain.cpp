#include "FilmGrain.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

FilmGrain::FilmGrain() :
	PostProcessingLayer::Effect(),
	_noise(nullptr),
	_shader(nullptr)
{
	Name = "Film Grain";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/filmGrain.glsl" }
	});

	_noise = ResourceManager::CreateAsset<Texture2D>("textures/noise_texture_0001.png");
}

FilmGrain::~FilmGrain() = default;

void FilmGrain::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();
	_noise->Bind(1);
}

void FilmGrain::RenderImGui()
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

FilmGrain::Sptr FilmGrain::FromJson(const nlohmann::json & data)
{
	FilmGrain::Sptr result = std::make_shared<FilmGrain>();
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

nlohmann::json FilmGrain::ToJson() const
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
