#pragma once

namespace nvr
{
	struct FormSpec
	{
		std::string plugin;
		RE::FormID localID{ 0 };
	};

	struct MapDefinition
	{
		std::string id;
		std::filesystem::path source;
		FormSpec worldspaceSpec;
		FormSpec leftMapSpec;
		FormSpec rightMapSpec;
		std::optional<FormSpec> ownershipItemSpec;
		std::int32_t priority{ 0 };
		bool enabled{ true };
		bool useForInteriors{ true };
		bool ownershipRequired{ false };
		bool hasExplicitMatch{ false };
		bool includeChildLocations{ true };
		std::vector<FormSpec> matchLocationSpecs;
		std::vector<FormSpec> matchWorldspaceSpecs;

		RE::TESWorldSpace* worldspace{ nullptr };
		std::vector<RE::BGSLocation*> matchLocations;
		std::vector<RE::TESWorldSpace*> matchWorldspaces;
		RE::TESObjectARMO* leftMap{ nullptr };
		RE::TESObjectARMO* rightMap{ nullptr };
		RE::TESObjectMISC* ownershipItem{ nullptr };
	};

	class Registry
	{
	public:
		bool Load(const std::filesystem::path& directory);

		[[nodiscard]] const MapDefinition* Find(
			const RE::TESWorldSpace* worldspace,
			const RE::BGSLocation* location) const;

		[[nodiscard]] std::size_t Size() const noexcept;

	private:
		std::vector<MapDefinition> maps_;
	};
}
