#include "GameplayAmbience.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../utils/Random.h"

namespace
{
	// A margin past the screen so parallax-shifted motes never leave a bare edge.
	constexpr float Margin = 140.f;
	constexpr float MinX = -Margin;
	constexpr float MaxX = 1920.f + Margin;
	constexpr float MinY = -Margin;
	constexpr float MaxY = 1080.f + Margin;

	constexpr float Pi = 3.14159265f;

	const sf::Color MoteColour{ 188, 206, 236 };

	struct LayerSpec
	{
		int count;
		float parallax;
		float radiusMin, radiusMax;
		float alphaMin, alphaMax;
		float speedMin, speedMax;
	};

	constexpr std::array<LayerSpec, 3> Layers = { {
		{ 26, 1.5f, 2.0f,  3.6f, 0.16f, 0.30f,  3.f,  8.f },   // far
		{ 18, 2.6f, 3.4f,  6.0f, 0.24f, 0.42f,  6.f, 13.f },   // mid
		{ 12, 3.9f, 5.5f, 10.0f, 0.34f, 0.58f, 10.f, 20.f },   // near
	} };

	[[nodiscard]] float Wrap(float value, float lo, float hi)
	{
		const float span = hi - lo;
		while (value < lo) { value += span; }
		while (value > hi) { value -= span; }
		return value;
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

			Mote mote;
			mote.base = { Random::Float(MinX, MaxX), Random::Float(MinY, MaxY) };
			mote.drift = { std::cos(angle) * speed, std::sin(angle) * speed * 0.6f };
			mote.radius = Random::Float(layer.radiusMin, layer.radiusMax);
			mote.swayPhase = Random::Float(0.f, 2.f * Pi);
			mote.swaySpeed = Random::Float(0.2f, 0.6f);
			mote.swayAmp = Random::Float(6.f, 18.f);
			mote.twinklePhase = Random::Float(0.f, 2.f * Pi);
			mote.twinkleSpeed = Random::Float(0.5f, 1.4f);
			mote.baseAlpha = Random::Float(layer.alphaMin, layer.alphaMax);
			mote.parallax = layer.parallax;

			motes.push_back(mote);
		}
	}
}

void GameplayAmbience::Update(float deltaTime, sf::Vector2f sceneOffset)
{
	offset = sceneOffset;

	for (Mote& mote : motes)
	{
		mote.base += mote.drift * deltaTime;
		mote.base.x = Wrap(mote.base.x, MinX, MaxX);
		mote.base.y = Wrap(mote.base.y, MinY, MaxY);

		mote.swayPhase += mote.swaySpeed * deltaTime;
		mote.twinklePhase += mote.twinkleSpeed * deltaTime;
	}
}

void GameplayAmbience::Render(sf::RenderTarget& target) const
{
	sf::RenderStates additive;
	additive.blendMode = sf::BlendAdd;

	sf::CircleShape dot;
	dot.setPointCount(14);

	for (const Mote& mote : motes)
	{
		const float twinkle = 0.72f + 0.28f * std::sin(mote.twinklePhase);
		const float alpha = std::clamp(mote.baseAlpha * twinkle, 0.f, 1.f);

		const sf::Vector2f position{
			mote.base.x + std::sin(mote.swayPhase) * mote.swayAmp + offset.x * mote.parallax,
			mote.base.y + std::cos(mote.swayPhase * 0.8f) * mote.swayAmp * 0.6f + offset.y * mote.parallax };

		dot.setRadius(mote.radius);
		dot.setOrigin({ mote.radius, mote.radius });
		dot.setPosition(position);
		dot.setFillColor(sf::Color(MoteColour.r, MoteColour.g, MoteColour.b,
			static_cast<std::uint8_t>(alpha * 255.f)));
		target.draw(dot, additive);
	}
}
