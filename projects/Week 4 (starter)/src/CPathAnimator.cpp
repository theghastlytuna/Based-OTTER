/*
NOU Framework - Created for INFR 2310 at Ontario Tech.
(c) Samantha Stahlke 2020

CPathAnimator.cpp
Simple component for animating an object along a path.

As a convention in NOU, we put "C" before a class name to signify
that we intend the class for use as a component with the ENTT framework.
*/

#include "CPathAnimator.h"

namespace nou
{
	CPathAnimator::CPathAnimator(Entity& owner)
	{
		m_owner = &owner;

		m_segmentIndex = 0;
		m_segmentTimer = 0.0f;

		m_segmentTravelTime = 1.0f;

		// Change this to test your update function
		m_mode = PathSampler::PathMode::BEZIER;
		// PathSampler::PathMode::CATMULL
		// PathSampler::PathMode::BEZIER
	}

	void CPathAnimator::SetMode(PathSampler::PathMode mode)
	{
		m_mode = mode;
		m_segmentIndex = 0;
	}

	void CPathAnimator::Update(const PathSampler::KeypointSet& keypoints, float deltaTime)
	{
		if (keypoints.size() == 0 || m_segmentTravelTime == 0)
			return;

		else
		{
			switch (m_mode)
			{
			case PathSampler::PathMode::LERP:
			{
				m_segmentTimer += deltaTime;

				//Ensure we are not "over time" and move to the next segment
				//if necessary.
				while (m_segmentTimer > m_segmentTravelTime)
				{
					m_segmentTimer -= m_segmentTravelTime;

					m_segmentIndex += 1;

					if (m_segmentIndex >= keypoints.size())
						m_segmentIndex = 0;
				}

				float t = m_segmentTimer / m_segmentTravelTime;

				// LERP

				// Need at least 2 points for 1 segment
				if (keypoints.size() < 2)
				{
					m_owner->transform.m_pos = keypoints[0]->transform.m_pos;
					return;
				}

				glm::vec3 p0, p1;
				size_t p0_index, p1_index;

				p1_index = m_segmentIndex;
				p0_index = (p1_index == 0) ? keypoints.size() - 1 : p1_index - 1;

				p0 = keypoints[p0_index]->transform.m_pos;
				p1 = keypoints[p1_index]->transform.m_pos;

				m_owner->transform.m_pos = PathSampler::Lerp(p0, p1, t);
			}
			break;

			case PathSampler::PathMode::CATMULL:
			{
				m_segmentTimer += deltaTime;

				//Ensure we are not "over time" and move to the next segment
				//if necessary.
				while (m_segmentTimer > m_segmentTravelTime)
				{
					m_segmentTimer -= m_segmentTravelTime;

					m_segmentIndex += 1;

					if (m_segmentIndex >= keypoints.size())
						m_segmentIndex = 0;
				}

				float t = m_segmentTimer / m_segmentTravelTime;

				// Neither Catmull nor Bezier make sense with less than 4 points.
				if (keypoints.size() < 4)
				{
					m_owner->transform.m_pos = keypoints[0]->transform.m_pos;
					return;
				}

				size_t p0_index, p1_index, p2_index, p3_index;
				glm::vec3 p0, p1, p2, p3;

				// TODO: Complete this function
			}
			break;

			case PathSampler::PathMode::BEZIER:
			{
				m_segmentTimer += deltaTime;

				//Ensure we are not "over time" and move to the next segment
				//if necessary.
				while (m_segmentTimer > m_segmentTravelTime)
				{
					m_segmentTimer -= m_segmentTravelTime;

					m_segmentIndex += 1;

					if (m_segmentIndex >= keypoints.size())
						m_segmentIndex = 0;
				}

				float t = m_segmentTimer / m_segmentTravelTime;

				// Neither Catmull nor Bezier make sense with less than 4 points.
				if (keypoints.size() < 4)
				{
					m_owner->transform.m_pos = keypoints[0]->transform.m_pos;
					return;
				}

				size_t p0_index, p1_index, p2_index, p3_index;
				glm::vec3 p0, p1, p2, p3;

				// TODO: Complete this function
				//p0 and p3 are endpoints
				//p1 and p2 are the tangents

				p0_index = m_segmentIndex - 1;
				p1_index = p0_index + 1;
				p2_index = p0_index + 2;
				p3_index = p0_index + 3;

				p0 = keypoints[p0_index]->transform.m_pos;
				p1 = keypoints[p1_index]->transform.m_pos;
				p2 = keypoints[p2_index]->transform.m_pos;
				p3 = keypoints[p3_index]->transform.m_pos;

				m_owner->transform.m_pos = PathSampler::Bezier(p0, p1, p2, p3, t);
			}
			break;
			}
		}
	}
}