#include "scotland2/shared/modloader.h"

#include "_config.hpp"
#include "Log.hpp"
#include "ModConfig.hpp"

MOD_EXTERN_FUNC void setup(CModInfo* info) noexcept {
    *info = SpotifySearchKakasi::modInfo.to_c();
    Paper::Logger::RegisterFileContextId("airbuds-search-kakasi");
    SpotifySearchKakasi::Log.info("Version: {}", info->version);
}

MOD_EXTERN_FUNC void late_load() noexcept {
    SpotifySearchKakasi::Log.info("Kakasi adapter loaded.");
}
