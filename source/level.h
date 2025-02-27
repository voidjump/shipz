#ifndef SHIPZ_LEVEL_H
#define SHIPZ_LEVEL_H

#define LEVEL_MAX_DIMENSION 4096
#include <string>
#include <vector>
#include <SDL3/SDL.h>


struct LevelBase {
    int owner;  // RED, BLUE or NEUTRAL
    int x;
    int y;
};

class LevelData {
   public:
    Sint16 m_width, m_height;
    Uint16 m_levelversion, m_num_bases;
    std::string m_filename, m_name, m_author, m_image_filename,
        m_colmap_filename;
    std::vector<LevelBase> m_bases;

    LevelData();
    // Load A level from a file
    bool Load();
    void SetFile(std::string filename);
};

extern LevelData lvl;

#endif