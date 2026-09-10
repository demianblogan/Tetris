#pragma once

#include <vector>

#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

// A quiet field of drifting motes behind the board, in three depth layers that
// parallax against SceneMotion by different amounts -- the near layer slides
// most, the far layer barely at all -- so the gameplay scene reads with some
// depth. Additive, dim, no interaction. Drawn between the backdrop and the
// board.
class GameplayAmbience
{
public:
	GameplayAmbience();

	void Update(float deltaTime, sf::Vector2f sceneOffset);
	void Render(sf::RenderTarget& target) const;

private:
	struct Mote
	{
		sf::Vector2f base;      // drift position, before sway and parallax
		sf::Vector2f drift;     // px per second
		float radius = 1.f;
		float swayPhase = 0.f;
		float swaySpeed = 0.f;
		float swayAmp = 0.f;
		float twinklePhase = 0.f;
		float twinkleSpeed = 0.f;
		float baseAlpha = 0.f;
		float parallax = 1.f;
	};

	std::vector<Mote> motes;
	sf::Vector2f offset;
};
