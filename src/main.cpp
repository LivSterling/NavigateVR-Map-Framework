#include "pch.h"
#include "EventSinks.h"
#include "Registry.h"

namespace
{
	nvr::Registry registry;
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
			const auto registryPath =
				std::filesystem::path(std::wstring(runtimePath)).parent_path() /
				"Data" / "SKSE" / "Plugins" / "NavigateVRMaps";
			registry.Load(registryPath);
			equipEventSink.Initialize(registry);
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	InitializeLog();
	SKSE::Init(skse);

	logger::info("NavigateVR Map Framework 0.3.1 loading.");

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnMessage)) {
		logger::critical("Failed to register the SKSE messaging listener.");
		return false;
	}

	return true;
}
