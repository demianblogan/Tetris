#include "GameplayHud.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../input/KeyName.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/GameSettings.h"
#include "../settings/SettingsManager.h"
#include "../ui/Easing.h"
#include "../utils/TimeFormat.h"

namespace
{
	// The board sits at x 720..1200, y 60..1020. The HUD hugs the well: a narrow
	// gap keeps every panel close so the eye barely has to travel off the stack.
	constexpr float WellLeft = 720.f;
	constexpr float WellRight = 1200.f;
	constexpr float ScreenHeight = 1080.f;

	constexpr float WellGap = 26.f;
	constexpr float Square = 210.f;
	constexpr float Gap = 24.f;

	constexpr float ColumnTop = (ScreenHeight - (4.f * Square + 3.f * Gap)) * 0.5f;
	constexpr float LeftX = WellLeft - WellGap - Square;
	constexpr float RightX = WellRight + WellGap;

	constexpr sf::FloatRect LegendBounds{ { LeftX, ColumnTop + 2.f * (Square + Gap) }, { Square, 2.f * Square + Gap } };

	constexpr unsigned int CaptionSize = 38;
	constexpr unsigned int ValueSize = 54;
	constexpr unsigned int LegendTitleSize = 30;
	constexpr unsigned int LegendActionSize = 22;
	constexpr unsigned int LegendKeySize = 26;

	constexpr sf::Vector2f FrameTargetBorder{ 32.f, 32.f };
	constexpr float FillInset = 16.f;

	constexpr float FlashDuration = 0.5f;

	const sf::Color FillColour{ 8, 11, 17, 214 };
	const sf::Color CaptionColour{ 150, 172, 196 };
	const sf::Color ValueColour{ 255, 255, 255 };
	const sf::Color FlashColour{ 120, 230, 255 };
	const sf::Color LegendActionColour{ 146, 162, 178 };
	const sf::Color LegendKeyColour{ 236, 240, 246 };

	[[nodiscard]] sf::Vector2f Centre(const sf::FloatRect& rect)
	{
		return { rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f };
	}

	[[nodiscard]] sf::FloatRect SquareAt(float x, int row)
	{
		return { { x, ColumnTop + static_cast<float>(row) * (Square + Gap) }, { Square, Square } };
	}

	[[nodiscard]] sf::Color MixColour(sf::Color from, sf::Color to, float t)
	{
		return sf::Color(
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.r), static_cast<float>(to.r), t)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.g), static_cast<float>(to.g), t)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.b), static_cast<float>(to.b), t)),
			static_cast<std::uint8_t>(UI::Easing::Lerp(static_cast<float>(from.a), static_cast<float>(to.a), t)));
	}

	void CentreText(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}

	// Cell indices, in build order.
	enum CellId : std::size_t { Hold = 0, Level = 1, Next = 2, Score = 3, Lines = 4, Time = 5 };

	[[nodiscard]] sf::Vector2f ValueCentre(std::size_t cell)
	{
		const bool left = cell == Level;
		const int row = left ? 1 : static_cast<int>(cell) - static_cast<int>(Next);
		const sf::FloatRect bounds = SquareAt(left ? LeftX : RightX, row);
		return { Centre(bounds).x, bounds.position.y + Square * 0.62f };
	}
}

GameplayHud::Cell GameplayHud::MakeCell(std::string_view captionKey, sf::FloatRect bounds)
{
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	sf::RectangleShape fill({ bounds.size.x - FillInset * 2.f, bounds.size.y - FillInset * 2.f });
	fill.setPosition({ bounds.position.x + FillInset, bounds.position.y + FillInset });
	fill.setFillColor(FillColour);

	UI::NineSliceFrame frame(context.textures.Get(Assets::TextureID::UiFrameBlue), bounds,
		UI::MenuFrameSourceBorder, FrameTargetBorder);

	sf::Text caption(font, context.localization.GetText(captionKey), CaptionSize);
	caption.setFillColor(CaptionColour);
	caption.setLetterSpacing(1.4f);
	CentreText(caption, { Centre(bounds).x, bounds.position.y + 42.f });

	return { std::move(fill), std::move(frame), std::move(caption), 0.f };
}

GameplayHud::GameplayHud(Context& context)
	: context(context)
	, scoreValue(context.fonts.Get(Assets::FontID::Main), "0", ValueSize)
	, levelValue(context.fonts.Get(Assets::FontID::Main), "1", ValueSize)
	, linesValue(context.fonts.Get(Assets::FontID::Main), "0", ValueSize)
	, timeValue(context.fonts.Get(Assets::FontID::Main), "0:00", ValueSize)
	, legendFrame(context.textures.Get(Assets::TextureID::UiFrameBlue), LegendBounds,
		UI::MenuFrameSourceBorder, FrameTargetBorder)
	, legendTitle(context.fonts.Get(Assets::FontID::Main),
		context.localization.GetText(TextKey::Hud::Controls), LegendTitleSize)
	, nextPreviewCentre{ Centre(SquareAt(RightX, 0)).x, SquareAt(RightX, 0).position.y + Square * 0.56f }
{
	cells.push_back(MakeCell(TextKey::Hud::Hold, SquareAt(LeftX, 0)));
	cells.push_back(MakeCell(TextKey::Hud::Level, SquareAt(LeftX, 1)));
	cells.push_back(MakeCell(TextKey::Hud::Next, SquareAt(RightX, 0)));
	cells.push_back(MakeCell(TextKey::Hud::Score, SquareAt(RightX, 1)));
	cells.push_back(MakeCell(TextKey::Hud::Lines, SquareAt(RightX, 2)));
	cells.push_back(MakeCell(TextKey::Hud::Time, SquareAt(RightX, 3)));

	scoreValue.setFillColor(ValueColour);
	levelValue.setFillColor(ValueColour);
	linesValue.setFillColor(ValueColour);
	timeValue.setFillColor(ValueColour);

	Set(0, 1, 0, 0.f);

	// Controls legend under the left column: one entry per action, key names read
	// once from the live bindings (layout-independent, via Input::KeyName).
	legendFill.setSize({ LegendBounds.size.x - FillInset * 2.f, LegendBounds.size.y - FillInset * 2.f });
	legendFill.setPosition({ LegendBounds.position.x + FillInset, LegendBounds.position.y + FillInset });
	legendFill.setFillColor(FillColour);

	legendTitle.setFillColor(CaptionColour);
	legendTitle.setLetterSpacing(1.4f);
	CentreText(legendTitle, { Centre(LegendBounds).x, LegendBounds.position.y + 40.f });

	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const ControlSettings& controls = context.settings.GetSettings().controls;

	const auto twoKeys = [](sf::Keyboard::Scancode a, sf::Keyboard::Scancode b)
	{
		return Input::KeyName(a) + sf::String("  ") + Input::KeyName(b);
	};

	const std::array<std::pair<std::string_view, sf::String>, 5> entries =
	{ {
		{ TextKey::Hud::Move,     twoKeys(controls.moveLeft, controls.moveRight) },
		{ TextKey::Hud::SoftDrop, Input::KeyName(controls.softDrop) },
		{ TextKey::Hud::HardDrop, Input::KeyName(controls.hardDrop) },
		{ TextKey::Hud::Rotate,   twoKeys(controls.rotateCounterClockwise, controls.rotateClockwise) },
		{ TextKey::Hud::Pause,    Input::KeyName(controls.pause) },
	} };

	const float entriesTop = LegendBounds.position.y + 86.f;
	const float entryStep = (LegendBounds.size.y - 104.f) / static_cast<float>(entries.size());
	const float centreX = Centre(LegendBounds).x;

	for (std::size_t i = 0; i < entries.size(); i++)
	{
		const float y = entriesTop + entryStep * (static_cast<float>(i) + 0.5f);

		ControlEntry entry{
			sf::Text(font, context.localization.GetText(entries[i].first), LegendActionSize),
			sf::Text(font, entries[i].second, LegendKeySize)
		};
		entry.action.setFillColor(LegendActionColour);
		entry.action.setLetterSpacing(1.2f);
		entry.keys.setFillColor(LegendKeyColour);
		CentreText(entry.action, { centreX, y - 15.f });
		CentreText(entry.keys, { centreX, y + 15.f });

		legendEntries.push_back(std::move(entry));
	}
}

void GameplayHud::Set(int score, int level, int lines, float seconds)
{
	scoreValue.setString(std::to_string(score));
	levelValue.setString(std::to_string(level));
	linesValue.setString(std::to_string(lines));
	timeValue.setString(TimeFormat::MinutesSeconds(seconds));

	CentreText(scoreValue, ValueCentre(Score));
	CentreText(levelValue, ValueCentre(Level));
	CentreText(linesValue, ValueCentre(Lines));
	CentreText(timeValue, ValueCentre(Time));
}

void GameplayHud::Update(float deltaTime)
{
	for (Cell& cell : cells)
	{
		cell.flash = std::max(0.f, cell.flash - deltaTime / FlashDuration);
	}
}

void GameplayHud::OnRowsCleared()
{
	cells[Score].flash = 1.f;
	cells[Lines].flash = 1.f;
}

void GameplayHud::OnLevelUp()
{
	cells[Level].flash = 1.f;
}

void GameplayHud::DrawValue(sf::RenderTarget& target, const sf::Text& value, float flash) const
{
	if (flash <= 0.f)
	{
		target.draw(value);
		return;
	}

	const float ease = UI::Easing::EaseOutCubic(flash);

	sf::Text lit = value;
	lit.setScale({ 1.f + 0.18f * ease, 1.f + 0.18f * ease });
	lit.setFillColor(MixColour(ValueColour, FlashColour, ease));
	target.draw(lit);
}

void GameplayHud::DrawCell(sf::RenderTarget& target, const Cell& cell) const
{
	target.draw(cell.fill);
	cell.frame.Draw(target);

	if (cell.flash > 0.f)
	{
		const float ease = UI::Easing::EaseOutCubic(cell.flash);

		sf::RectangleShape glow = cell.fill;
		glow.setFillColor(MixColour(sf::Color(FlashColour.r, FlashColour.g, FlashColour.b, 0),
			sf::Color(FlashColour.r, FlashColour.g, FlashColour.b, 60), ease));
		glow.setOutlineThickness(3.f);
		glow.setOutlineColor(sf::Color(FlashColour.r, FlashColour.g, FlashColour.b,
			static_cast<std::uint8_t>(ease * 230.f)));
		target.draw(glow);
	}

	target.draw(cell.caption);
}

void GameplayHud::Render(sf::RenderTarget& target) const
{
	for (const Cell& cell : cells)
	{
		DrawCell(target, cell);
	}

	DrawValue(target, scoreValue, cells[Score].flash);
	DrawValue(target, levelValue, cells[Level].flash);
	DrawValue(target, linesValue, cells[Lines].flash);
	DrawValue(target, timeValue, cells[Time].flash);

	if (showControls)
	{
		target.draw(legendFill);
		legendFrame.Draw(target);
		target.draw(legendTitle);

		for (const ControlEntry& entry : legendEntries)
		{
			target.draw(entry.action);
			target.draw(entry.keys);
		}
	}
}
