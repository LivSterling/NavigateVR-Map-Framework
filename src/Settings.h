#pragma once

namespace nvr
{
	struct ControllerSettings
	{
		std::string plugin;
		std::string editorID;
		std::optional<RE::FormID> localID;

		bool Load(const std::filesystem::path& path);
	};
}
