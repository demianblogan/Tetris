#pragma once

#include <vector>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

struct Context;

namespace sf
{
	class RenderTarget;
}

// The in-game HUD: dark framed panels flanking the board -- HOLD (an empty slot
// until the mechanic lands), NEXT, SCORE, LINES, LEVEL, TIME. GameplayState
// pushes the numbers in each frame with Set(); BoardRenderer draws the next
// piece at NextPreviewCentre().
class GameplayHud
{
public:
	explicit GameplayHud(Context& context);

	void Set(int score, int level, int lines, float seconds);
	void SetControlsLegendVisible(bool visible) { showControls = visible; }
	void Render(sf::RenderTarget& target) const;

	[[nodiscard]] sf::Vector2f NextPreviewCentre() const { return nextPreviewCentre; }

private:
	Context& context;

	std::vector<sf::Text> labels;   // the fixed captions, positioned once

	sf::Text scoreValue;
	sf::Text levelValue;
	sf::Text linesValue;
	sf::Text timeValue;

	// The always-on controls legend, bottom-left. A row is an action name plus
	// the key(s) bound to it, read once from the settings.
	struct ControlRow
	{
		sf::Text action;
		sf::Text keys;
	};
	sf::Text controlsTitle;
	std::vector<ControlRow> controlRows;
	bool showControls = true;

	sf::Vector2f nextPreviewCentre;
};
