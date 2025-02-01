#include <iostream>
#include <fstream>
#include <map>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include "level.h"
#include "base.h"
#include "team.h"
#include "other.h"

LevelData lvl;

// Constructor
LevelData::LevelData() {
	m_width = 0;
	m_height = 0;
}

std::map<std::string, SHIPZ_TEAM> teams_from_string {
	{"red", SHIPZ_TEAM::RED},
	{"blue", SHIPZ_TEAM::BLUE},
	{"neutral", SHIPZ_TEAM::NEUTRAL},
};

void LevelData::SetFile(const char * filename ) {
	std::cout << "setting file: " << filename << std::endl;
	std::cout << strlen(filename) << std::endl;
	m_filename.assign(filename);
}

bool LevelData::Load() {

	std::string filepath = std::string(SHAREPATH) + std::string("./levels/") + m_filename;
	std::ifstream file_handle(filepath);

	std::cout << std::endl << "@ reading levelfile: " << m_filename << std::endl;

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
		for( int idx = 0; idx < m_num_bases; idx++ ) {
			m_bases.push_back(LevelBase{ teams_from_string[base_list.at(idx).at("team")],
							base_list.at(idx).at("x"), 
							base_list.at(idx).at("y")});
		}


	} catch (const nlohmann::json::exception& e) {
		std::cout << "failed to open level file: " << e.what() << std::endl;
		return false;
	}

	// Get level dimensions 
	SDL_Surface *level = LoadIMG( m_image_filename.c_str() );
	if( !level ) {
		std::cout << "failed to load level image" << std::endl;
		return false;
	}
	m_width = level->w;
	m_height = level->h;
	SDL_DestroySurface(level);

	// Legacy stuff here, TODO: remove
	std::cout << "  author: " << m_author << std::endl;
	std::cout << "  name: " << m_name << std::endl;
	std::cout << "  version: " << (int) m_levelversion << std::endl;
	std::cout << "  bases: " << (int) m_num_bases << std::endl;
	std::cout << "  collisionmap: " << m_colmap_filename << std::endl;
	std::cout << "  image: " << m_image_filename << std::endl;

	uint base_index = 0;
	for( auto &base_iter : m_bases ) {
		if( base_iter.owner == SHIPZ_TEAM::RED )
		{
			red_team.bases++;
		}
		if( base_iter.owner == SHIPZ_TEAM::BLUE )
		{
			blue_team.bases++;
		}
		Base * baseptr = &bases[base_index];
		baseptr->used = 1;
		baseptr->owner = base_iter.owner;
		std::cout << "  base team " << baseptr->owner << " :";
		baseptr->x = base_iter.x;
		baseptr->y = base_iter.y;
		std::cout << baseptr->x <<", " << baseptr->y << std::endl;
		base_index++;
	}

	std::cout << "  size: " << m_width << " x " << m_height << std::endl;
	std::cout << "@ done reading." << std::endl;
	return true;
}