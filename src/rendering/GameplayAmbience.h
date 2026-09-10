#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

// A quiet field of drifting pixel flecks behind the board, in three depth
// layers that parallax against SceneMotion by different amounts -- the near
// layer slides most, the far layer barely at all. Square, snapped to a pixel
// grid and tinted from the tetromino palette so they sit inside the game's
// pixel look. Additive, dim, no interaction. Drawn between the backdrop and the
// board.
class GameplayAmbience
{
public:
	GameplayAmbience();

	void Update(float deltaTime, sf::Vector2f sceneOffset);
	void Render(sf::RenderTarget& target) const;

private:
	struct Fleck
	{
		sf::Vector2f base;      // drift position, before sway and parallax
		sf::Vector2f drift;     // px per second
		float size = 3.f;       // square side, a grid multiple
		float swayPhase = 0.f;
		float swaySpeed = 0.f;
		float swayAmp = 0.f;
		float twinklePhase = 0.f;
		float twinkleSpeed = 0.f;
		float baseAlpha = 0.f;
		float parallax = 1.f;
		sf::Color tint;
	};

	std::vector<Fleck> flecks;
	sf::Vector2f offset;
};
