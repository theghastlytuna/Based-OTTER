#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Framebuffer.h"

class PixelizationEffect : public PostProcessingLayer::Effect
{
public:
	MAKE_PTRS(PixelizationEffect);
	int PixelsX;
	int PixelsY;
	float Power;

	PixelizationEffect();
	virtual ~PixelizationEffect();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	// Inherited from IResource

	PixelizationEffect::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;
};

