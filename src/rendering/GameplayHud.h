#pragma once

#include <string_view>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "../ui/NineSliceFrame.h"

struct Context;

namespace sf
{
	class RenderTarget;
}

// The in-game HUD: square nine-slice frames down each side of the board --
// HOLD and LEVEL on the left, NEXT / SCORE / LINES / TIME on the right -- plus
// an always-on controls legend under the left column. GameplayState pushes the
// numbers each frame with Set(); BoardRenderer draws the next piece at
// NextPreviewCentre().
class GameplayHud
{
public:
	explicit GameplayHud(Context& context);

	void Set(int score, int level, int lines, float seconds);
	void SetControlsLegendVisible(bool visible) { showControls = visible; }
	void Render(sf::RenderTarget& target) const;

	[[nodiscard]] sf::Vector2f NextPreviewCentre() const { return nextPreviewCentre; }

private:
	// One framed square: the decorative border, a dark inner fill, and the
	// caption pinned near its top.
	struct Cell
	{
		sf::RectangleShape fill;
		UI::NineSliceFrame frame;
		sf::Text caption;
	};

	// A controls-legend line: an action name on the left, its key(s) on the right.
	struct ControlRow
	{
		sf::Text action;
		sf::Text keys;
	};

	Cell MakeCell(std::string_view captionKey, sf::FloatRect bounds);

	Context& context;

	std::vector<Cell> cells;

	sf::Text scoreValue;
	sf::Text levelValue;
	sf::Text linesValue;
	sf::Text timeValue;

	UI::NineSliceFrame legendFrame;
	sf::RectangleShape legendFill;
	sf::Text legendTitle;
	std::vector<ControlRow> legendRows;
	bool showControls = true;

	sf::Vector2f nextPreviewCentre;
};
