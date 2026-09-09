#include "ConfirmDialog.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>

#include "../audio/AudioPlayer.h"
#include "../resources/Assets.h"
#include "Easing.h"
#include "TextLayout.h"

namespace
{
	constexpr sf::Vector2f Centre{ 960.f, 540.f };
	constexpr sf::Vector2f BoxSize{ 880.f, 380.f };
	constexpr sf::FloatRect BoxBounds{
		{ Centre.x - BoxSize.x * 0.5f, Centre.y - BoxSize.y * 0.5f }, BoxSize };
	constexpr sf::Vector2f FrameTargetBorder{ 40.f, 40.f };

	constexpr unsigned int MessageSize = 40;
	constexpr unsigned int ButtonSize = 40;

	constexpr float MessageY = Centre.y - 62.f;
	constexpr float ButtonY = Centre.y + 100.f;
	constexpr float ButtonSpacing = 196.f;

	// The box slides in from above: fully off the top at appear 0, home at 1.
	constexpr float EntryDrop = 780.f;
	constexpr float AppearSpeed = 1.f / 0.16f;

	constexpr float ResolveDuration = 0.20f;
	constexpr float PressPunch = 0.13f;
	constexpr float PressFlash = 0.6f;
	constexpr float SelectedScale = 1.06f;
	constexpr float UnselectedAlpha = 0.5f;
	constexpr float DimAlpha = 175.f;
	constexpr float Pi = 3.14159265f;

	const sf::Color MessageColour{ 244, 234, 210 };
	const sf::Color YesHue{ 70, 200, 110 };   // matches the Options "Apply" green
	const sf::Color NoHue{ 240, 70, 78 };     // the menu red

	using UI::Easing::EaseOutCubic;
	using UI::Easing::Lerp;
	using UI::Easing::SmoothStep;

	[[nodiscard]] std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 1.f) * 255.f);
	}
}

namespace UI
{
	ConfirmDialog::ConfirmDialog(const sf::Font& messageFont, const sf::Font& buttonFont,
		const sf::Texture& frameTexture, sf::Shader& neonDilate, sf::Shader& neonBlur, AudioPlayer& audio)
		: messageText(messageFont, "", MessageSize)
		, frame(frameTexture, BoxBounds, UI::MenuFrameSourceBorder, FrameTargetBorder)
		, yesLabel(buttonFont, ButtonSize)
		, noLabel(buttonFont, ButtonSize)
		, glow(neonDilate, neonBlur)
		, audio(audio)
	{
		messageText.setFillColor(MessageColour);
	}

	void ConfirmDialog::Show(const sf::String& message, const sf::String& yesText, const sf::String& noText)
	{
		messageText.setString(message);
		UI::TextLayout::CentreOrigin(messageText);

		yesLabel.SetText(yesText);
		noLabel.SetText(noText);

		phase = Phase::Open;
		yesSelected = false;   // default to "No" -- the safe answer
		result.reset();
		appear = 0.f;
		resolveTime = 0.f;
	}

	std::optional<bool> ConfirmDialog::TakeResult()
	{
		const std::optional<bool> value = result;
		result.reset();
		return value;
	}

	void ConfirmDialog::Choose(bool answer)
	{
		chosenAnswer = answer;
		yesSelected = answer;   // show the chosen button as selected while it flashes
		phase = Phase::Resolving;
		resolveTime = 0.f;
	}

	void ConfirmDialog::Navigate(MenuInput::Action action)
	{
		if (phase != Phase::Open)
		{
			return;
		}

		switch (action)
		{
		case MenuInput::Action::Left:
		case MenuInput::Action::Right:
			yesSelected = !yesSelected;
			audio.Restart(Assets::SoundID::MenuItemSelected);
			break;
		case MenuInput::Action::Confirm:
			Choose(yesSelected);
			break;
		case MenuInput::Action::Back:
			Choose(false);
			break;
		default:
			break;
		}
	}

	void ConfirmDialog::Update(float deltaTime)
	{
		if (phase == Phase::Closed)
		{
			return;
		}

		appear = std::min(1.f, appear + deltaTime * AppearSpeed);

		yesLabel.SetWaveEnabled(yesSelected);
		noLabel.SetWaveEnabled(!yesSelected);
		yesLabel.Update(deltaTime);
		noLabel.Update(deltaTime);
		glow.Update(deltaTime);

		if (phase == Phase::Resolving)
		{
			resolveTime += deltaTime;
			if (resolveTime >= ResolveDuration)
			{
				result = chosenAnswer;
				phase = Phase::Closed;
			}
		}
	}

	void ConfirmDialog::DrawButton(sf::RenderTarget& target, MenuLabel& label, sf::Vector2f centre,
		sf::Color hue, bool selected, float contentAlpha)
	{
		const float press = (phase == Phase::Resolving && selected)
			? std::sin(std::clamp(1.f - resolveTime / ResolveDuration, 0.f, 1.f) * Pi)
			: 0.f;

		const float scale = (selected ? SelectedScale : 1.f) + PressPunch * press;
		const float alpha = contentAlpha * (selected ? 1.f : UnselectedAlpha);

		if (selected)
		{
			const sf::Color glowTint(hue.r, hue.g, hue.b, ToAlpha(contentAlpha));
			label.DrawGlow(target, glow, centre, scale, glowTint);
		}

		label.Draw(target, centre, scale, hue, alpha, PressFlash * press);
	}

	void ConfirmDialog::Render(sf::RenderTarget& target)
	{
		if (phase == Phase::Closed)
		{
			return;
		}

		sf::RectangleShape dim({ 1920.f, 1080.f });
		dim.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(SmoothStep(appear) * DimAlpha)));
		target.draw(dim);

		const float slideY = Lerp(-EntryDrop, 0.f, EaseOutCubic(appear));

		// A dark fill behind the frame so the message stays legible whatever the
		// frame texture's centre does.
		sf::RectangleShape fill(BoxSize);
		fill.setOrigin(BoxSize * 0.5f);
		fill.setPosition({ Centre.x, Centre.y + slideY });
		fill.setFillColor(sf::Color(12, 11, 16, 225));
		target.draw(fill);

		sf::Transform slide;
		slide.translate({ 0.f, slideY });
		frame.SetColor(sf::Color::White);
		frame.Draw(target, sf::RenderStates(slide));

		messageText.setPosition({ Centre.x, MessageY + slideY });
		target.draw(messageText);

		DrawButton(target, noLabel, { Centre.x - ButtonSpacing, ButtonY + slideY }, NoHue, !yesSelected, 1.f);
		DrawButton(target, yesLabel, { Centre.x + ButtonSpacing, ButtonY + slideY }, YesHue, yesSelected, 1.f);
	}
}
