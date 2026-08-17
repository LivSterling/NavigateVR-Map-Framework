#include "Registry.h"

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

	std::optional<nvr::FormSpec> ParseFormSpec(const json& value)
	{
		if (!value.is_object()) {
			return std::nullopt;
		}

		const auto plugin = value.value("plugin", std::string{});
		const auto formIt = value.find("formID");
		if (plugin.empty() || formIt == value.end()) {
			return std::nullopt;
		}

		const auto formID = ParseLocalFormID(*formIt);
		if (!formID) {
			return std::nullopt;
		}

		return nvr::FormSpec{ plugin, *formID };
	}

	template <class T>
	T* Resolve(const nvr::FormSpec& spec)
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		return dataHandler ? dataHandler->LookupForm<T>(spec.localID, spec.plugin) : nullptr;
	}
}

namespace nvr
{
	bool Registry::Load(const std::filesystem::path& directory)
	{
		maps_.clear();

		std::error_code error;
		if (!std::filesystem::exists(directory, error)) {
			logger::warn("Map registry directory does not exist: {}", directory.string());
			return true;
		}
		if (error || !std::filesystem::is_directory(directory, error)) {
			logger::error("Map registry path is not a readable directory: {}", directory.string());
			return false;
		}

		std::vector<std::filesystem::path> files;
		for (std::filesystem::directory_iterator it(directory, error), end; it != end; it.increment(error)) {
			if (error) {
				logger::error("Failed while enumerating {}: {}", directory.string(), error.message());
				return false;
			}
			if (it->is_regular_file() && it->path().extension() == ".json") {
				files.push_back(it->path());
			}
		}
		std::ranges::sort(files);

		for (const auto& path : files) {
			try {
				std::ifstream stream(path);
				if (!stream) {
					logger::error("Unable to open map definition: {}", path.string());
					continue;
				}

				const auto root = json::parse(stream);
				if (root.value("schemaVersion", 0) != 1) {
					logger::error("Unsupported or missing schemaVersion in {}", path.string());
					continue;
				}

				const auto mapsIt = root.find("maps");
				if (mapsIt == root.end() || !mapsIt->is_array()) {
					logger::error("Missing maps array in {}", path.string());
					continue;
				}

				for (const auto& value : *mapsIt) {
					const auto id = value.value("id", std::string{});
					const auto worldspace = ParseFormSpec(value.value("worldspace", json{}));
					const auto itemsIt = value.find("items");
					if (id.empty() || !worldspace || itemsIt == value.end() || !itemsIt->is_object()) {
						logger::error("Invalid map entry in {}", path.string());
						continue;
					}

					const auto left = ParseFormSpec(itemsIt->value("left", json{}));
					const auto right = ParseFormSpec(itemsIt->value("right", json{}));
					if (!left || !right) {
						logger::error("Map {} has invalid left/right item forms", id);
						continue;
					}

					MapDefinition definition;
					definition.id = id;
					definition.source = path;
					definition.worldspaceSpec = *worldspace;
					definition.leftMapSpec = *left;
					definition.rightMapSpec = *right;

					if (const auto selection = value.find("selection");
						selection != value.end() && selection->is_object()) {
						definition.enabled = selection->value("enabled", true);
						definition.priority = selection->value("priority", 0);
						definition.useForInteriors = selection->value("useForInteriors", true);
					}

					if (const auto ownership = value.find("ownership");
						ownership != value.end() && ownership->is_object()) {
						definition.ownershipRequired = ownership->value("required", false);
						if (const auto item = ParseFormSpec(ownership->value("item", json{})); item) {
							definition.ownershipItemSpec = *item;
						}
					}

					if (!definition.enabled) {
						continue;
					}

					definition.worldspace = Resolve<RE::TESWorldSpace>(definition.worldspaceSpec);
					definition.leftMap = Resolve<RE::TESObjectARMO>(definition.leftMapSpec);
					definition.rightMap = Resolve<RE::TESObjectARMO>(definition.rightMapSpec);
					if (!definition.worldspace || !definition.leftMap || !definition.rightMap) {
						logger::warn(
							"Skipping unresolved map {} from {}",
							definition.id,
							path.filename().string());
						continue;
					}

					if (definition.ownershipRequired) {
						if (!definition.ownershipItemSpec) {
							logger::warn(
								"Skipping map {}: ownership is required but no valid item was declared",
								definition.id);
							continue;
						}

						definition.ownershipItem =
							Resolve<RE::TESObjectMISC>(*definition.ownershipItemSpec);
						if (!definition.ownershipItem) {
							logger::warn(
								"Skipping map {}: required ownership item could not be resolved",
								definition.id);
							continue;
						}
					}

					maps_.push_back(std::move(definition));
				}
			} catch (const std::exception& exception) {
				logger::error("Failed to parse {}: {}", path.string(), exception.what());
			}
		}

		std::ranges::sort(maps_, [](const auto& left, const auto& right) {
			if (left.worldspaceSpec.plugin != right.worldspaceSpec.plugin) {
				return left.worldspaceSpec.plugin < right.worldspaceSpec.plugin;
			}
			if (left.worldspaceSpec.localID != right.worldspaceSpec.localID) {
				return left.worldspaceSpec.localID < right.worldspaceSpec.localID;
			}
			if (left.priority != right.priority) {
				return left.priority > right.priority;
			}
			if (left.source != right.source) {
				return left.source.generic_string() < right.source.generic_string();
			}
			return left.id < right.id;
		});

		logger::info("Loaded {} resolved NavigateVR map definitions from {}", maps_.size(), directory.string());
		return true;
	}

	const MapDefinition* Registry::Find(const RE::TESWorldSpace* worldspace) const
	{
		if (!worldspace) {
			return nullptr;
		}

		const auto match = std::ranges::find_if(
			maps_,
			[worldspace](const auto& map) { return map.worldspace == worldspace; });
		return match == maps_.end() ? nullptr : std::addressof(*match);
	}

	std::size_t Registry::Size() const noexcept
	{
		return maps_.size();
	}
}
