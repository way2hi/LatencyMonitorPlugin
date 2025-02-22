#include "pch.h"
#include "LatencyMonitor.h"
#include <chrono>
#include <thread>

#pragma warning (disable: 28020)

BAKKESMOD_PLUGIN(LatencyMonitor, "Latency Monitor", plugin_version, PERMISSION_ALL);

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void LatencyMonitor::InitializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cvarManager->log("WSAStartup failed with error: " + std::to_string(result));
    }
}

void LatencyMonitor::onLoad() {
    if (!pluginActiveCheck()) { cvarManager->executeCommand("plugin unload latencymonitor"); return; }
    _globalCvarManager = cvarManager;
    cvarManager->registerCvar("lmplugin_enabled", "1", "Enable the plugin").bindTo(pluginActive);
	cvarManager->registerCvar("lmplugin_debug", "0", "Debug mode").bindTo(debugMode);
    InitializeWinsock();

    cvarManager->registerCvar("lmplugin_region", "EU", "Your region").bindTo(mmRegion);
    cvarManager->registerCvar("lmplugin_max_allowed_latency", "60", "Max allowed queue latency").bindTo(maxAllowedLatency);

    if (!cvarManager) { cvarManager->log("Critical error: cvarManager is null."); return; }
    if (!gameWrapper) { cvarManager->log("Critical error: gameWrapper is null."); return; }
    MatchmakingWrapper matchmaking = gameWrapper->GetMatchmakingWrapper();
    if (matchmaking.IsNull()) { cvarManager->log("Critical error: matchmakingWrapper is null."); return; }
    *mmRegion = matchmaking.GetRegionLabel(GetCurrentRegionEnum());

    cvarManager->log("Latency Monitor loaded successfully.");

    if (pluginActiveCheck()) {
        gameWrapper->HookEvent("Function OnlineGameMatchmaking_X.Searching.StartMatchmaking", [this](...) {
            if (!pluginActiveCheck()) { return; }

            if (*debugMode)
            {
                NotifyUser("Matchmaking has started!");
            }
            
            CheckLatencyAndCancelQueue();
            });

        gameWrapper->HookEvent("Function OnlineGameMatchmaking_X.Searching.EndState", [this](...) {
            if (!pluginActiveCheck()) { return; }

            if (*debugMode)
            {
                NotifyUser("Matchmaking was canceled");
            }
            
            });

        gameWrapper->HookEvent("Function OnlineSubsystemSteamworks.OnlineSubsystemSteamworks.IsOriginalAppOwner", [this](...) {
            if (!pluginActiveCheck()) { return; }
            cvarManager->log("IsOriginalAppOwner hooked");
            });
    }
}

int LatencyMonitor::pluginActiveCheck() {
    return *pluginActive;
}

void LatencyMonitor::CheckLatencyAndCancelQueue() {
    if (pluginActiveCheck()) {
        int lat = 0, maxAllowedLatency = cvarManager->getCvar("lmplugin_max_allowed_latency").getIntValue();
        cvarManager->log("Max allowed latency: " + std::to_string(maxAllowedLatency));
        MatchmakingWrapper matchmaking = gameWrapper->GetMatchmakingWrapper();
        // need to rework aaaaall of this asap because like it's just horrendous
        std::string servers_EU[] = { "18.175.175.220", "15.188.232.52", "46.182.19.48", "82.96.64.2" };
        std::string servers_USE[] = { "64.233.207.16", "8.26.56.26", "64.6.65.6" };
        std::string servers_USW[] = { "199.85.126.10", "129.250.35.250", "208.67.220.222" };
        std::string servers_OCE[] = { "139.134.5.51", "139.130.4.4", "203.50.2.71", "203.109.149.192" };
        std::string servers_ASM[] = { "123.125.81.6", "182.19.95.34", "202.78.97.41" };
        int servers_EU_size = sizeof(servers_EU) / sizeof(servers_EU[0]);
        int servers_USE_size = sizeof(servers_USE) / sizeof(servers_USE[0]);
        int servers_USW_size = sizeof(servers_USW) / sizeof(servers_USW[0]);
        int servers_OCE_size = sizeof(servers_OCE) / sizeof(servers_OCE[0]);
        int servers_ASM_size = sizeof(servers_ASM) / sizeof(servers_ASM[0]);
        if (!cvarManager) { return; }
        if (*mmRegion == "US-East") { lat = GetPingFromServerArray(servers_USE, servers_USE_size); }
        if (*mmRegion == "US-West") { lat = GetPingFromServerArray(servers_USW, servers_USW_size); }
        if (*mmRegion == "EU") { lat = GetPingFromServerArray(servers_EU, servers_EU_size); }
        if (*mmRegion == "Oceania") { lat = GetPingFromServerArray(servers_OCE, servers_OCE_size); }
        if (*mmRegion == "Asia-Mainland") { lat = GetPingFromServerArray(servers_ASM, servers_ASM_size); }
        if (lat == -1) {
            CancelQueue();
            return;
        }
        if (lat > maxAllowedLatency) {
            CancelQueue();
			if (*debugMode)
                NotifyUser("Queue canceled due to high latency (" + std::to_string(lat) + "ms).");
        }
        else {
			if (*debugMode)
                NotifyUser("Acceptable latency (" + std::to_string(lat) + "ms).");
        }
    }

}

int LatencyMonitor::GetPingFromServerArray(std::string* a, int size) {
    int ping = 0;
    for (int i = 0; i < size; i++) {
        ping += GetPing(a[i]);
    }
    return ping / size;
}

void LatencyMonitor::onUnload() {
    cvarManager->executeCommand("writeconfig", true);
    gameWrapper->UnregisterDrawables();
    gameWrapper->UnhookEvent("Function OnlineGameMatchmaking_X.Searching.StartMatchmaking");
    gameWrapper->UnhookEvent("Function OnlineGameMatchmaking_X.Searching.EndState");
    gameWrapper->UnhookEvent("Function OnlineSubsystemSteamworks.OnlineSubsystemSteamworks.IsOriginalAppOwner");
    WSACleanup();
    cvarManager->removeCvar("lmplugin_enabled");
    cvarManager->removeCvar("lmplugin_region");
    cvarManager->removeCvar("lmplugin_max_allowed_latency");
	cvarManager->removeCvar("lmplugin_debug");
    cvarManager->log("Latency Monitor unloaded.");
}

int LatencyMonitor::GetPing(const std::string& address) {
    // need to implement better logic asap this is causing freezing ingame
    HANDLE icmpHandle = IcmpCreateFile();
    if (icmpHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    struct sockaddr_in sa {};
    if (inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) <= 0) {
        IcmpCloseHandle(icmpHandle);
        return -1;
    }
    char sendData[] = "a";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    auto replyBuffer = std::make_unique<char[]>(replySize);
    DWORD result = IcmpSendEcho(
        icmpHandle,
        sa.sin_addr.S_un.S_addr,
        sendData,
        sizeof(sendData),
        NULL,
        replyBuffer.get(),
        replySize,
        100
    );
    if (result == 0) {
        IcmpCloseHandle(icmpHandle);
        return -1;
    }
    auto reply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer.get());
    int latency = reply->RoundTripTime;
    if (latency == 0)
    {
        IcmpCloseHandle(icmpHandle);
        return -1;
    }
    IcmpCloseHandle(icmpHandle);
    return latency;
}

void LatencyMonitor::CancelQueue() {
    if (!pluginActiveCheck()) { return; }
    MatchmakingWrapper matchmaking = gameWrapper->GetMatchmakingWrapper();
    matchmaking.CancelMatchmaking();
}

void LatencyMonitor::NotifyUser(std::string message) {
    _globalCvarManager = cvarManager;
    if (!pluginActiveCheck()) { return; }
    cvarManager->log(message);
    gameWrapper->Toast("LatencyMonitor", message, "default");
}

std::string LatencyMonitor::GetCurrentRegion() {
    auto regionCvar = cvarManager->getCvar("lmplugin_region");
    if (!regionCvar) {
        NotifyUser("Region detection failed. Please set your region manually.");
        return "Unknown";
    }
    return regionCvar.getStringValue();
}

void LatencyMonitor::SetCurrentRegion(std::string region) {
    *mmRegion = region;
}

// these functions can stay i guess theyre just hanging out

Region LatencyMonitor::StrToRegion(std::string regionLabel) {
    if (regionLabel == "US-East") return Region::USE;
    if (regionLabel == "US-West") return Region::USW;
    if (regionLabel == "EU") return Region::EU;
    if (regionLabel == "Oceania") return Region::OCE;
    if (regionLabel == "Asia-Mainland") return Region::ASM;
    if (regionLabel == "Middle-East") return Region::ME;
    if (regionLabel == "South-America") return Region::SAM;
    if (regionLabel == "Africa") return Region::SAF;
    return Region::EU;
}

std::string LatencyMonitor::RegionToStr(Region region) {
    switch (region) {
    case Region::USE: return "US-East";
    case Region::USW: return "US-West";
    case Region::EU: return "EU";
    case Region::OCE: return "Oceania";
    case Region::ASM: return "Asia-Mainland";
    case Region::ME: return "Middle-East";
    case Region::SAM: return "South-America";
    case Region::SAF: return "Africa";
    default: return "EU";
    }
}

int LatencyMonitor::StrToRegionInt(std::string regionLabel) {
    if (regionLabel == "US-East") return 0; // USE
    if (regionLabel == "US-West") return 1; // USW
    if (regionLabel == "EU") return 2; // EU
    if (regionLabel == "Oceania") return 3; // OCE
    if (regionLabel == "Asia-Mainland") return 4; // ASM
    if (regionLabel == "Middle-East") return 5; // ME
    if (regionLabel == "South-America") return 6; // SAM
    if (regionLabel == "Africa") return 7; // SAF
    return 1; // EU
}

Region LatencyMonitor::GetCurrentRegionEnum() {
    std::string regionStr = GetCurrentRegion();
    return StrToRegion(regionStr);
}