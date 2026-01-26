#pragma once

#include <filesystem>

#include "beatsaber-hook/shared/config/config-utils.hpp"

namespace SpotifySearchKakasi {

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

std::filesystem::path getDataDirectory();

}
