#include "GameplayHud.h"

#include <string>
#include <string_view>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../utils/TimeFormat.h"

namespace
{
	// Board is at x 720..1200; the panels flank it.
	constexpr sf::FloatRect HoldPanel { { 66.f, 66.f },    { 240.f, 240.f } };
	constexpr sf::FloatRect LevelPanel{ { 66.f, 344.f },   { 260.f, 204.f } };
	constexpr sf::FloatRect NextPanel { { 1236.f, 66.f },  { 272.f, 336.f } };
	constexpr sf::FloatRect ScorePanel{ { 1236.f, 438.f }, { 408.f, 176.f } };
	constexpr sf::FloatRect StatsPanel{ { 1236.f, 652.f }, { 408.f, 204.f } };

	constexpr unsigned int CaptionSize = 30;
	constexpr unsigned int BigValueSize = 66;
	constexpr unsigned int StatValueSize = 46;

	constexpr float AccentBarHeight = 3.f;

	const sf::Color PanelFill{ 9, 12, 18, 212 };
	const sf::Color PanelOutline{ 46, 66, 88 };
	const sf::Color Accent{ 0, 200, 220 };
	const sf::Color CaptionColour{ 132, 150, 166 };
	const sf::Color BigValueColour{ 255, 255, 255 };
	const sf::Color StatValueColour{ 226, 232, 240 };

	[[nodiscard]] sf::Vector2f Centre(const sf::FloatRect& rect)
	{
		return { rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f };
	}

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}

	void DrawPanel(sf::RenderTarget& target, const sf::FloatRect& bounds)
	{
		sf::RectangleShape box(bounds.size);
		box.setPosition(bounds.position);
		box.setFillColor(PanelFill);
		box.setOutlineThickness(2.f);
		box.setOutlineColor(PanelOutline);
		target.draw(box);

		sf::RectangleShape bar({ bounds.size.x, AccentBarHeight });
		bar.setPosition(bounds.position);
		bar.setFillColor(Accent);
		target.draw(bar);
	}
}

GameplayHud::GameplayHud(Context& context)
	: context(context)
	, scoreValue(context.fonts.Get(Assets::FontID::Main), "0", BigValueSize)
	, levelValue(context.fonts.Get(Assets::FontID::Main), "1", BigValueSize)
	, linesValue(context.fonts.Get(Assets::FontID::Main), "0", StatValueSize)
	, timeValue(context.fonts.Get(Assets::FontID::Main), "0:00", StatValueSize)
	, nextPreviewCentre{ Centre(NextPanel).x, NextPanel.position.y + 206.f }
{
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	const auto caption = [&](std::string_view key, sf::Vector2f centre)
	{
		sf::Text text(font, context.localization.GetText(key), CaptionSize);
		text.setFillColor(CaptionColour);
		text.setLetterSpacing(1.35f);
		CentreText(text, centre);
		labels.push_back(std::move(text));
	};

	caption(TextKey::Hud::Hold, { Centre(HoldPanel).x, HoldPanel.position.y + 36.f });
	caption(TextKey::Hud::Next, { Centre(NextPanel).x, NextPanel.position.y + 36.f });
	caption(TextKey::Hud::Level, { Centre(LevelPanel).x, LevelPanel.position.y + 36.f });
	caption(TextKey::Hud::Score, { Centre(ScorePanel).x, ScorePanel.position.y + 34.f });
	caption(TextKey::Hud::Lines, { StatsPanel.position.x + StatsPanel.size.x * 0.27f, StatsPanel.position.y + 42.f });
	caption(TextKey::Hud::Time, { StatsPanel.position.x + StatsPanel.size.x * 0.73f, StatsPanel.position.y + 42.f });

	scoreValue.setFillColor(BigValueColour);
	levelValue.setFillColor(BigValueColour);
	linesValue.setFillColor(StatValueColour);
	timeValue.setFillColor(StatValueColour);

	Set(0, 1, 0, 0.f);
}

void GameplayHud::Set(int score, int level, int lines, float seconds)
{
	const auto place = [](sf::Text& text, const std::string& string, sf::Vector2f centre)
	{
		text.setString(string);
		CentreText(text, centre);
	};

	place(scoreValue, std::to_string(score), { Centre(ScorePanel).x, ScorePanel.position.y + 118.f });
	place(levelValue, std::to_string(level), { Centre(LevelPanel).x, LevelPanel.position.y + 130.f });
	place(linesValue, std::to_string(lines),
		{ StatsPanel.position.x + StatsPanel.size.x * 0.27f, StatsPanel.position.y + 126.f });
	place(timeValue, TimeFormat::MinutesSeconds(seconds),
		{ StatsPanel.position.x + StatsPanel.size.x * 0.73f, StatsPanel.position.y + 126.f });
}

void GameplayHud::Render(sf::RenderTarget& target) const
{
	DrawPanel(target, HoldPanel);
	DrawPanel(target, LevelPanel);
	DrawPanel(target, NextPanel);
	DrawPanel(target, ScorePanel);
	DrawPanel(target, StatsPanel);

	sf::RectangleShape divider({ 2.f, StatsPanel.size.y - 76.f });
	divider.setPosition({ Centre(StatsPanel).x, StatsPanel.position.y + 54.f });
	divider.setFillColor(PanelOutline);
	target.draw(divider);

	for (const sf::Text& label : labels)
	{
		target.draw(label);
	}

	target.draw(scoreValue);
	target.draw(levelValue);
	target.draw(linesValue);
	target.draw(timeValue);
}
