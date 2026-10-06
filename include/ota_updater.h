#pragma once

#include <Arduino.h>

struct OtaReleaseInfo {
    bool available;
    String tagName;
    String assetName;
    String downloadUrl;
    size_t assetSize;
    String error;
};

class TableBleClient;

class OtaUpdater {
public:
    OtaReleaseInfo getLatestRelease(
        const String& assetName = "rotating_table.bin");
    bool installLatestRelease(String& error);
    bool installTiltLatestRelease(TableBleClient& bleClient, String& error);

private:
    static constexpr const char* GITHUB_API_URL =
        "https://api.github.com/repos/set-st/rotating_table/releases/latest";
    bool findFirmwareAsset(const String& releaseJson,
                           const String& expectedAssetName,
                           OtaReleaseInfo& info);
};

extern OtaUpdater otaUpdater;
