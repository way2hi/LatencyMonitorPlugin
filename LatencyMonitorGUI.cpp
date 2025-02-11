#include "pch.h"
#include "LatencyMonitor.h"

void LatencyMonitor2::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

std::string LatencyMonitor2::GetPluginName()
{
	return "Latency Monitor";
}

void LatencyMonitor2::RenderSettings() {
	static auto pluginCvar = cvarManager->getCvar("latency_monitor2_enabled");
	auto pluginEnabled = pluginCvar.getBoolValue();
	if (ImGui::Checkbox("Enable plugin", &pluginEnabled)) { pluginCvar.setValue(pluginEnabled); }
	ImGui::Separator();
	static auto maxAllowedLatency = cvarManager->getCvar("latency_monitor2_max_allowed_latency");
	auto latency = maxAllowedLatency.getIntValue();
	if (ImGui::SliderInt("Max allowed latency", &latency, 0, 999)) { maxAllowedLatency.setValue(latency); }
	static auto mmRegion = cvarManager->getCvar("latency_monitor2_region");
	int currentRegion = StrToRegionInt(GetCurrentRegion());
	const char* regions[] = { "US-East", "US-West", "EU", "Oceania", "Asia-Mainland" };
	if (ImGui::Combo("Region", &currentRegion, regions, IM_ARRAYSIZE(regions))) { mmRegion.setValue(regions[currentRegion]); }
}