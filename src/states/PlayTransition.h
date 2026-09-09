#pragma once

#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "../core/State.h"
#include "../rendering/NeonGlow.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
	class RenderTexture;
}

class GameplayState;

// The menu -> gameplay transition. The main menu has already played its ring
// exit and handed over a snapshot of its emptied ambient frame plus the box the
// "Play" entry occupied. This state fades that snapshot out over the arriving
// (still frozen) gameplay scene while a neon outline in the Play hue unfolds
// from the entry's box into the playfield frame, then hands the gameplay state
// over.
class PlayTransition final : public State
{
public:
	PlayTransition(Context& context, std::unique_ptr<sf::RenderTexture> menuSnapshot,
		sf::Vector2f fromCentre, sf::Vector2f fromSize, sf::Color accent);
	~PlayTransition() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] bool ShowsCursor() const override { return false; }

private:
	Context& context;
	std::unique_ptr<sf::RenderTexture> menuSnapshot;
	std::unique_ptr<GameplayState> gameplay;
	NeonGlow morphGlow;

	sf::Vector2f fromCentre;
	sf::Vector2f fromSize;
	sf::Color accent;

	float timer = 0.f;
	bool handedOver = false;
};
