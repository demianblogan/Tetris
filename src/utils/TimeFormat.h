#pragma once

#include <algorithm>
#include <string>

namespace TimeFormat
{
	// Whole seconds as "M:SS" (no hour rollover -- a Tetris run never gets there).
	[[nodiscard]] inline std::string MinutesSeconds(float seconds)
	{
		const int total = std::max(0, static_cast<int>(seconds));
		const int minutes = total / 60;
		const int rest = total % 60;
		return std::to_string(minutes) + ":" + (rest < 10 ? "0" : "") + std::to_string(rest);
	}
}
