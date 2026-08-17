#pragma once

#include "Registry.h"

namespace nvr
{
	class EquipEventSink final : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		bool Initialize(Registry& registry);

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESEquipEvent* event,
			RE::BSTEventSource<RE::TESEquipEvent>* source) override;

	private:
		void ClearActiveMap(RE::PlayerCharacter& player);
		void EquipRegisteredMap(
			RE::PlayerCharacter& player,
			const MapDefinition& definition,
			bool leftHand);
		void ReplaceNavigateVRMap(
			RE::PlayerCharacter& player,
			RE::TESObjectARMO& navigateVRMap);

		Registry* registry_{ nullptr };
		RE::TESObjectWEAP* controller_{ nullptr };
		RE::TESWorldSpace* lastExteriorWorldspace_{ nullptr };
		const MapDefinition* pendingDefinition_{ nullptr };
		RE::TESObjectARMO* activeMap_{ nullptr };
		bool pendingLeftHand_{ false };
		std::atomic_flag equipmentMutationInProgress_ = ATOMIC_FLAG_INIT;
	};
}
