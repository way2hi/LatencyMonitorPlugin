#pragma once

#include "pch.h"
#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class LatencyMonitor2 : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
public:

    std::shared_ptr<bool> pluginActive = std::make_shared<bool>(true);
    std::shared_ptr<int> maxAllowedLatency = std::make_shared<int>(60);
	std::shared_ptr<std::string> mmRegion = std::make_shared<std::string>("EU");

    void InitializeWinsock();
    void onLoad() override;
	int pluginActiveCheck();
    void onUnload() override;
    void onQueue();
    void CheckLatencyAndCancelQueue();
    int GetPingFromServerArray(std::string* a, int size);
    int GetPing(const std::string& adr);
    void CancelQueue();
    void NotifyUser(std::string message);
    std::string GetCurrentRegion();
    void SetCurrentRegion(std::string region);
    Region StrToRegion(std::string regionLabel);
    std::string RegionToStr(Region region);
    int StrToRegionInt(std::string regionLabel);
    Region GetCurrentRegionEnum();

    // Interface
    void RenderSettings() override;
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;
};