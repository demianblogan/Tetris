#include "PlayTransition.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../core/Context.h"
#include "../gameplay/Board.h"
#include "../rendering/BoardRenderer.h"
#include "../resources/Assets.h"
#include "../ui/Easing.h"
#include "GameplayState.h"

namespace
{
	constexpr float MorphDuration = 0.55f;

	// The playfield frame the "Play" box unfolds into.
	constexpr sf::Vector2f FieldSize{
		Board::WIDTH * BoardRenderer::BlockSize, Board::HEIGHT * BoardRenderer::BlockSize };
	constexpr sf::Vector2f FieldCentre{
		BoardRenderer::BoardPosition.x + FieldSize.x * 0.5f,
		BoardRenderer::BoardPosition.y + FieldSize.y * 0.5f };

	// Fixed box for the outline's glow, so NeonGlow sizes its buffers once
	// instead of every frame as the rectangle grows.
	constexpr sf::FloatRect MorphGlowArea{
		{ BoardRenderer::BoardPosition.x - 40.f, BoardRenderer::BoardPosition.y - 40.f },
		{ FieldSize.x + 80.f, FieldSize.y + 80.f } };

	constexpr float OutlineThickness = 4.f;

	using UI::Easing::EaseInCubic;
	using UI::Easing::EaseOutCubic;
	using UI::Easing::Lerp;
	using UI::Easing::SmoothStep;

	[[nodiscard]] std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(std::clamp(value, 0.f, 1.f) * 255.f);
	}
}

PlayTransition::PlayTransition(Context& context, std::unique_ptr<sf::RenderTexture> menuSnapshot,
	sf::Vector2f fromCentre, sf::Vector2f fromSize, sf::Color accent)
	: State(context.stateMachine)
	, context(context)
	, menuSnapshot(std::move(menuSnapshot))
	, gameplay(std::make_unique<GameplayState>(context, true))
	, morphGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, fromCentre(fromCentre)
	, fromSize(fromSize)
	, accent(accent)
{
}

PlayTransition::~PlayTransition() = default;

void PlayTransition::HandleEvent(const sf::Event& /*event*/)
{
	// The transition owns the screen; nothing reacts to input.
}

void PlayTransition::Update(float deltaTime)
{
	timer += deltaTime;
	morphGlow.Update(deltaTime);

	if (gameplay)
	{
		gameplay->Update(deltaTime);
	}

	if (!handedOver && timer >= MorphDuration && gameplay)
	{
		handedOver = true;
		RequestChange(std::move(gameplay));
	}
}

void PlayTransition::Render(sf::RenderTarget& target)
{
	if (gameplay)
	{
		gameplay->Render(target);
	}
	else
	{
		target.clear(sf::Color::Black);
	}

	const float t = std::clamp(timer / MorphDuration, 0.f, 1.f);

	// The emptied menu ambient, fading out over the first ~60%.
	if (menuSnapshot)
	{
		const float snapshotAlpha = 1.f - EaseOutCubic(std::clamp(t / 0.6f, 0.f, 1.f));
		if (snapshotAlpha > 0.f)
		{
			sf::Sprite snapshot(menuSnapshot->getTexture());
			snapshot.setColor(sf::Color(255, 255, 255, ToAlpha(snapshotAlpha)));
			target.draw(snapshot);
		}
	}

	// The neon outline unfolding from the "Play" box into the playfield frame.
	const float e = SmoothStep(t);
	const sf::Vector2f size = Lerp(fromSize, FieldSize, e);
	const sf::Vector2f centre = Lerp(fromCentre, FieldCentre, e);

	const float fadeIn = std::clamp(t / 0.12f, 0.f, 1.f);
	const float fadeOut = 1.f - EaseInCubic(std::clamp((t - 0.72f) / 0.28f, 0.f, 1.f));
	const float outlineAlpha = std::min(fadeIn, fadeOut);
	if (outlineAlpha <= 0.f)
	{
		return;
	}

	sf::RectangleShape outline(size);
	outline.setOrigin(size * 0.5f);
	outline.setPosition(centre);
	outline.setFillColor(sf::Color::Transparent);
	outline.setOutlineThickness(OutlineThickness);
	outline.setOutlineColor(sf::Color(accent.r, accent.g, accent.b, ToAlpha(outlineAlpha)));

	morphGlow.Draw(target, MorphGlowArea,
		[&outline](sf::RenderTarget& buffer, const sf::RenderStates& states)
		{
			buffer.draw(outline, states);
		},
		sf::Color(accent.r, accent.g, accent.b, ToAlpha(outlineAlpha)), true);

	target.draw(outline);
}
