#include "GameplayAmbience.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../ui/TetrominoPalette.h"
#include "../utils/Random.h"

namespace
{
	// A margin past the screen so parallax-shifted flecks never leave a bare edge.
	constexpr float Margin = 160.f;
	constexpr float MinX = -Margin;
	constexpr float MaxX = 1920.f + Margin;
	constexpr float MinY = -Margin;
	constexpr float MaxY = 1080.f + Margin;

	constexpr float Pi = 3.14159265f;

	// Everything snaps to this grid so the flecks hop like low-res sprites
	// instead of gliding.
	constexpr float PixelGrid = 4.f;

	struct LayerSpec
	{
		int count;
		float parallax;
		float sizeCells;       // square side in grid cells
		float alphaMin, alphaMax;
		float speedMin, speedMax;
		float dim;             // how far the tint is pulled toward black
	};

	constexpr std::array<LayerSpec, 3> Layers = { {
		{ 30, 1.4f, 1.f, 0.10f, 0.20f,  3.f,  7.f, 0.35f },   // far: single dim pixels
		{ 20, 2.4f, 2.f, 0.16f, 0.30f,  5.f, 11.f, 0.55f },   // mid
		{ 12, 3.6f, 3.f, 0.24f, 0.42f,  9.f, 17.f, 0.75f },   // near
	} };

	[[nodiscard]] float Wrap(float value, float lo, float hi)
	{
		const float span = hi - lo;
		while (value < lo) { value += span; }
		while (value > hi) { value -= span; }
		return value;
	}

	[[nodiscard]] float Snap(float value)
	{
		return std::round(value / PixelGrid) * PixelGrid;
	}

	[[nodiscard]] sf::Color Dimmed(sf::Color colour, float amount)
	{
		const float keep = 1.f - amount;
		return sf::Color(
			static_cast<std::uint8_t>(static_cast<float>(colour.r) * keep),
			static_cast<std::uint8_t>(static_cast<float>(colour.g) * keep),
			static_cast<std::uint8_t>(static_cast<float>(colour.b) * keep));
	}
}

GameplayAmbience::GameplayAmbience()
{
	for (const LayerSpec& layer : Layers)
	{
		for (int i = 0; i < layer.count; ++i)
		{
			const float angle = Random::Float(0.f, 2.f * Pi);
			const float speed = Random::Float(layer.speedMin, layer.speedMax);
			const sf::Color base = UI::TetrominoColours[static_cast<std::size_t>(Random::Int(0, 6))];

			Fleck fleck;
			fleck.base = { Random::Float(MinX, MaxX), Random::Float(MinY, MaxY) };
			fleck.drift = { std::cos(angle) * speed, std::sin(angle) * speed * 0.6f };
			fleck.size = layer.sizeCells * PixelGrid;
			fleck.swayPhase = Random::Float(0.f, 2.f * Pi);
			fleck.swaySpeed = Random::Float(0.15f, 0.5f);
			fleck.swayAmp = Random::Float(5.f, 16.f);
			fleck.twinklePhase = Random::Float(0.f, 2.f * Pi);
			fleck.twinkleSpeed = Random::Float(0.4f, 1.2f);
			fleck.baseAlpha = Random::Float(layer.alphaMin, layer.alphaMax);
			fleck.parallax = layer.parallax;
			fleck.tint = Dimmed(base, layer.dim);

			flecks.push_back(fleck);
		}
	}
}

void GameplayAmbience::Update(float deltaTime, sf::Vector2f sceneOffset)
{
	offset = sceneOffset;

	for (Fleck& fleck : flecks)
	{
		fleck.base += fleck.drift * deltaTime;
		fleck.base.x = Wrap(fleck.base.x, MinX, MaxX);
		fleck.base.y = Wrap(fleck.base.y, MinY, MaxY);

		fleck.swayPhase += fleck.swaySpeed * deltaTime;
		fleck.twinklePhase += fleck.twinkleSpeed * deltaTime;
	}
}

void GameplayAmbience::Render(sf::RenderTarget& target) const
{
	sf::RenderStates additive;
	additive.blendMode = sf::BlendAdd;

	sf::RectangleShape pixel;

	for (const Fleck& fleck : flecks)
	{
		const float twinkle = 0.7f + 0.3f * std::sin(fleck.twinklePhase);
		const float alpha = std::clamp(fleck.baseAlpha * twinkle, 0.f, 1.f);

		const sf::Vector2f position{
			Snap(fleck.base.x + std::sin(fleck.swayPhase) * fleck.swayAmp + offset.x * fleck.parallax),
			Snap(fleck.base.y + std::cos(fleck.swayPhase * 0.8f) * fleck.swayAmp * 0.6f + offset.y * fleck.parallax) };

		pixel.setSize({ fleck.size, fleck.size });
		pixel.setPosition(position);
		pixel.setFillColor(sf::Color(fleck.tint.r, fleck.tint.g, fleck.tint.b,
			static_cast<std::uint8_t>(alpha * 255.f)));
		target.draw(pixel, additive);
	}
}
