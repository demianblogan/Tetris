#include "HudCategoryPanel.h"

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "OptionsSfx.h"

namespace
{
	namespace Sfx = OptionsSfx;

	constexpr sf::FloatRect PanelBounds{ { 680.f, 286.f }, { 1120.f, 508.f } };
	constexpr float RowsTop = PanelBounds.position.y + 60.f;
	constexpr float RowMargin = 88.f;
	constexpr float RowHeight = 92.f;
	constexpr float RowGap = 14.f;
}

HudCategoryPanel::HudCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds, context.textures.Get(Assets::TextureID::UiFrameWhiteRed))
{
	BuildRows();
}

bool HudCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	return a.showControlsLegend == b.showControlsLegend;
}

GameSettings HudCategoryPanel::DefaultSettings() const
{
	return GameSettings{};
}

void HudCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);
	const sf::Texture& checkbox = context.textures.Get(Assets::TextureID::Checkbox);

	rows.clear();

	auto legendRow = std::make_unique<UI::ToggleRow>(font, text.GetText(TextKey::Options::HudControlsLegend),
		checkbox, working.showControlsLegend, [this](bool on) { working.showControlsLegend = on; });
	legendRowPtr = legendRow.get();
	rows.push_back(std::move(legendRow));

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
}

void HudCategoryPanel::SyncRows()
{
	legendRowPtr->SetOn(working.showControlsLegend);
}

void HudCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	saved.showControlsLegend = working.showControlsLegend;

	// Picked up the next time a game starts (GameplayState reads it in its ctor).
	context.settings.Save();
}

void HudCategoryPanel::ResetWorking()
{
	const GameSettings defaults;
	working.showControlsLegend = defaults.showControlsLegend;
	SyncRows();
}

void HudCategoryPanel::AdjustRow(std::size_t index, int direction)
{
	if (index < rows.size())
	{
		rows[index]->Adjust(direction);
		Sfx::Toggle(context.audioPlayer);
	}
}

void HudCategoryPanel::ActivateRow(std::size_t index)
{
	if (index < rows.size())
	{
		rows[index]->Activate();
		Sfx::Toggle(context.audioPlayer);
	}
}

void HudCategoryPanel::RowClicked(std::size_t /*index*/)
{
	Sfx::Toggle(context.audioPlayer);
}
