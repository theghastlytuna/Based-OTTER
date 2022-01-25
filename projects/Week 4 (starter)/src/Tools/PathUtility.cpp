/*
NOU Framework - Created for INFR 2310 at Ontario Tech.
(c) Samantha Stahlke 2020

PathUtility.cpp
Simple utility for sampling paths and drawing lines with OpenGL.

As a convention in NOU, we put "C" before a class name to signify
that we intend the class for use as a component with the ENTT framework.
*/

#include "PathUtility.h"
#include "NOU/Mesh.h"
#include "NOU/CCamera.h"

namespace nou
{
	const int PathSampler::NUM_SAMPLES = 8;
	const float PathSampler::SAMPLE_T = 1.0f / static_cast<float>(PathSampler::NUM_SAMPLES);
	PathSampler::SegmentInterpolator PathSampler::Lerp = nullptr;
	PathSampler::CurveInterpolator PathSampler::Catmull = nullptr;
	PathSampler::CurveInterpolator PathSampler::Bezier = nullptr;

	PathSampler::PathSampler()
	{
		m_mode = PathMode::BEZIER;
		m_samples.push_back(glm::vec3(0.0f));
	}

	void PathSampler::Resample(const KeypointSet& keypoints)
	{
		CurveInterpolator interp = (m_mode == PathMode::CATMULL) ? Catmull : Bezier;

		if (interp == nullptr)
			m_samples.clear();

		if (m_samples.size() == 0)
			m_samples.push_back(glm::vec3(0.0f));
	}

	const std::vector<glm::vec3>& PathSampler::GetSamples() const
	{
		return m_samples;
	}

	CLineRenderer::CLineRenderer(Entity& owner, PathSampler& pathSource, Material& mat)
	{
		m_owner = &owner;
		m_pathSource = &pathSource;
		m_mat = &mat;

		m_vao = std::make_unique<VertexArray>();
		m_vao->SetDrawMode(VertexArray::DrawMode::LINE_STRIP);

		m_vbo = std::make_unique<VertexBuffer>(3, m_pathSource->GetSamples(), true);

		m_vao->BindAttrib(*m_vbo, (GLint)(Mesh::Attrib::POSITION));
	}

	void CLineRenderer::Draw(const PathSampler::KeypointSet& keypoints)
	{
		m_pathSource->Resample(keypoints);
		m_vbo->UpdateData(m_pathSource->GetSamples());
		m_mat->Use();

		//We are assuming the names used by uniform shader variables as a convention here.
		//In a larger project, we would have a more elegant system for registering
		//or even automatically detecting uniform names.
		ShaderProgram::Current()->SetUniform("viewproj", CCamera::current->Get<CCamera>().GetVP());
		ShaderProgram::Current()->SetUniform("model", m_owner->transform.GetGlobal());

		m_vao->Draw();
	}
}