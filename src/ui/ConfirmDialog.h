#pragma once

#include <optional>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include "../input/MenuInput.h"
#include "../rendering/NeonGlow.h"
#include "MenuLabel.h"
#include "NineSliceFrame.h"

class AudioPlayer;

namespace sf
{
	class Font;
	class Shader;
	class Texture;
	class RenderTarget;
}

namespace UI
{
	// A modal yes / no dialog: dims the screen and shows a gold-framed box with a
	// message and two menu-style buttons -- Yes green, No red -- that carry the
	// same idle wave, selection glow and press flash as the rest of the menus.
	// Drive it with Navigate(); poll TakeResult() for the answer (nullopt while
	// it is still open).
	class ConfirmDialog
	{
	public:
		ConfirmDialog(const sf::Font& messageFont, const sf::Font& buttonFont,
			const sf::Texture& frameTexture, sf::Shader& neonDilate, sf::Shader& neonBlur,
			AudioPlayer& audio);

		void Show(const sf::String& message, const sf::String& yesLabel, const sf::String& noLabel);

		[[nodiscard]] bool IsOpen() const { return phase != Phase::Closed; }
		[[nodiscard]] std::optional<bool> TakeResult();

		void Navigate(MenuInput::Action action);

		void Update(float deltaTime);
		void Render(sf::RenderTarget& target);

	private:
		enum class Phase { Closed, Open, Resolving };

		void Choose(bool answer);
		void DrawButton(sf::RenderTarget& target, MenuLabel& label, sf::Vector2f centre,
			sf::Color hue, bool selected, float contentAlpha);

		sf::Text messageText;
		NineSliceFrame frame;
		MenuLabel yesLabel;
		MenuLabel noLabel;
		NeonGlow glow;
		AudioPlayer& audio;

		Phase phase = Phase::Closed;
		bool yesSelected = false;
		bool chosenAnswer = false;
		std::optional<bool> result;

		float appear = 0.f;        // 0..1 fade-in
		float resolveTime = 0.f;   // seconds since a choice was made
	};
}
