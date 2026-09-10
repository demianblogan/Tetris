#include "SceneMotion.h"

#include <algorithm>
#include <cmath>

namespace
{
	// Spring pulling the offset back to rest.
	constexpr float Stiffness = 26.f;
	constexpr float Damping = 7.5f;

	// Hard limit so a flurry of nudges can't slide the backdrop off its overscan.
	constexpr float MaxX = 46.f;
	constexpr float MaxY = 30.f;

	// The always-on drift.
	constexpr float IdleAmpX = 7.f;
	constexpr float IdleAmpY = 4.5f;

	[[nodiscard]] float Clamp(float value, float limit)
	{
		return std::clamp(value, -limit, limit);
	}
}

void SceneMotion::Nudge(sf::Vector2f impulse)
{
	velocity += impulse;
}

void SceneMotion::Update(float deltaTime)
{
	// Semi-implicit Euler on a damped spring toward the origin.
	velocity += (-Stiffness * position - Damping * velocity) * deltaTime;
	position += velocity * deltaTime;

	position.x = Clamp(position.x, MaxX);
	position.y = Clamp(position.y, MaxY);

	idleTime += deltaTime;
}

sf::Vector2f SceneMotion::Offset() const
{
	const sf::Vector2f drift{
		std::sin(idleTime * 0.13f) * IdleAmpX,
		std::sin(idleTime * 0.09f + 1.7f) * IdleAmpY };

	return position + drift;
}
