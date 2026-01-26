#include <cerrno>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>

extern "C" {
#include "libkakasi.h"
}
#include <iconv.h>

#include "JapaneseConverter.hpp"
#include "Log.hpp"
#include "ModConfig.hpp"

namespace {

std::mutex kakasiMutex;
std::once_flag kakasiInitFlag;
bool kakasiInitOk = false;
std::string kakasiDictPath;
std::once_flag kakasiInitFailureLogFlag;
std::once_flag kakasiDictMissingLogFlag;
std::once_flag kakasiIconvFailureLogFlag;
std::once_flag kakasiDoFailureLogFlag;
std::once_flag kakasiConvertFailureLogFlag;

bool hasKakasiDicts(const std::filesystem::path& dir) {
    return std::filesystem::exists(dir / "kanwadict")
        && std::filesystem::exists(dir / "itaijidict");
}

void initKakasiOnce() {
    std::call_once(kakasiInitFlag, []() {
        const char* argv[] = {
            "kakasi",
            "-Ha",
            "-Ka",
            "-Ja",
            "-Ea",
            "-s",
            "-i",
            "euc",
            "-o",
            "ascii",
            nullptr
        };
        constexpr int argc = 10;

        auto setEnvVar = [](const char* key, const std::string& path) {
#if defined(_WIN32)
            _putenv_s(key, path.c_str());
#else
            setenv(key, path.c_str(), 1);
#endif
        };

        const char* existingPath = std::getenv("KANWADICTPATH");
        const char* existingItaiPath = std::getenv("ITAIJIDICTPATH");
        std::filesystem::path dictDir;

        if (existingPath && *existingPath) {
            dictDir = existingPath;
            if (std::filesystem::is_regular_file(dictDir)) {
                dictDir = dictDir.parent_path();
            }
        } else {
            dictDir = SpotifySearchKakasi::getDataDirectory() / "kakasi";
        }

        if (!hasKakasiDicts(dictDir)) {
            std::call_once(kakasiDictMissingLogFlag, [dictDir]() {
                SpotifySearchKakasi::Log.warn("Kakasi dicts missing in {}", dictDir.string());
            });
            kakasiInitOk = false;
            return;
        }

        const std::filesystem::path kanwaPath = dictDir / "kanwadict";
        const std::filesystem::path itaiPath = dictDir / "itaijidict";
        setEnvVar("KANWADICTPATH", kanwaPath.string());
        setEnvVar("ITAIJIDICTPATH", itaiPath.string());
        kakasiDictPath = dictDir.string();

        const int rc = kakasi_getopt_argv(argc, const_cast<char**>(argv));
        kakasiInitOk = rc == 0;
        if (!kakasiInitOk) {
            std::call_once(kakasiInitFailureLogFlag, [rc]() {
                SpotifySearchKakasi::Log.warn("Kakasi init failed (rc={})", rc);
            });
        } else {
            SpotifySearchKakasi::Log.info("Kakasi initialized with dict dir {}", dictDir.string());
        }
    });
}

bool convertUtf8ToEucJp(const std::string& text, std::string& output) {
    output.clear();
    if (text.empty()) {
        return true;
    }

    const iconv_t invalidIconv = reinterpret_cast<iconv_t>(-1);
    iconv_t cd = iconv_open("EUC-JP//IGNORE", "UTF-8");
    if (cd == invalidIconv) {
        cd = iconv_open("EUC-JP", "UTF-8");
    }
    if (cd == invalidIconv) {
        const int err = errno;
        std::call_once(kakasiIconvFailureLogFlag, [err]() {
            SpotifySearchKakasi::Log.warn("iconv_open failed (errno={})", err);
        });
        return false;
    }

    std::string buffer;
    buffer.resize(text.size() * 3 + 8);
    char* inBuf = const_cast<char*>(text.data());
    size_t inBytes = text.size();
    char* outBuf = buffer.data();
    size_t outBytes = buffer.size();

    while (true) {
        errno = 0;
        const size_t rc = iconv(cd, &inBuf, &inBytes, &outBuf, &outBytes);
        if (rc != static_cast<size_t>(-1)) {
            break;
        }
        if (errno == E2BIG) {
            const size_t used = buffer.size() - outBytes;
            buffer.resize(buffer.size() * 2);
            outBuf = buffer.data() + used;
            outBytes = buffer.size() - used;
            continue;
        }
        const int err = errno;
        std::call_once(kakasiIconvFailureLogFlag, [err]() {
            SpotifySearchKakasi::Log.warn("iconv conversion failed (errno={})", err);
        });
        iconv_close(cd);
        return false;
    }

    iconv_close(cd);
    buffer.resize(buffer.size() - outBytes);
    output = std::move(buffer);
    return true;
}

std::string romanizeWithKakasi(const std::string& text) {
    initKakasiOnce();
    if (!kakasiInitOk) {
        return "";
    }

    std::string eucText;
    if (!convertUtf8ToEucJp(text, eucText) || eucText.empty()) {
        return "";
    }

    std::lock_guard<std::mutex> lock(kakasiMutex);
    std::string mutableText = eucText;
    char* result = kakasi_do(mutableText.data());
    if (!result) {
        std::call_once(kakasiDoFailureLogFlag, []() {
            SpotifySearchKakasi::Log.warn("kakasi_do returned null.");
        });
        return "";
    }

    return std::string(result);
}

bool convertWithKakasi(const char* utf8, char* outBuffer, size_t outBufferSize) {
    if (!utf8 || !outBuffer || outBufferSize == 0) {
        return false;
    }

    const std::string input(utf8);
    if (input.empty()) {
        outBuffer[0] = '\0';
        return true;
    }

    const std::string romaji = romanizeWithKakasi(input);
    if (romaji.empty()) {
        return false;
    }
    if (romaji.size() + 1 > outBufferSize) {
        std::call_once(kakasiConvertFailureLogFlag, []() {
            SpotifySearchKakasi::Log.warn("Converter output buffer too small.");
        });
        return false;
    }

    std::memcpy(outBuffer, romaji.c_str(), romaji.size() + 1);
    return true;
}

const SpotifySearch::IJapaneseConverter kConverter{
    1,
    "kakasi",
    &convertWithKakasi
};

}

extern "C" __attribute__((visibility("default")))
const SpotifySearch::IJapaneseConverter* airbuds_search_get_japanese_converter_v1() {
    return &kConverter;
}
