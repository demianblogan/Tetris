#include "GameOverState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
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
#include "../ui/Easing.h"
#include "GameplayState.h"
#include "MenuShell.h"

namespace
{
	constexpr sf::Vector2f Screen{ 1920.f, 1080.f };
	constexpr float CentreX = 960.f;

	constexpr sf::Vector2f PanelPos{ 260.f, 130.f };
	constexpr float PanelW = 1400.f;
	constexpr float PanelHRecord = 800.f;
	constexpr float PanelHPlain = 600.f;
	constexpr sf::Vector2f PanelTargetBorder{ 46.f, 46.f };

	constexpr float HeadingY = 246.f;
	constexpr float ScoreLabelY = 380.f;
	constexpr float ScoreValueY = 476.f;
	constexpr float StatRowY = 610.f;
	constexpr float StatValueDrop = 66.f;
	constexpr float StatSpread = 400.f;
	constexpr float BadgeY = 770.f;
	constexpr float NameY = 858.f;
	constexpr float NamePromptY = 908.f;

	constexpr unsigned int HeadingSize = 128;
	constexpr unsigned int ScoreLabelSize = 44;
	constexpr unsigned int ScoreValueSize = 132;
	constexpr unsigned int StatLabelSize = 40;
	constexpr unsigned int StatValueSize = 68;
	constexpr unsigned int BadgeSize = 56;
	constexpr unsigned int NameSize = 72;
	constexpr unsigned int PromptSize = 30;
	constexpr unsigned int ButtonSize = 44;

	constexpr float ButtonSpacing = 290.f;

	// Matches GameplayState's dimmed backdrop at the end of the death beat, so
	// the screen arrives with no visible cut.
	constexpr std::uint8_t SceneDim = 140;

	constexpr float AppearSpeed = 1.f / 0.26f;
	constexpr float HeadingDropSpeed = 1.f / 0.42f;
	constexpr float PressDuration = 0.18f;
	constexpr float PressPunch = 0.12f;
	constexpr float PressFlash = 0.55f;
	constexpr float SelectedScale = 1.05f;
	constexpr float UnselectedAlpha = 0.5f;
	constexpr float ButtonGlowIntensity = 0.5f;

	constexpr float PlayAgainDelay = 0.16f;
	constexpr float MainMenuDelay = 0.24f;

	constexpr float Pi = 3.14159265f;

	const sf::Color HeadingFill{ 250, 236, 233 };
	const sf::Color HeadingOutline{ 120, 20, 20 };
	const sf::Color LabelColour{ 168, 150, 158 };
	const sf::Color ScoreColour{ 252, 244, 240 };
	const sf::Color StatValueColour{ 224, 228, 236 };
	const sf::Color BadgeColour{ 255, 208, 120 };
	const sf::Color NameColour{ 252, 244, 240 };
	const sf::Color PromptColour{ 150, 148, 160 };
	const sf::Color PlayHue{ 90, 205, 130 };
	const sf::Color MenuHue{ 228, 232, 240 };

	[[nodiscard]] sf::FloatRect PanelBoundsFor(bool record)
	{
		return { PanelPos, { PanelW, record ? PanelHRecord : PanelHPlain } };
	}

	[[nodiscard]] std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 1.f) * 255.f);
	}

	[[nodiscard]] sf::Color Faded(sf::Color colour, float alpha)
	{
		return sf::Color(colour.r, colour.g, colour.b, ToAlpha(alpha));
	}

	void PlaceCentred(sf::Text& text, sf::Vector2f centre)
	{
		const sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(centre);
	}

	[[nodiscard]] std::string FormatTime(float seconds)
	{
		const int total = std::max(0, static_cast<int>(seconds));
		const int minutes = total / 60;
		const int rest = total % 60;
		return std::to_string(minutes) + ":" + (rest < 10 ? "0" : "") + std::to_string(rest);
	}

	using UI::Easing::EaseOutBack;
	using UI::Easing::SmoothStep;
}

GameOverState::GameOverState(Context& context, int finalScore, int finalLines, int finalLevel, float finalSeconds)
	: State(context.stateMachine)
	, context(context)
	, finalScore(finalScore)
	, finalLines(finalLines)
	, finalLevel(finalLevel)
	, finalSeconds(finalSeconds)
	, isRecord(context.highScores.IsHighScore(finalScore))
	, buttonY(PanelPos.y + (isRecord ? PanelHRecord : PanelHPlain) + 60.f)
	, backdrop(context.textures.Get(Assets::TextureID::GameplayBackground))
	, panel(context.textures.Get(Assets::TextureID::UiFrameRed), PanelBoundsFor(isRecord),
		UI::MenuFrameSourceBorder, PanelTargetBorder)
	, heading(context.fonts.Get(Assets::FontID::Main), "", HeadingSize)
	, recordBadge(context.fonts.Get(Assets::FontID::Main), "", BadgeSize)
	, nameField(context.fonts.Get(Assets::FontID::Main), "", NameSize)
	, namePrompt(context.fonts.Get(Assets::FontID::Main), "", PromptSize)
	, playAgainLabel(context.fonts.Get(Assets::FontID::Menu), ButtonSize)
	, mainMenuLabel(context.fonts.Get(Assets::FontID::Menu), ButtonSize)
	, buttonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	backdrop.setColor(sf::Color(150, 150, 150));

	if (isRecord)
	{
		recordRank = 1;
		for (const HighScoreEntry& entry : context.highScores.GetRecords())
		{
			if (finalScore < entry.score)
			{
				recordRank++;
			}
		}
		recordRank = std::min<int>(recordRank, static_cast<int>(HighScoreManager::MAX_RECORDS));
	}

	playAgainLabel.SetText(context.localization.GetText(TextKey::GameOver::PlayAgain));
	mainMenuLabel.SetText(context.localization.GetText(TextKey::GameOver::MainMenu));

	BuildContent();

	sf::Music& music = context.music.Get(Assets::MusicID::GameOver);
	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void GameOverState::BuildContent()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	heading.setString(text.GetText(TextKey::GameOver::Title));
	heading.setOutlineThickness(4.f);
	heading.setLetterSpacing(1.1f);

	lines.clear();

	const auto add = [&](const sf::String& string, unsigned int size, sf::Vector2f centre, sf::Color colour)
	{
		sf::Text line(font, string, size);
		line.setLetterSpacing(1.1f);
		PlaceCentred(line, centre);
		lines.push_back({ std::move(line), colour });
	};

	add(text.GetText(TextKey::GameOver::Score), ScoreLabelSize, { CentreX, ScoreLabelY }, LabelColour);
	add(std::to_string(finalScore), ScoreValueSize, { CentreX, ScoreValueY }, ScoreColour);

	const float leftX = CentreX - StatSpread;
	add(text.GetText(TextKey::GameOver::Lines), StatLabelSize, { leftX, StatRowY }, LabelColour);
	add(std::to_string(finalLines), StatValueSize, { leftX, StatRowY + StatValueDrop }, StatValueColour);
	add(text.GetText(TextKey::GameOver::Level), StatLabelSize, { CentreX, StatRowY }, LabelColour);
	add(std::to_string(finalLevel), StatValueSize, { CentreX, StatRowY + StatValueDrop }, StatValueColour);
	add(text.GetText(TextKey::GameOver::Time), StatLabelSize, { CentreX + StatSpread, StatRowY }, LabelColour);
	add(FormatTime(finalSeconds), StatValueSize, { CentreX + StatSpread, StatRowY + StatValueDrop }, StatValueColour);

	if (isRecord)
	{
		recordBadge.setString(text.GetText(TextKey::GameOver::NewRecord) + sf::String("   #" + std::to_string(recordRank)));
		recordBadge.setLetterSpacing(1.2f);
		recordBadge.setOutlineThickness(3.f);
		recordBadge.setOutlineColor(sf::Color(70, 44, 0));
		PlaceCentred(recordBadge, { CentreX, BadgeY });

		namePrompt.setString(text.GetText(TextKey::GameOver::EnterName));
		PlaceCentred(namePrompt, { CentreX, NamePromptY });
	}
}

bool GameOverState::NameEntered() const
{
	for (const char32_t character : playerName)
	{
		if (character != U' ')
		{
			return true;
		}
	}
	return false;
}

sf::String GameOverState::TrimmedName() const
{
	std::size_t start = 0;
	std::size_t end = playerName.getSize();
	while (start < end && playerName[start] == U' ')
	{
		start++;
	}
	while (end > start && playerName[end - 1] == U' ')
	{
		end--;
	}
	return playerName.substring(start, end - start);
}

void GameOverState::SaveRecord()
{
	if (!isRecord || !NameEntered())
	{
		return;
	}

	context.highScores.AddRecord({ TrimmedName(), finalScore, finalLines, finalLevel });
	context.highScores.Save();
}

void GameOverState::HandleTextInput(char32_t character)
{
	if (character == U'\b')
	{
		if (!playerName.isEmpty())
		{
			playerName.erase(playerName.getSize() - 1, 1);
		}
	}
	else if (character >= 32 && character != 127)
	{
		if (playerName.getSize() >= MaxNameLength || (character == U' ' && playerName.isEmpty()))
		{
			return;
		}
		playerName += character;
	}
}

void GameOverState::Activate()
{
	if (leaving != Leaving::No)
	{
		return;
	}

	if (isRecord && !NameEntered())
	{
		nameShake = 1.f;
		context.audioPlayer.Play(Assets::SoundID::PieceHitWall, 0.9f);
		return;
	}

	SaveRecord();

	pressTime = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	leaving = focus == Focus::PlayAgain ? Leaving::PlayAgain : Leaving::MainMenu;
	leaveTimer = 0.f;
}

void GameOverState::HandleEvent(const sf::Event& event)
{
	if (leaving != Leaving::No)
	{
		return;
	}

	if (isRecord)
	{
		if (const auto* entered = event.getIf<sf::Event::TextEntered>())
		{
			HandleTextInput(entered->unicode);
			return;
		}
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Left:
	case MenuInput::Action::Right:
		focus = focus == Focus::PlayAgain ? Focus::MainMenu : Focus::PlayAgain;
		context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		return;
	case MenuInput::Action::Confirm:
		Activate();
		return;
	case MenuInput::Action::Back:
		focus = Focus::MainMenu;
		Activate();
		return;
	default:
		break;
	}

	const auto hit = [&](sf::Vector2f point, UI::MenuLabel& label, sf::Vector2f centre) -> bool
	{
		return label.Bounds(centre, 1.f).contains(point);
	};

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = context.window.mapPixelToCoords(moved->position);
		if (hit(point, playAgainLabel, { CentreX - ButtonSpacing, buttonY }))
		{
			focus = Focus::PlayAgain;
		}
		else if (hit(point, mainMenuLabel, { CentreX + ButtonSpacing, buttonY }))
		{
			focus = Focus::MainMenu;
		}
	}
	else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (pressed->button != sf::Mouse::Button::Left)
		{
			return;
		}
		const sf::Vector2f point = context.window.mapPixelToCoords(pressed->position);
		if (hit(point, playAgainLabel, { CentreX - ButtonSpacing, buttonY }))
		{
			focus = Focus::PlayAgain;
			Activate();
		}
		else if (hit(point, mainMenuLabel, { CentreX + ButtonSpacing, buttonY }))
		{
			focus = Focus::MainMenu;
			Activate();
		}
	}
}

void GameOverState::Update(float deltaTime)
{
	appear = std::min(1.f, appear + deltaTime * AppearSpeed);
	headingDrop = std::min(1.f, headingDrop + deltaTime * HeadingDropSpeed);
	pressTime += deltaTime;
	cursorTime += deltaTime;
	nameShake = std::max(0.f, nameShake - deltaTime * 4.f);

	const bool interactive = leaving == Leaving::No;
	playAgainLabel.SetWaveEnabled(interactive && focus == Focus::PlayAgain);
	mainMenuLabel.SetWaveEnabled(interactive && focus == Focus::MainMenu);
	playAgainLabel.Update(deltaTime);
	mainMenuLabel.Update(deltaTime);
	buttonGlow.Update(deltaTime);

	if (leaving == Leaving::No)
	{
		return;
	}

	leaveTimer += deltaTime;

	if (leaving == Leaving::PlayAgain && leaveTimer >= PlayAgainDelay)
	{
		RequestChange(std::make_unique<GameplayState>(context));
	}
	else if (leaving == Leaving::MainMenu && leaveTimer >= MainMenuDelay)
	{
		RequestClear();
		RequestPush(std::make_unique<MenuShell>(context));
	}
}

void GameOverState::DrawButton(sf::RenderTarget& target, UI::MenuLabel& label, sf::Vector2f centre,
	sf::Color hue, bool selected, float alpha)
{
	const float press = (selected && pressTime < PressDuration)
		? std::sin(std::clamp(1.f - pressTime / PressDuration, 0.f, 1.f) * Pi)
		: 0.f;
	const float scale = (selected ? SelectedScale : 1.f) + PressPunch * press;
	const float drawAlpha = alpha * (selected ? 1.f : UnselectedAlpha);

	if (selected)
	{
		label.DrawGlow(target, buttonGlow, centre, scale,
			sf::Color(hue.r, hue.g, hue.b, static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f * ButtonGlowIntensity)));
	}

	label.Draw(target, centre, scale, hue, drawAlpha, PressFlash * press);
}

void GameOverState::Render(sf::RenderTarget& target)
{
	target.draw(backdrop);

	sf::RectangleShape dim(Screen);
	dim.setFillColor(sf::Color(0, 0, 0, SceneDim));
	target.draw(dim);

	const float in = SmoothStep(appear);
	const auto contentAlpha = std::clamp((appear - 0.2f) / 0.8f, 0.f, 1.f);

	panel.SetColor(sf::Color(255, 255, 255, ToAlpha(in)));
	panel.Draw(target);

	if (contentAlpha > 0.f)
	{
		const float rise = (1.f - EaseOutBack(headingDrop)) * 70.f;
		heading.setFillColor(Faded(HeadingFill, contentAlpha));
		heading.setOutlineColor(Faded(HeadingOutline, contentAlpha));
		PlaceCentred(heading, { CentreX, HeadingY - rise });
		target.draw(heading);

		for (Line& line : lines)
		{
			line.text.setFillColor(Faded(line.base, contentAlpha));
			target.draw(line.text);
		}

		if (isRecord)
		{
			recordBadge.setFillColor(Faded(BadgeColour, contentAlpha));
			target.draw(recordBadge);

			const float shake = std::sin(nameShake * 40.f) * nameShake * 10.f;
			const bool showCursor = std::fmod(cursorTime, 1.f) < 0.55f;
			nameField.setString(playerName + (showCursor ? sf::String("_") : sf::String(" ")));
			PlaceCentred(nameField, { CentreX + shake, NameY });
			nameField.setFillColor(Faded(nameShake > 0.f ? sf::Color(240, 120, 120) : NameColour, contentAlpha));
			target.draw(nameField);

			if (!NameEntered())
			{
				namePrompt.setFillColor(Faded(PromptColour, contentAlpha * 0.9f));
				target.draw(namePrompt);
			}
		}
	}

	const bool buttonsLive = !isRecord || NameEntered();
	const float buttonAlpha = contentAlpha * (buttonsLive ? 1.f : 0.4f);
	DrawButton(target, playAgainLabel, { CentreX - ButtonSpacing, buttonY }, PlayHue,
		leaving == Leaving::No && focus == Focus::PlayAgain, buttonAlpha);
	DrawButton(target, mainMenuLabel, { CentreX + ButtonSpacing, buttonY }, MenuHue,
		leaving == Leaving::No && focus == Focus::MainMenu, buttonAlpha);

	if (leaving == Leaving::MainMenu)
	{
		sf::RectangleShape fade(Screen);
		fade.setFillColor(sf::Color(0, 0, 0, ToAlpha(leaveTimer / MainMenuDelay)));
		target.draw(fade);
	}
}
