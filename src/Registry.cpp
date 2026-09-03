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

	bool ParseFormSpecArray(
		const json& value,
		std::vector<nvr::FormSpec>& destination)
	{
		if (!value.is_array()) {
			return false;
		}

		for (const auto& entry : value) {
			const auto spec = ParseFormSpec(entry);
			if (!spec) {
				return false;
			}
			destination.push_back(*spec);
		}
		return true;
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
				const auto schemaVersion = root.value("schemaVersion", 0);
				if (schemaVersion != 1 && schemaVersion != 2) {
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

						if (const auto match = selection->find("match"); match != selection->end()) {
							if (schemaVersion < 2 || !match->is_object()) {
								logger::error(
									"Map {} has selection.match but does not use schemaVersion 2",
									id);
								continue;
							}

							definition.hasExplicitMatch = true;
							definition.includeChildLocations =
								match->value("includeChildLocations", true);

							if (const auto locations = match->find("locations");
								locations != match->end() &&
								!ParseFormSpecArray(*locations, definition.matchLocationSpecs)) {
								logger::error("Map {} has invalid location match forms", id);
								continue;
							}

							if (const auto worldspaces = match->find("worldspaces");
								worldspaces != match->end() &&
								!ParseFormSpecArray(*worldspaces, definition.matchWorldspaceSpecs)) {
								logger::error("Map {} has invalid worldspace match forms", id);
								continue;
							}

							if (definition.matchLocationSpecs.empty() &&
								definition.matchWorldspaceSpecs.empty()) {
								logger::error("Map {} declares an empty selection.match", id);
								continue;
							}
						}
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

					if (definition.hasExplicitMatch) {
						for (const auto& spec : definition.matchLocationSpecs) {
							if (auto* location = Resolve<RE::BGSLocation>(spec)) {
								definition.matchLocations.push_back(location);
							} else {
								logger::warn(
									"Map {} ignored unresolved optional location {}:{:06X}",
									definition.id,
									spec.plugin,
									spec.localID);
							}
						}
						for (const auto& spec : definition.matchWorldspaceSpecs) {
							if (auto* matchWorldspace = Resolve<RE::TESWorldSpace>(spec)) {
								definition.matchWorldspaces.push_back(matchWorldspace);
							} else {
								logger::warn(
									"Map {} ignored unresolved optional worldspace {}:{:06X}",
									definition.id,
									spec.plugin,
									spec.localID);
							}
						}

						if (definition.matchLocations.empty() &&
							definition.matchWorldspaces.empty()) {
							logger::warn(
								"Skipping map {}: none of its selection.match forms could be resolved",
								definition.id);
							continue;
						}
					} else {
						definition.matchWorldspaces.push_back(definition.worldspace);
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

	const MapDefinition* Registry::Find(
		const RE::TESWorldSpace* worldspace,
		const RE::BGSLocation* location) const
	{
		const MapDefinition* best{ nullptr };
		std::uint8_t bestSpecificity{ 0 };

		for (const auto& map : maps_) {
			std::uint8_t specificity{ 0 };

			if (location) {
				for (const auto* registeredLocation : map.matchLocations) {
					if (registeredLocation == location) {
						specificity = 3;
						break;
					}
					if (map.includeChildLocations && registeredLocation->IsChild(location)) {
						specificity = (std::max)(specificity, static_cast<std::uint8_t>(2));
					}
				}
			}

			if (specificity == 0 && worldspace &&
				std::ranges::find(map.matchWorldspaces, worldspace) != map.matchWorldspaces.end()) {
				specificity = 1;
			}

			if (specificity == 0) {
				continue;
			}

			if (!best || specificity > bestSpecificity ||
				(specificity == bestSpecificity && map.priority > best->priority)) {
				best = std::addressof(map);
				bestSpecificity = specificity;
			}
		}

		return best;
	}

	std::size_t Registry::Size() const noexcept
	{
		return maps_.size();
	}
}
