#include "ColorCorrectionEffect.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Application/Application.h"
#include "Application/Layers/RenderLayer.h"

ColorCorrectionEffect::ColorCorrectionEffect() :
	ColorCorrectionEffect(true) { }

ColorCorrectionEffect::ColorCorrectionEffect(bool defaultLut) :
	PostProcessingLayer::Effect(),
	_shader(nullptr),
	_strength(1.0f),
	Lut1(nullptr),
	Lut2(nullptr),
	Lut3(nullptr),
	_selection(0.0)
{
	Name = "Color Correction";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/color_correction.glsl" }
	});
	
	Lut1 = ResourceManager::CreateAsset<Texture3D>("luts/Desert.CUBE");
	Lut2 = ResourceManager::CreateAsset<Texture3D>("luts/Coolish.CUBE");
	Lut3 = ResourceManager::CreateAsset<Texture3D>("luts/shrooms.cube");
	
}

ColorCorrectionEffect::~ColorCorrectionEffect() = default;

void ColorCorrectionEffect::Apply(const Framebuffer::Sptr& gBuffer)
{
	_shader->Bind();
	Lut1->Bind(1);
	Lut2->Bind(2);
	Lut3->Bind(3);
	_shader->SetUniform("u_Strength", _strength);
	_shader->SetUniform("u_Selection", _selection);
}

void ColorCorrectionEffect::RenderImGui()
{
	LABEL_LEFT(ImGui::LabelText, "LUT", Lut1 ? Lut1->GetDebugName().c_str() : "none");
	LABEL_LEFT(ImGui::SliderFloat, "Strength", &_strength, 0, 1);
	LABEL_LEFT(ImGui::SliderFloat, "Selection", &_selection, -1, 3);
}

ColorCorrectionEffect::Sptr ColorCorrectionEffect::FromJson(const nlohmann::json& data)
{
	ColorCorrectionEffect::Sptr result = std::make_shared<ColorCorrectionEffect>(false);
	result->Enabled = JsonGet(data, "enabled", true);
	result->_strength = JsonGet(data, "strength", result->_strength);
	result->Lut1 = ResourceManager::Get<Texture3D>(Guid(data["lut"].get<std::string>()));
	return result;
}

nlohmann::json ColorCorrectionEffect::ToJson() const
{
	return {
		{ "enabled", Enabled },
		{ "lut", Lut1 != nullptr ? Lut1->GetGUID().str() : "null" }, 
		{ "strength", _strength }
	};
}
