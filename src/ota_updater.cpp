#include "ota_updater.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include "table_ble_client.h"

OtaUpdater otaUpdater;

bool OtaUpdater::findFirmwareAsset(const String& releaseJson,
                                   const String& expectedAssetName,
                                   OtaReleaseInfo& info) {
    JsonDocument doc;
    if (deserializeJson(doc, releaseJson)) {
        info.error = "Некоректна відповідь GitHub API";
        return false;
    }

    info.tagName = doc["tag_name"] | "";
    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        const String name = asset["name"] | "";
        if (name != expectedAssetName) {
            continue;
        }
        info.downloadUrl = asset["browser_download_url"] | "";
        info.assetName = name;
        info.assetSize = asset["size"] | 0;
        if (info.downloadUrl.length() > 0 && info.assetSize > 0) {
            return true;
        }
    }

    info.error = "У релізі GitHub не знайдено файл " + expectedAssetName;
    return false;
}

OtaReleaseInfo OtaUpdater::getLatestRelease(const String& assetName) {
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
    if (findFirmwareAsset(body, assetName, info)) {
        info.available = true;
    }
    return info;
}

bool OtaUpdater::installLatestRelease(String& error) {
    OtaReleaseInfo release = getLatestRelease("rotating_table.bin");
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
    if (!http.begin(client, release.downloadUrl)) {
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
        const esp_partition_t* nextPartition = esp_ota_get_next_update_partition(nullptr);
        if (nextPartition && static_cast<size_t>(contentLength) > nextPartition->size) {
            error = "Прошивка (" + String(contentLength) + " байт) більша за доступний "
                    "OTA-розділ (" + String(nextPartition->size) +
                    " байт). Для переходу з v1.0.6 потрібне одноразове прошивання "
                    "основної плати через USB з новою таблицею розділів.";
        } else {
            error = "Не вдалося розпочати OTA-оновлення: " +
                    String(Update.errorString());
        }
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

bool OtaUpdater::installTiltLatestRelease(TableBleClient& bleClient,
                                          String& error) {
    if (WiFi.status() != WL_CONNECTED) {
        error = "OTA потребує підключення основної плати до Wi-Fi з Інтернетом";
        return false;
    }
    const size_t chunkSize = bleClient.getOtaChunkSize();
    if (chunkSize == 0) {
        error = "Плата нахилу не підключена або не підтримує BLE OTA; оновіть її через USB";
        return false;
    }

    OtaReleaseInfo release = getLatestRelease("tilt_platform.bin");
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
    if (!http.begin(client, release.downloadUrl)) {
        error = "Не вдалося відкрити BIN-файл плати нахилу";
        return false;
    }
    http.addHeader("User-Agent", "rotating-table-esp32");
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        error = "Помилка завантаження прошивки slave: " + String(code) +
                " (" + HTTPClient::errorToString(code) + ")";
        http.end();
        return false;
    }

    const int contentLength = http.getSize();
    if (contentLength <= 0 ||
        static_cast<size_t>(contentLength) != release.assetSize) {
        error = "Некоректний розмір BIN-файла плати нахилу";
        http.end();
        return false;
    }
    String slaveStatus;
    Serial.printf("[BLE OTA] Початок OTA для slave: size=%d байт\n", contentLength);
    if (!bleClient.beginTiltOta(static_cast<size_t>(contentLength), &slaveStatus)) {
        if (slaveStatus.length() > 0 && slaveStatus != "") {
            error = "Slave відхилив початок OTA-передачі: " + slaveStatus;
        } else {
            error = "Slave відхилив початок OTA-передачі";
        }
        Serial.printf("[BLE OTA] Slave відповів на BEGIN: '%s'\n", slaveStatus.c_str());
        http.end();
        return false;
    }
    Serial.printf("[BLE OTA] Slave прийняв BEGIN: '%s'\n", slaveStatus.c_str());

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[512];
    size_t remaining = static_cast<size_t>(contentLength);
    size_t transferred = 0;
    bool success = true;
    while (remaining > 0) {
        const size_t requestSize = min(remaining, min(chunkSize, sizeof(buffer)));
        const size_t received =
            stream->readBytes(reinterpret_cast<char*>(buffer), requestSize);
        if (received != requestSize ||
            !bleClient.writeTiltOtaChunk(buffer, received)) {
            success = false;
            error = "BLE OTA-передача перервалася на " +
                    String(static_cast<unsigned int>(transferred)) + " байтах";
            Serial.printf("[BLE OTA] Передача перервалася: transferred=%u, remaining=%u, request=%u\n",
                          static_cast<unsigned int>(transferred),
                          static_cast<unsigned int>(remaining),
                          static_cast<unsigned int>(requestSize));
            break;
        }
        transferred += received;
        remaining -= received;
        vTaskDelay(1);
    }
    http.end();

    if (success) {
        Serial.println("[BLE OTA] Відправка даних завершена. Очікуємо FINISH...");
        success = bleClient.finishTiltOta();
        if (!success) {
            error = "Slave не підтвердив завершення запису прошивки";
            Serial.println("[BLE OTA] Slave не підтвердив FINISH");
        }
    }
    if (!success) {
        bleClient.abortTiltOta();
        return false;
    }
    return true;
}
