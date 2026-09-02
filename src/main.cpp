#include "pch.h"
#include "EventSinks.h"
#include "Registry.h"
#include "Settings.h"

namespace
{
	nvr::Registry registry;
	nvr::ControllerSettings controllerSettings;
	nvr::EquipEventSink equipEventSink;

	void InitializeLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Unable to resolve the SKSE log directory."sv);
		}

		*path /= "NavigateVRMapFramework.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
	}

	void OnMessage(SKSE::MessagingInterface::Message* message)
	{
		if (!message) {
			return;
		}

		if (message->type == SKSE::MessagingInterface::kDataLoaded) {
			const auto runtimePath = REL::Module::get().filePath();
			const auto dataPath =
				std::filesystem::path(std::wstring(runtimePath)).parent_path() / "Data";
			const auto registryPath = dataPath / "SKSE" / "Plugins" / "NavigateVRMaps";
			const auto settingsPath =
				dataPath / "SKSE" / "Plugins" / "NavigateVRMapFramework.json";
			if (!controllerSettings.Load(settingsPath)) {
				logger::error("Framework settings are invalid; map selection is disabled.");
				return;
			}
			registry.Load(registryPath);
			equipEventSink.Initialize(registry, controllerSettings);
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	InitializeLog();
	SKSE::Init(skse);

	logger::info("NavigateVR Map Framework 0.4.0 loading.");

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnMessage)) {
		logger::critical("Failed to register the SKSE messaging listener.");
		return false;
	}

	return true;
}
