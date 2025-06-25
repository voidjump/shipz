#include "common/level.h"

#include <SDL3/SDL.h>

#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

#include "common/base.h"
#include "utils/log.h"
#include "common/other.h"
#include "common/team.h"

LevelData lvl;

// Constructor
LevelData::LevelData() {
    m_width = 0;
    m_height = 0;
}

std::map<std::string, SHIPZ_TEAM> teams_from_string{
    {"red", SHIPZ_TEAM::RED},
    {"blue", SHIPZ_TEAM::BLUE},
    {"neutral", SHIPZ_TEAM::NEUTRAL},
};

void LevelData::SetFile(std::string filename) { m_filename = filename; }

// Load a level
bool LevelData::Load() {
    // Validate inputs
    if(m_filename.length() == 0) {
        logger::error("refusing to load empty file");
        return false;
    }

    std::string filepath =
        std::string(SHAREPATH) + std::string("./levels/") + m_filename;
    std::ifstream file_handle(filepath);

    logger::info("reading levelfile: ", m_filename);

    try {
        nlohmann::json data = nlohmann::json::parse(file_handle);
        m_author = data.at("author");
        m_name = data.at("name");
        // m_levelversion = data.at("version") & 0xFF;
        m_levelversion = data["version"].template get<Uint16>() & 0xFF;
        m_num_bases = data["bases"].template get<Uint16>() & 0xFF;
        m_colmap_filename = data.at("collisionmap");
        m_image_filename = data.at("image");
        auto base_list = data.at("base_list");
        for (int idx = 0; idx < m_num_bases; idx++) {
            m_bases.push_back(LevelBase{
                teams_from_string[base_list.at(idx).at("team")],
                base_list.at(idx).at("x"), base_list.at(idx).at("y")});
        }

    } catch (const nlohmann::json::exception &e) {
        logger::error("failed to open level file: ", e.what());
        return false;
    }

    // Get level dimensions
    SDL_Surface *level = LoadIMG(m_image_filename.c_str());
    if (!level) {
        logger::error("failed to load level image");
        return false;
    }
    m_width = level->w;
    m_height = level->h;
    SDL_DestroySurface(level);

    logger::info(" . author:", m_author);
    logger::info(" . name:", m_name);
    logger::info(" . version:", m_levelversion);
    logger::info(" . bases:", m_num_bases);
    logger::info(" . collisionmap:", m_colmap_filename);
    logger::info(" . image:", m_image_filename);

    logger::info(" . size:", m_width, " x ", m_height);
    logger::info("done reading");
    return true;
}