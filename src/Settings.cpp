#include "Settings.h"

#include <nlohmann/json.hpp>

namespace
{
	using json = nlohmann::json;

	std::optional<RE::FormID> ParseLocalFormID(const json& value)
	{
		if (value.is_number_unsigned()) {
			const auto id = value.get<std::uint64_t>();
			if (id <= (std::numeric_limits<RE::FormID>::max)()) {
				return static_cast<RE::FormID>(id);
			}
			return std::nullopt;
		}

		if (!value.is_string()) {
			return std::nullopt;
		}

		auto text = value.get<std::string>();
		if (text.starts_with("0x") || text.starts_with("0X")) {
			text.erase(0, 2);
		}

		RE::FormID result{ 0 };
		const auto [end, error] =
			std::from_chars(text.data(), text.data() + text.size(), result, 16);
		if (error != std::errc{} || end != text.data() + text.size()) {
			return std::nullopt;
		}
		return result;
	}
}

namespace nvr
{
	bool ControllerSettings::Load(const std::filesystem::path& path)
	{
		*this = ControllerSettings{};

		if (!std::filesystem::exists(path)) {
			logger::error("Framework settings file was not found: {}", path.string());
			return false;
		}

		try {
			std::ifstream stream(path);
			if (!stream) {
				logger::error("Could not open framework settings file: {}", path.string());
				return false;
			}

			json document;
			stream >> document;
			if (!document.is_object()) {
				logger::error("Framework settings root must be a JSON object: {}", path.string());
				return false;
			}

			const auto schemaVersion = document.value("schemaVersion", 0);
			if (schemaVersion != 1) {
				logger::error(
					"Unsupported framework settings schemaVersion {} in {}.",
					schemaVersion,
					path.string());
				return false;
			}

			const auto controllerIt = document.find("navigateVRController");
			if (controllerIt == document.end() || !controllerIt->is_object()) {
				logger::error("navigateVRController must be an object in {}.", path.string());
				return false;
			}

			plugin = controllerIt->value("plugin", std::string{});
			editorID = controllerIt->value("editorID", std::string{});
			if (plugin.empty()) {
				logger::error("navigateVRController.plugin is required in {}.", path.string());
				return false;
			}

			const auto formIt = controllerIt->find("formID");
			if (formIt == controllerIt->end() || formIt->is_null()) {
				localID.reset();
			} else {
				localID = ParseLocalFormID(*formIt);
				if (!localID) {
					logger::error(
						"navigateVRController.formID is not a valid local FormID in {}.",
						path.string());
					return false;
				}
			}

			if (editorID.empty() && !localID) {
				logger::error(
					"navigateVRController requires editorID, formID, or both in {}.",
					path.string());
				return false;
			}

			logger::info(
				"Loaded NavigateVR controller settings from {}: plugin={}, editorID={}, formID={}",
				path.string(),
				plugin,
				editorID.empty() ? "<disabled>" : editorID,
				localID ? std::format("0x{:06X}", *localID) : "<disabled>");
			return true;
		} catch (const std::exception& exception) {
			logger::error("Failed to parse {}: {}", path.string(), exception.what());
			return false;
		}
	}
}
