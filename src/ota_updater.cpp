#include "ota_updater.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

OtaUpdater otaUpdater;

bool OtaUpdater::findFirmwareAsset(const String& releaseJson,
                                   String& downloadUrl, String& tagName,
                                   String& assetName, size_t& assetSize,
                                   String& error) {
    JsonDocument doc;
    if (deserializeJson(doc, releaseJson)) {
        error = "Некоректна відповідь GitHub API";
        return false;
    }

    tagName = doc["tag_name"] | "";
    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        const String name = asset["name"] | "";
        if (!name.endsWith(".bin")) {
            continue;
        }
        downloadUrl = asset["browser_download_url"] | "";
        assetName = name;
        assetSize = asset["size"] | 0;
        if (downloadUrl.length() > 0 && assetSize > 0) {
            return true;
        }
    }

    error = "У релізі GitHub не знайдено BIN-файл прошивки";
    return false;
}

OtaReleaseInfo OtaUpdater::getLatestRelease() {
    OtaReleaseInfo info = {};
    if (WiFi.status() != WL_CONNECTED) {
        info.error = "OTA потребує підключення ESP32 до роутера з доступом до Інтернету; режим SoftAP не підходить";
        return info;
    }
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15);
    HTTPClient http;
    http.setTimeout(20000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (!http.begin(client, GITHUB_API_URL)) {
        info.error = "Не вдалося підключитися до GitHub";
        return info;
    }
    http.addHeader("User-Agent", "rotating-table-esp32");
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        info.error = "Помилка підключення до GitHub: " + String(code) +
                     " (" + HTTPClient::errorToString(code) + ")";
        http.end();
        return info;
    }

    String body = http.getString();
    http.end();
    String downloadUrl;
    if (findFirmwareAsset(body, downloadUrl, info.tagName, info.assetName,
                           info.assetSize, info.error)) {
        info.available = true;
    }
    return info;
}

bool OtaUpdater::installLatestRelease(String& error) {
    OtaReleaseInfo release = getLatestRelease();
    if (!release.available) {
        error = release.error;
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15);
    HTTPClient http;
    http.setTimeout(30000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (!http.begin(client, String("https://github.com/set-st/rotating_table/releases/download/") +
                              release.tagName + "/" + release.assetName)) {
        error = "Не вдалося відкрити BIN-файл релізу";
        return false;
    }
    http.addHeader("User-Agent", "rotating-table-esp32");
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        error = "Помилка завантаження прошивки: " + String(code) +
                " (" + HTTPClient::errorToString(code) + ")";
        http.end();
        return false;
    }

    const int contentLength = http.getSize();
    if (contentLength <= 0 || (release.assetSize > 0 &&
                               static_cast<size_t>(contentLength) != release.assetSize)) {
        error = "Некоректний розмір BIN-файла";
        http.end();
        return false;
    }
    if (!Update.begin(static_cast<size_t>(contentLength))) {
        error = "Недостатньо місця для OTA-оновлення";
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    const size_t written = Update.writeStream(*stream);
    const bool success = written == static_cast<size_t>(contentLength) &&
                         Update.end() && Update.isFinished();
    http.end();
    if (!success) {
        Update.abort();
        error = "Помилка запису прошивки: " + String(Update.getError());
        return false;
    }
    return true;
}
