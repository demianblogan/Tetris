#pragma once

#include <filesystem>
#include <vector>

#include "HighScoreEntry.h"

class HighScoreManager
{
private:
	std::vector<HighScoreEntry> records;
	std::filesystem::path filepath;

public:
	static constexpr std::size_t MAX_RECORDS = 10;

	// Bumped whenever the on-disk layout changes; a file with a different
	// version is preserved as .corrupt and the board starts empty.
	// v2: each entry gained its lines-cleared and level-reached counts.
	static constexpr int FormatVersion = 2;

	HighScoreManager(const std::filesystem::path& filepath);

	void Load();
	void Save() const;
	void AddRecord(const HighScoreEntry& entry);
	void Clear();

	[[nodiscard]] bool IsHighScore(int score) const;
	[[nodiscard]] const std::vector<HighScoreEntry>& GetRecords() const;
};