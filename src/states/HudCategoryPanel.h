#pragma once

#include <cstddef>

#include <SFML/Graphics/Color.hpp>

#include "SettingsCategoryPanel.h"

struct Context;

// HUD settings: a single toggle for the always-on in-game controls legend.
class HudCategoryPanel final : public SettingsCategoryPanel
{
public:
	HudCategoryPanel(Context& context, sf::Color accent);

protected:
	void BuildRows() override;
	[[nodiscard]] bool SettingsEqual(const GameSettings& a, const GameSettings& b) const override;
	[[nodiscard]] GameSettings DefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index) override;

private:
	void SyncRows();

	UI::ToggleRow* legendRowPtr = nullptr;
};
