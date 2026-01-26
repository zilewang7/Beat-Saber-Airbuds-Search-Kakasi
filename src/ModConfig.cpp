#include "ModConfig.hpp"

namespace SpotifySearchKakasi {

std::filesystem::path getDataDirectory() {
    static const std::string dataDirectoryString = getDataDir(modInfo);
    static const std::filesystem::path dataDirectory{dataDirectoryString};
    if (!std::filesystem::exists(dataDirectory)) {
        std::filesystem::create_directory(dataDirectory);
    }
    return dataDirectory;
}

}
