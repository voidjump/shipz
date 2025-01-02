#ifndef SHIPZ_LEVEL_H
#define SHIPZ_LEVEL_H

struct LevelBase {
	int owner; // RED, BLUE or NEUTRAL
	int x;
	int y;
};

class LevelData
{
	public:
	Sint16 m_width, m_height;
	Uint8 m_levelversion, m_num_bases;
	std::string m_filename, m_name, m_author, m_image_filename, m_colmap_filename;
	std::vector<LevelBase> m_bases;

	LevelData();
	// Load A level from a file
	bool Load();
	void SetFile(const char *);
};

extern LevelData lvl;

#endif