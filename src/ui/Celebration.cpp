#include "Celebration.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../utils/Random.h"

namespace
{
	constexpr sf::Vector2f Screen{ 1920.f, 1080.f };
	constexpr float Pi = 3.14159265f;

	constexpr float MinLaunchGap = 0.45f;
	constexpr float MaxLaunchGap = 1.25f;

	constexpr float RocketGravity = 110.f;
	constexpr int MinBurst = 34;
	constexpr int MaxBurst = 52;

	// Corner jets: total sparks per second across all four corners.
	constexpr float CornerEmitRate = 90.f;

	const std::array<sf::Color, 6> WarmPalette = { {
		sf::Color(255, 214, 128),
		sf::Color(255, 176, 92),
		sf::Color(255, 240, 200),
		sf::Color(255, 148, 96),
		sf::Color(255, 226, 150),
		sf::Color(170, 214, 255),   // an occasional cool one
	} };

	[[nodiscard]] sf::Color PickWarm()
	{
		return WarmPalette[static_cast<std::size_t>(Random::Int(0, static_cast<int>(WarmPalette.size()) - 1))];
	}

	[[nodiscard]] sf::Color Jitter(sf::Color colour, int amount)
	{
		const auto nudge = [&](std::uint8_t channel)
		{
			return static_cast<std::uint8_t>(std::clamp(static_cast<int>(channel) + Random::Int(-amount, amount), 0, 255));
		};
		return sf::Color(nudge(colour.r), nudge(colour.g), nudge(colour.b));
	}
}

namespace UI
{
	Celebration::Celebration()
	{
		launchTimer = Random::Float(0.1f, 0.4f);
		rockets.reserve(8);
		burstSparks.reserve(512);
		cornerSparks.reserve(256);
	}

	void Celebration::SetCorners(const std::array<sf::Vector2f, 4>& corners)
	{
		cornerPoints = corners;

		sf::Vector2f centre{ 0.f, 0.f };
		for (const sf::Vector2f& corner : corners)
		{
			centre += corner;
		}
		centre /= 4.f;

		for (std::size_t i = 0; i < corners.size(); ++i)
		{
			sf::Vector2f away = corners[i] - centre;
			const float length = std::sqrt(away.x * away.x + away.y * away.y);
			cornerDirections[i] = length > 0.f ? away / length : sf::Vector2f{ 0.f, -1.f };
		}

		cornersSet = true;
	}

	void Celebration::Explode(const Rocket& rocket)
	{
		const int count = Random::Int(MinBurst, MaxBurst);
		const float baseSpeed = Random::Float(150.f, 260.f);

		for (int i = 0; i < count; ++i)
		{
			const float angle = Random::Float(0.f, 2.f * Pi);
			const float speed = baseSpeed * Random::Float(0.35f, 1.15f);

			Spark spark;
			spark.position = rocket.position;
			spark.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
			spark.size = Random::Float(1.8f, 3.6f);
			spark.maxLife = Random::Float(0.7f, 1.5f);
			spark.life = spark.maxLife;
			spark.gravity = Random::Float(160.f, 260.f);
			spark.drag = Random::Float(0.28f, 0.5f);
			spark.colour = Jitter(rocket.colour, 22);
			burstSparks.push_back(spark);
		}
	}

	void Celebration::Update(float deltaTime)
	{
		// Launch new shells.
		launchTimer -= deltaTime;
		if (launchTimer <= 0.f)
		{
			launchTimer = Random::Float(MinLaunchGap, MaxLaunchGap);

			Rocket rocket;
			rocket.position = { Random::Float(320.f, Screen.x - 320.f), Screen.y + 20.f };
			rocket.velocity = { Random::Float(-45.f, 45.f), Random::Float(-780.f, -600.f) };
			rocket.fuse = Random::Float(0.75f, 1.15f);
			rocket.colour = PickWarm();
			rockets.push_back(rocket);
		}

		for (Rocket& rocket : rockets)
		{
			rocket.velocity.y += RocketGravity * deltaTime;
			rocket.position += rocket.velocity * deltaTime;
			rocket.fuse -= deltaTime;
		}

		for (auto it = rockets.begin(); it != rockets.end();)
		{
			if (it->fuse <= 0.f || it->velocity.y >= -30.f)
			{
				Explode(*it);
				it = rockets.erase(it);
			}
			else
			{
				++it;
			}
		}

		const auto stepSpark = [deltaTime](Spark& spark)
		{
			const float keep = std::pow(spark.drag, deltaTime);
			spark.velocity *= keep;
			spark.velocity.y += spark.gravity * deltaTime;
			spark.position += spark.velocity * deltaTime;
			spark.life -= deltaTime;
		};

		for (Spark& spark : burstSparks)
		{
			stepSpark(spark);
		}
		std::erase_if(burstSparks, [](const Spark& spark) { return spark.life <= 0.f; });

		// Corner jets.
		if (cornersSet)
		{
			cornerEmitCarry += CornerEmitRate * deltaTime;
			const int toEmit = static_cast<int>(cornerEmitCarry);
			cornerEmitCarry -= static_cast<float>(toEmit);

			for (int i = 0; i < toEmit; ++i)
			{
				const std::size_t corner = static_cast<std::size_t>(Random::Int(0, 3));
				const sf::Vector2f dir = cornerDirections[corner];
				const float spread = Random::Float(-0.6f, 0.6f);
				const float cos = std::cos(spread);
				const float sin = std::sin(spread);
				const sf::Vector2f aimed{ dir.x * cos - dir.y * sin, dir.x * sin + dir.y * cos };
				const float speed = Random::Float(90.f, 260.f);

				Spark spark;
				spark.position = cornerPoints[corner];
				spark.velocity = aimed * speed;
				spark.size = Random::Float(1.4f, 3.2f);
				spark.maxLife = Random::Float(0.35f, 0.8f);
				spark.life = spark.maxLife;
				spark.gravity = Random::Float(90.f, 190.f);
				spark.drag = Random::Float(0.15f, 0.35f);
				spark.colour = Random::Float(0.f, 1.f) < 0.4f
					? sf::Color(255, 248, 224)
					: sf::Color(255, 206, 110);
				cornerSparks.push_back(spark);
			}
		}

		for (Spark& spark : cornerSparks)
		{
			stepSpark(spark);
		}
		std::erase_if(cornerSparks, [](const Spark& spark) { return spark.life <= 0.f; });
	}

	void Celebration::RenderFireworks(sf::RenderTarget& target) const
	{
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape dot;
		dot.setPointCount(10);

		for (const Rocket& rocket : rockets)
		{
			dot.setRadius(2.6f);
			dot.setOrigin({ 2.6f, 2.6f });
			dot.setPosition(rocket.position);
			dot.setFillColor(sf::Color(255, 240, 210));
			target.draw(dot, additive);
		}

		for (const Spark& spark : burstSparks)
		{
			const float fade = std::clamp(spark.life / spark.maxLife, 0.f, 1.f);
			const float alpha = fade * fade;   // linger bright, then drop fast

			dot.setRadius(spark.size);
			dot.setOrigin({ spark.size, spark.size });
			dot.setPosition(spark.position);
			dot.setFillColor(sf::Color(spark.colour.r, spark.colour.g, spark.colour.b,
				static_cast<std::uint8_t>(alpha * 255.f)));
			target.draw(dot, additive);
		}
	}

	void Celebration::RenderCornerSparks(sf::RenderTarget& target) const
	{
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape dot;
		dot.setPointCount(8);

		for (const Spark& spark : cornerSparks)
		{
			const float fade = std::clamp(spark.life / spark.maxLife, 0.f, 1.f);

			dot.setRadius(spark.size);
			dot.setOrigin({ spark.size, spark.size });
			dot.setPosition(spark.position);
			dot.setFillColor(sf::Color(spark.colour.r, spark.colour.g, spark.colour.b,
				static_cast<std::uint8_t>(fade * 255.f)));
			target.draw(dot, additive);
		}
	}
}
