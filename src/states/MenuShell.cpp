#include "MenuShell.h"

#include <memory>
#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/View.hpp>

#include "../core/Context.h"
#include "../core/GameVersion.h"
#include "../display/DisplayManager.h"
#include "../resources/Assets.h"
#include "MainMenuScreen.h"
#include "PlayTransition.h"

namespace
{
	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };
}

MenuShell::MenuShell(Context& context)
	: ScreenHost(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::GameBackground))
	, aurora(context.shaders.Get(Assets::ShaderID::MenuAurora))
	, backdrop(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline))
	, versionText(context.fonts.Get(Assets::FontID::Main), std::string(GameVersion::Text), VersionTextSize)
{
	versionText.setFillColor(sf::Color(150, 160, 170));
	const sf::FloatRect versionBounds = versionText.getLocalBounds();
	versionText.setOrigin(
		{
			versionBounds.position.x + versionBounds.size.x,
			versionBounds.position.y + versionBounds.size.y
		});

	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& menuMusic = context.music.Get(Assets::MusicID::MainMenu);
	menuMusic.setLooping(true);
	if (menuMusic.getStatus() != sf::Music::Status::Playing)
	{
		menuMusic.play();
	}

	SetInitialScreen(std::make_unique<MainMenuScreen>(*this));
}

void MenuShell::Update(float deltaTime)
{
	ScreenHost::Update(deltaTime);

	if (!playPending || !CurrentScreen() || !CurrentScreen()->ExitFinished())
	{
		return;
	}

	playPending = false;

	// Freeze the shell's current frame -- the emptied ring over the live ambient
	// -- so the transition can fade it out over the arriving gameplay.
	std::unique_ptr<sf::RenderTexture> snapshot;
	auto capture = std::make_unique<sf::RenderTexture>();
	if (capture->resize(sf::Vector2u(Display::DisplayManager::VirtualSize)))
	{
		capture->setView(sf::View(sf::FloatRect({ 0.f, 0.f }, Display::DisplayManager::VirtualSize)));
		capture->clear(sf::Color::Black);
		Render(*capture);
		capture->display();
		snapshot = std::move(capture);
	}

	RequestChange(std::make_unique<PlayTransition>(
		context, std::move(snapshot), playFromCentre, playFromSize, playAccent));
}

void MenuShell::OnNavigate(float direction)
{
	backdrop.Push(direction);
}

void MenuShell::BeginPlay(sf::Vector2f fromCentre, sf::Vector2f fromSize, sf::Color accent)
{
	if (playPending || !CurrentScreen())
	{
		return;
	}

	playPending = true;
	playFromCentre = fromCentre;
	playFromSize = fromSize;
	playAccent = accent;

	CurrentScreen()->PlayActivatePulse();
	CurrentScreen()->StartExit();
}

void MenuShell::UpdateBackground(float deltaTime)
{
	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
}

void MenuShell::RenderBackground(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	aurora.Render(target);
	backdrop.Render(target);
	sparks.Render(target);
}

void MenuShell::RenderOverlay(sf::RenderTarget& target)
{
	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}

std::unique_ptr<MenuScreen> MenuShell::BuildHomeScreen(std::size_t returnEntryIndex)
{
	return std::make_unique<MainMenuScreen>(*this, false, returnEntryIndex);
}
