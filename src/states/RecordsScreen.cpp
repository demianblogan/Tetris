#include "RecordsScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../statistics/HighScoreManager.h"
#include "../ui/ColourUtils.h"
#include "ScreenHost.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 260.f, 210.f }, { 1400.f, 710.f } };
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr float HeaderY = 272.f;
	constexpr float RuleY = 302.f;
	constexpr float RowTopY = 338.f;
	constexpr float RowStep = 57.f;

	// Column anchors: rank / score / lines / level are right-aligned to their
	// value, the name is left-aligned.
	constexpr float ColRankRight = 440.f;
	constexpr float ColNameLeft = 490.f;
	constexpr float ColScoreRight = 1226.f;
	constexpr float ColLinesRight = 1436.f;
	constexpr float ColLevelRight = 1596.f;

	constexpr unsigned int RankSize = 32;
	constexpr unsigned int NameSize = 42;
	constexpr unsigned int ScoreSize = 46;
	constexpr unsigned int SubSize = 34;
	constexpr unsigned int HeaderSize = 27;

	constexpr unsigned int ButtonTextSize = 42;
	constexpr sf::Vector2f ResetCentre{ 838.f, 968.f };
	constexpr sf::Vector2f BackCentre{ 1082.f, 968.f };

	constexpr float IntroDuration = 0.24f;
	constexpr float ExitDuration = 0.18f;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;
	constexpr float SelectedScale = 1.05f;
	constexpr float UnselectedAlpha = 0.5f;
	constexpr float ButtonGlowIntensity = 0.5f;
	constexpr float Pi = 3.14159265f;

	const sf::Color HeaderColour{ 162, 116, 202 };
	const sf::Color RankColour{ 150, 135, 165 };
	const sf::Color NameColour{ 232, 236, 244 };
	const sf::Color ChampionColour{ 255, 255, 255 };
	const sf::Color EmptyColour{ 120, 118, 130 };
	const sf::Color SubColour{ 158, 155, 172 };
	const sf::Color RuleColour{ 150, 90, 200, 150 };
	const sf::Color ResetHue{ 255, 162, 62 };   // the Options "Reset" orange
	const sf::Color BackHue{ 232, 236, 244 };

	[[nodiscard]] sf::Color Faded(sf::Color c, float alpha)
	{
		return sf::Color(c.r, c.g, c.b,
			static_cast<std::uint8_t>(static_cast<float>(c.a) * std::clamp(alpha, 0.f, 1.f)));
	}

	// Sets the text's origin so it sits at `x` (align -1 = left edge, +1 = right
	// edge) and is vertically centred on `y`.
	void PlaceCell(sf::Text& text, float x, float y, int align)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		const float originX = align < 0 ? bounds.position.x : bounds.position.x + bounds.size.x;
		text.setOrigin({ originX, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition({ x, y });
	}
}

RecordsScreen::RecordsScreen(ScreenHost& host, sf::Color accent)
	: MenuScreen(host)
	, accent(accent)
	, panel(context.textures.Get(Assets::TextureID::UiFramePurple), PanelBounds,
		UI::MenuFrameSourceBorder, PanelTargetBorder)
	, resetLabel(context.fonts.Get(Assets::FontID::Menu), ButtonTextSize)
	, backLabel(context.fonts.Get(Assets::FontID::Menu), ButtonTextSize)
	, buttonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, dialog(context.fonts.Get(Assets::FontID::Main), context.fonts.Get(Assets::FontID::Menu),
		context.textures.Get(Assets::TextureID::UiFrameWarning),
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur),
		context.audioPlayer)
{
	resetLabel.SetText(context.localization.GetText(TextKey::Records::Reset));
	backLabel.SetText(context.localization.GetText(TextKey::Records::Back));

	rule.setSize({ 1240.f, 2.f });
	rule.setPosition({ 360.f, RuleY });

	// TEMP: a sample leaderboard so the filled table can be reviewed. Only seeds
	// when the real board is empty, and is never saved. REMOVE before shipping.
	if (context.highScores.GetRecords().empty())
	{
		context.highScores.AddRecord({ sf::String("ALICE"), 128400, 612, 43 });
		context.highScores.AddRecord({ sf::String("BOBBY"), 95220, 478, 32 });
		context.highScores.AddRecord({ sf::String("CARMEN"), 74800, 401, 26 });
		context.highScores.AddRecord({ sf::String("DELTA FORCE"), 61050, 355, 21 });
		context.highScores.AddRecord({ sf::String("EVE"), 52300, 298, 18 });
		context.highScores.AddRecord({ sf::String("FRANK"), 40120, 244, 14 });
		context.highScores.AddRecord({ sf::String("GHOST"), 28900, 190, 10 });
		context.highScores.AddRecord({ sf::String("HANNAH"), 19600, 142, 7 });
		context.highScores.AddRecord({ sf::String("IVANWITHALONGNAME"), 11200, 95, 4 });
		context.highScores.AddRecord({ sf::String("K"), 3400, 38, 2 });
	}

	BuildHeader();
	RefreshRows();
}

void RecordsScreen::BuildHeader()
{
	headerCells.clear();
	const sf::Font& bodyFont = context.fonts.Get(Assets::FontID::Main);

	const auto add = [&](const sf::String& string, float x, int align)
	{
		sf::Text text(bodyFont, string, HeaderSize);
		text.setLetterSpacing(1.2f);
		text.setFillColor(HeaderColour);
		PlaceCell(text, x, HeaderY, align);
		headerCells.push_back({ std::move(text), HeaderColour });
	};

	add(sf::String("#"), ColRankRight, +1);
	add(context.localization.GetText(TextKey::Records::HeaderName), ColNameLeft, -1);
	add(context.localization.GetText(TextKey::Records::HeaderScore), ColScoreRight, +1);
	add(context.localization.GetText(TextKey::Records::HeaderLines), ColLinesRight, +1);
	add(context.localization.GetText(TextKey::Records::HeaderLevel), ColLevelRight, +1);
}

void RecordsScreen::RefreshRows()
{
	rowCells.clear();
	const sf::Font& bodyFont = context.fonts.Get(Assets::FontID::Main);
	const std::vector<HighScoreEntry>& records = context.highScores.GetRecords();
	const sf::Color scoreColour = UI::MixToWhite(accent, 0.5f);

	const auto add = [&](const sf::String& string, float x, float y, int align, unsigned int size, sf::Color colour)
	{
		sf::Text text(bodyFont, string, size);
		text.setFillColor(colour);
		PlaceCell(text, x, y, align);
		rowCells.push_back({ std::move(text), colour });
	};

	for (std::size_t rank = 0; rank < HighScoreManager::MAX_RECORDS; ++rank)
	{
		const float y = RowTopY + static_cast<float>(rank) * RowStep;
		const bool present = rank < records.size();

		add(std::to_string(rank + 1) + ".", ColRankRight, y, +1, RankSize, RankColour);

		if (!present)
		{
			add(sf::String("-"), ColNameLeft, y, -1, NameSize, EmptyColour);
			continue;
		}

		const HighScoreEntry& entry = records[rank];
		add(entry.playerName, ColNameLeft, y, -1, NameSize, rank == 0 ? ChampionColour : NameColour);
		add(std::to_string(entry.score), ColScoreRight, y, +1, ScoreSize, scoreColour);
		add(std::to_string(entry.lines), ColLinesRight, y, +1, SubSize, SubColour);
		add(std::to_string(entry.level), ColLevelRight, y, +1, SubSize, SubColour);
	}
}

void RecordsScreen::PlayIntro()
{
	introTime = 0.f;
}

void RecordsScreen::StartExit()
{
	if (exitTime < 0.f)
	{
		exitTime = 0.f;
	}
}

bool RecordsScreen::ExitFinished() const
{
	return exitTime >= ExitDuration;
}

float RecordsScreen::PanelAlpha() const
{
	if (exitTime >= 0.f)
	{
		return std::clamp(1.f - exitTime / ExitDuration, 0.f, 1.f);
	}
	return std::clamp(introTime / IntroDuration, 0.f, 1.f);
}

void RecordsScreen::Leave()
{
	if (leaving)
	{
		return;
	}

	leaving = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	host.BeginBack();
}

void RecordsScreen::Activate()
{
	pressTime = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);

	if (focus == Focus::Reset)
	{
		dialog.Show(context.localization.GetText(TextKey::Records::ConfirmReset),
			context.localization.GetText(TextKey::Common::Yes),
			context.localization.GetText(TextKey::Common::No));
	}
	else
	{
		Leave();
	}
}

void RecordsScreen::DoReset()
{
	context.highScores.Clear();
	context.highScores.Save();
	RefreshRows();
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.7f);
}

void RecordsScreen::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
		return;
	}

	if (dialog.IsOpen())
	{
		dialog.Navigate(MenuInput::Resolve(event, context.gamepad));
		return;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Left:
	case MenuInput::Action::Right:
		focus = focus == Focus::Reset ? Focus::Back : Focus::Reset;
		context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		return;
	case MenuInput::Action::Confirm:
		Activate();
		return;
	case MenuInput::Action::Back:
		Leave();
		return;
	default:
		break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		if (resetLabel.Bounds(ResetCentre, 1.f).contains(point))
		{
			focus = Focus::Reset;
		}
		else if (backLabel.Bounds(BackCentre, 1.f).contains(point))
		{
			focus = Focus::Back;
		}
	}
	else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button != sf::Mouse::Button::Left)
		{
			return;
		}

		const sf::Vector2f point = context.window.mapPixelToCoords(clicked->position);
		if (resetLabel.Bounds(ResetCentre, 1.f).contains(point))
		{
			focus = Focus::Reset;
			Activate();
		}
		else if (backLabel.Bounds(BackCentre, 1.f).contains(point))
		{
			focus = Focus::Back;
			Activate();
		}
	}
}

void RecordsScreen::Update(float deltaTime)
{
	introTime += deltaTime;
	pressTime += deltaTime;
	if (exitTime >= 0.f)
	{
		exitTime += deltaTime;
	}

	const bool interactive = !dialog.IsOpen() && !leaving;
	resetLabel.SetWaveEnabled(interactive && focus == Focus::Reset);
	backLabel.SetWaveEnabled(interactive && focus == Focus::Back);
	resetLabel.Update(deltaTime);
	backLabel.Update(deltaTime);
	buttonGlow.Update(deltaTime);

	dialog.Update(deltaTime);
	if (const std::optional<bool> answer = dialog.TakeResult())
	{
		if (*answer)
		{
			DoReset();
		}
	}
}

void RecordsScreen::DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f centre,
	sf::Color hue, bool selected, float alpha)
{
	const float press = (selected && pressTime < PressDuration)
		? std::sin(std::clamp(1.f - pressTime / PressDuration, 0.f, 1.f) * Pi)
		: 0.f;
	const float scale = (selected ? SelectedScale : 1.f) + PressPunch * press;
	const float drawAlpha = alpha * (selected ? 1.f : UnselectedAlpha);

	if (selected)
	{
		const auto glowAlpha = static_cast<std::uint8_t>(
			std::clamp(alpha, 0.f, 1.f) * 255.f * ButtonGlowIntensity);
		label.DrawGlow(target, buttonGlow, centre, scale, sf::Color(hue.r, hue.g, hue.b, glowAlpha));
	}

	label.Draw(target, centre, scale, hue, drawAlpha, PressFlash * press);
}

void RecordsScreen::Render(sf::RenderTarget& target)
{
	const float alpha = PanelAlpha();

	panel.SetColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * 255.f)));
	panel.Draw(target);

	if (alpha > 0.f)
	{
		for (Cell& cell : headerCells)
		{
			cell.text.setFillColor(Faded(cell.base, alpha));
			target.draw(cell.text);
		}

		rule.setFillColor(Faded(RuleColour, alpha));
		target.draw(rule);

		for (Cell& cell : rowCells)
		{
			cell.text.setFillColor(Faded(cell.base, alpha));
			target.draw(cell.text);
		}
	}

	DrawButton(target, resetLabel, ResetCentre, ResetHue, !dialog.IsOpen() && focus == Focus::Reset, alpha);
	DrawButton(target, backLabel, BackCentre, BackHue, !dialog.IsOpen() && focus == Focus::Back, alpha);

	dialog.Render(target);
}
