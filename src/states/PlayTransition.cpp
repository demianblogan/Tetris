#include "PlayTransition.h"

#include <algorithm>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../core/Context.h"
#include "../display/DisplayManager.h"
#include "../resources/Assets.h"
#include "../ui/Easing.h"
#include "GameplayState.h"

namespace
{
	// The "solidified" look of the frozen menu -- kept in step with PauseState so
	// the two mosaic effects read as the same material.
	constexpr float MosaicCellPx = 22.f;
	constexpr float MosaicBlurPx = 16.f;
	constexpr float MosaicDarken = 0.5f;
	constexpr float MosaicFrontSoft = 46.f;

	// Solidify top-down, then slide the solid sheet down and off.
	constexpr float SolidifyDuration = 0.26f;
	constexpr float LiftDuration = 0.34f;
	constexpr float TotalDuration = SolidifyDuration + LiftDuration;

	using UI::Easing::EaseInCubic;
}

PlayTransition::PlayTransition(Context& context, std::unique_ptr<sf::RenderTexture> menuSnapshot)
	: State(context.stateMachine)
	, context(context)
	, menuSnapshot(std::move(menuSnapshot))
	, gameplay(std::make_unique<GameplayState>(context, true))
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

	if (gameplay)
	{
		gameplay->Update(deltaTime);
	}

	if (!handedOver && timer >= TotalDuration && gameplay)
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

	if (!menuSnapshot)
	{
		return;
	}

	const float reveal = std::clamp(timer / SolidifyDuration, 0.f, 1.f);
	const float liftT = std::clamp((timer - SolidifyDuration) / LiftDuration, 0.f, 1.f);
	const float offsetY = EaseInCubic(liftT) * Display::DisplayManager::VirtualSize.y;

	sf::Shader& mosaic = context.shaders.Get(Assets::ShaderID::Mosaic);
	mosaic.setUniform("texture", sf::Shader::CurrentTexture);
	mosaic.setUniform("resolution", sf::Glsl::Vec2(Display::DisplayManager::VirtualSize));
	mosaic.setUniform("cellPx", MosaicCellPx);
	mosaic.setUniform("blurPx", MosaicBlurPx);
	mosaic.setUniform("darken", MosaicDarken);
	mosaic.setUniform("reveal", reveal);
	mosaic.setUniform("frontSoft", MosaicFrontSoft);

	sf::Sprite sheet(menuSnapshot->getTexture());
	sheet.setPosition({ 0.f, offsetY });
	target.draw(sheet, &mosaic);
}
