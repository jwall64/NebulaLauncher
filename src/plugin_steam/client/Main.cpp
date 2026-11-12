/*
 * Copyright 2026 jwall64
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Core/Types.hpp>
#include <Core/Result.hpp>
#include <Core/String.hpp>
#include <Core/Array.hpp>
#include <Platform/Allocator.hpp>
#include <Platform/DynamicLibrary.hpp>
#include <Platform/Logging.hpp>
#include <Platform/Process.hpp>
#include <Platform/PlatformDefines.hpp>
#include <Platform/Time.hpp>
#include <Utility/StringBuilder.hpp>
#include <Codec/Json.hpp>
#include <Codec/Base64.hpp>

#include "SchemaBin.hpp"

#include <openssl/evp.h>
#include <steam/steam_api.h>

typedef void*(*pfnCreateInterface)(const char*,i32*);
typedef bool(*pfnBGetCallback)(HSteamPipe,CallbackMsg_t*,bool*);
typedef bool(*pfnFreeLastCallback)(HSteamPipe);

namespace {

nbl::DynamicLibrary _library;
pfnCreateInterface _pfnCreateInterface;
pfnBGetCallback _pfnBGetCallback;
pfnFreeLastCallback _pfnFreeLastCallback;

nbl::StringView _getSteamClientPath() {

    static nbl::StringView _out{};
    if(_out.empty()) {

#if defined(NBL_PLATFORM_LINUX)

        static char _path[4096];
        
        nbl::StringView home = nbl::getEnv("HOME");
        nbl::InPlaceAllocator alloc(_path, 4096);
        NBL_ASSERT(nbl::StringBuilder(&alloc).append(home).append("/.steam/steam/linux64/steamclient.so").buildNullTerminated(&_out).success());

#endif
    }

    return _out;
}

nbl::StringView _getSteamInstallPath() {

    static nbl::StringView _out{};
    if(_out.empty()) {
        nbl::StringView client = _getSteamClientPath(); 
#if defined(NBL_PLATFORM_LINUX)
        _out = client.slice(0, client.length() - 22);
#endif
    }

    return _out;
}

}

int main(int argc, char** argv) {

    NBL_ASSERT(nbl::DynamicLibrary::load(_getSteamClientPath(), &_library).success());
    NBL_ASSERT(_library.findSymbol("CreateInterface", &_pfnCreateInterface).success());
    NBL_ASSERT(_library.findSymbol("Steam_BGetCallback", &_pfnBGetCallback).success());
    NBL_ASSERT(_library.findSymbol("Steam_FreeLastCallback", &_pfnFreeLastCallback).success());

    u8 buffer[4096]{};
    nbl::BufferedWriter bufferedWriter(nbl::stdoutWriter(), buffer, 4096);
    nbl::JsonWriter writer(bufferedWriter);

    nbl::StringView appIdStr = nbl::getEnv("SteamAppId");
    if(appIdStr.empty()) {
        writer.writeObject([&](auto& obj) {
            obj.add("success", false);
            obj.add("code", -1);
            obj.add("error", "SteamAppId is unset");
            return nbl::Result::SUCCESS;
        });
        return 0;
    }

    i32 rc;
    ISteamClient* client = static_cast<ISteamClient*>(_pfnCreateInterface(STEAMCLIENT_INTERFACE_VERSION, &rc));
    if(rc != 0 || client == nullptr) {
        writer.writeObject([&](auto& obj) {
            obj.add("success", false);
            obj.add("code", -2);
            obj.add("error", "Failed to create Steam client.");
            return nbl::Result::SUCCESS;
        });
        return 0;
    }

    HSteamPipe pipe = client->CreateSteamPipe();
    HSteamUser user = client->ConnectToGlobalUser(pipe);

    if(pipe == 0 || user == 0) {
        writer.writeObject([&](auto& obj) {
            obj.add("success", false);
            obj.add("code", -3);
            obj.add("error", "Failed to connect to Steam. Is it running?");
            return nbl::Result::SUCCESS;
        });
        return 0;
    }


    ISteamUserStats* stats = client->GetISteamUserStats(user, pipe, STEAMUSERSTATS_INTERFACE_VERSION);
    CSteamID id = client->GetISteamUser(user, pipe, STEAMUSER_INTERFACE_VERSION)->GetSteamID();
    
    bool userStatsReady = false;
    bool globalPercentagesReady = false;
    stats->RequestUserStats(id);

    u64 startTime = nbl::getCurrentTime();

    while(!userStatsReady || !globalPercentagesReady) {
        CallbackMsg_t msg{};
        bool failed = false;
        bool result = _pfnBGetCallback(pipe, &msg, &failed);

        if(result && !failed) {

            _pfnFreeLastCallback(pipe);
            if(msg.m_iCallback == UserStatsReceived_t::k_iCallback) {
                userStatsReady = true;

                if(stats->GetNumAchievements() == 0) {
                    writer.writeObject([&](auto& obj) {
                        obj.add("success", true);
                        obj.add("achievements", nbl::JsonArray{});
                        obj.add("revision_hash", "0");
                        obj.add("latest_unlock", 0);
                        return nbl::Result::SUCCESS;
                    });
                    return 0;
                }

                f32 probe = 1.0f;
                if(!stats->GetAchievementAchievedPercent(stats->GetAchievementName(0), &probe) || probe == 0.0f) {
                    stats->RequestGlobalAchievementPercentages();
                } else {
                    globalPercentagesReady = true;
                }
            } else if(msg.m_iCallback == GlobalAchievementPercentagesReady_t::k_iCallback) {
                globalPercentagesReady = true;
            }
        } else {
            // Steam's callback pump is polled, not blocking. Sleep briefly so
            // the timeout below is still checked while nothing is happening.
            nbl::sleep(10);
        }

        if(nbl::getCurrentTime() - startTime > 5000) {
            break;
        }
    }

    if(!userStatsReady) {
        // Without user stats the achievement list cannot be trusted. Bail out
        // instead of reporting an empty set and wiping the database.
        writer.writeObject([&](auto& obj) {
            obj.add("success", false);
            obj.add("code", -4);
            obj.add("error", "Timed out waiting for Steam user stats.");
            return nbl::Result::SUCCESS;
        });
        return 0;
    }


    // Load schema from disk
    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView schemaPath{};
    NBL_CHECK(nbl::StringBuilder(&frame)
            .append(_getSteamInstallPath())
            .append("appcache/stats/UserGameStatsSchema_")
            .append(nbl::getEnv("SteamAppId"))
            .append(".bin")
            .build(&schemaPath)
    );

    nbl::JsonObject schema{};
    NBL_CHECK(parseSchemaBin(schemaPath, &schema, &frame));

    u64 latestUnlock = 0;
    nbl::Array<u8, 32> hash{};

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if(ctx == nullptr) {
        return nbl::Result::LIBRARY_ERROR;
    }

    if(EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return nbl::Result::LIBRARY_ERROR;
    }

    writer.writeObject([&](auto& obj) {
        obj.add("success", true);
        obj.writeArray("achievements", [&](auto& array) {

            nbl::JsonObject schemaStats = schema
                .getObject(nbl::getEnv("SteamAppId"))
                .getObject("stats");

            u64 index = 0;
            for(nbl::JsonObject::Entry& ent : schemaStats) {
                if(ent.value.asObject().getString("type") != "ACHIEVEMENTS") {
                    continue;
                }
                nbl::JsonObject bits = ent.value.asObject().getObject("bits");

                for(nbl::JsonObject::Entry& bit : bits) {

                    nbl::JsonObject displayObj = bit.value.asObject().getObject("display");

                    const char* name = bit.value.asObject().getString("name").data();
                    const char* displayName = stats->GetAchievementDisplayAttribute(name, "name");
                    const char* description = stats->GetAchievementDisplayAttribute(name, "desc");

                    u64 number = index++;
                    bool hidden = displayObj.getNumber("hidden").asInt() != 0;
                    nbl::StringView icon = displayObj.getString("icon");
                    nbl::StringView iconGray = displayObj.getString("icon_gray");

                    // Digest all of the schema data so changes can be detected efficiently.
                    if(EVP_DigestUpdate(ctx, name, strlen(name)) != 1
                        || EVP_DigestUpdate(ctx, displayName, strlen(displayName)) != 1
                        || EVP_DigestUpdate(ctx, description, strlen(description)) != 1
                        || EVP_DigestUpdate(ctx, &number, sizeof(u64)) != 1
                        || EVP_DigestUpdate(ctx, &hidden, sizeof(bool)) != 1
                        || EVP_DigestUpdate(ctx, icon.data(), icon.length()) != 1
                        || EVP_DigestUpdate(ctx, iconGray.data(), iconGray.length()) != 1) {

                        EVP_MD_CTX_free(ctx);
                        return nbl::Result::LIBRARY_ERROR;
                    }

                    array.writeObject([&](auto& object) -> nbl::Result {

                        u32 unlockTime;
                        bool unlocked;
                        stats->GetAchievementAndUnlockTime(name, &unlocked, &unlockTime);
                        
                        u64 unlockTimeMillis = static_cast<u64>(unlockTime) * 1000;
                        if(unlockTimeMillis > latestUnlock) {
                            latestUnlock = unlockTimeMillis;
                        }

                        f32 globalUnlockRate = 0.0f;
                        stats->GetAchievementAchievedPercent(name, &globalUnlockRate);

                        object.add("name", nbl::StringView(name));
                        object.add("number", number);
                        object.add("display_name", nbl::StringView(displayName));
                        object.add("description", nbl::StringView(description));
                        object.add("hidden", hidden);
                        object.add("unlocked", unlocked);
                        object.add("unlock_time", unlockTimeMillis);
                        object.add("global_unlock_rate", globalUnlockRate);
                        object.writeString("icon", [&](nbl::Writer w) -> nbl::Result {
                            NBL_CHECK(w.writeString("https://cdn.steamstatic.com/steamcommunity/public/images/apps/"));
                            NBL_CHECK(w.writeString(nbl::getEnv("SteamAppId")));
                            NBL_CHECK(w.writeChar('/'));
                            NBL_CHECK(w.writeString(icon));
                            return nbl::Result::SUCCESS;
                        });
                        object.writeString("icon_gray", [&](nbl::Writer w) -> nbl::Result {
                            NBL_CHECK(w.writeString("https://cdn.steamstatic.com/steamcommunity/public/images/apps/"));
                            NBL_CHECK(w.writeString(nbl::getEnv("SteamAppId")));
                            NBL_CHECK(w.writeChar('/'));
                            NBL_CHECK(w.writeString(iconGray));
                            return nbl::Result::SUCCESS;
                        });

                        return nbl::Result::SUCCESS;
                    });
                }
            }
            return nbl::Result::SUCCESS;
        });

        u32 length = hash.size();
        if(EVP_DigestFinal_ex(ctx, hash.data, &length) != 1) {
            EVP_MD_CTX_free(ctx);
            return nbl::Result::LIBRARY_ERROR;
        }
        EVP_MD_CTX_free(ctx);

        obj.writeString("revision_hash", [&](nbl::Writer writer) -> nbl::Result { 
            return nbl::write(nbl::HexEncoder({ hash.data, length }), writer);
        });

        obj.add("latest_unlock", latestUnlock);
        return nbl::Result::SUCCESS;
    });

    client->ReleaseUser(pipe, user);
    client->BReleaseSteamPipe(pipe);

    return 0;

}
