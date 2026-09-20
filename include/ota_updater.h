#pragma once

#include <Arduino.h>

struct OtaReleaseInfo {
    bool available;
    String tagName;
    String assetName;
    size_t assetSize;
    String error;
};

class OtaUpdater {
public:
    OtaReleaseInfo getLatestRelease();
    bool installLatestRelease(String& error);

private:
    static constexpr const char* GITHUB_API_URL =
        "https://api.github.com/repos/set-st/rotating_table/releases/latest";
    bool findFirmwareAsset(const String& releaseJson, String& downloadUrl,
                           String& tagName, String& assetName, size_t& assetSize,
                           String& error);
};

extern OtaUpdater otaUpdater;
