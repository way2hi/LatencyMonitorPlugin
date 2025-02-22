#include "pch.h"
#include "LatencyMonitor.h"

void LatencyMonitor::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

std::string LatencyMonitor::GetPluginName()
{
	return "Latency Monitor";
}

void LatencyMonitor::RenderSettings() {

	static auto pluginCvar = cvarManager->getCvar("lmplugin_enabled");
	auto pluginEnabled = pluginCvar.getBoolValue();
	if (ImGui::Checkbox("Enable plugin", &pluginEnabled))
	{
		pluginCvar.setValue(pluginEnabled);
	}

	static auto maxAllowedLatency = cvarManager->getCvar("lmplugin_max_allowed_latency");
	auto latency = maxAllowedLatency.getIntValue();
	if (ImGui::SliderInt("Max allowed latency", &latency, 0, 999))
	{
		maxAllowedLatency.setValue(latency);
	}

	static auto mmRegion = cvarManager->getCvar("lmplugin_region");
	int currentRegion = StrToRegionInt(GetCurrentRegion());
	const char* regions[] = { "US-East", "US-West", "EU", "Oceania", "Asia-Mainland" };
	if (ImGui::Combo("Region", &currentRegion, regions, IM_ARRAYSIZE(regions)))
	{
		mmRegion.setValue(regions[currentRegion]);
	}

	static auto debugCvar = cvarManager->getCvar("lmplugin_debug");
	auto debugMode = debugCvar.getBoolValue();
	if (ImGui::Checkbox("Debug mode", &debugMode))
	{
		debugCvar.setValue(debugMode);
	}
}