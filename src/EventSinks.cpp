#include "EventSinks.h"

namespace
{
	constexpr RE::FormID kNavigateVRWorldMapLocalID = 0x037482;
	constexpr std::string_view kNavigateVRPlugin =
		"Navigate VR - Equipable Dynamic Compass and Maps.esp";

	bool IsNavigateVRMapArmor(const RE::TESObjectARMO& armor)
	{
		const auto* file = armor.GetFile(0);
		return file && file->GetFilename() == kNavigateVRPlugin;
	}

	class EquipmentMutationGuard
	{
	public:
		explicit EquipmentMutationGuard(std::atomic_flag& flag) :
			flag_(flag),
			acquired_(!flag_.test_and_set(std::memory_order_acquire))
		{}

		~EquipmentMutationGuard()
		{
			if (acquired_) {
				flag_.clear(std::memory_order_release);
			}
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return acquired_;
		}

	private:
		std::atomic_flag& flag_;
		bool acquired_{ false };
	};
}

namespace nvr
{
	bool EquipEventSink::Initialize(Registry& registry)
	{
		registry_ = std::addressof(registry);

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		controller_ = dataHandler ?
			dataHandler->LookupForm<RE::TESObjectWEAP>(
				kNavigateVRWorldMapLocalID,
				kNavigateVRPlugin) :
			nullptr;
		if (!controller_) {
			logger::error(
				"NavigateVR controller weapon 0x{:06X} could not be resolved from {}",
				kNavigateVRWorldMapLocalID,
				kNavigateVRPlugin);
			return false;
		}

		auto* eventSource = RE::ScriptEventSourceHolder::GetSingleton();
		if (!eventSource) {
			logger::error("ScriptEventSourceHolder is unavailable.");
			return false;
		}

		eventSource->AddEventSink<RE::TESEquipEvent>(this);
		logger::info(
			"Registered NavigateVR equip observer for {} ({:08X}).",
			controller_->GetFormEditorID(),
			controller_->GetFormID());
		return true;
	}

	RE::BSEventNotifyControl EquipEventSink::ProcessEvent(
		const RE::TESEquipEvent* event,
		[[maybe_unused]] RE::BSTEventSource<RE::TESEquipEvent>* source)
	{
		if (!event || !controller_ || !registry_) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || event->actor.get() != player) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (equipmentMutationInProgress_.test(std::memory_order_acquire)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (event->baseObject != controller_->GetFormID()) {
			if (event->equipped && pendingDefinition_) {
				if (auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(event->baseObject);
					armor && IsNavigateVRMapArmor(*armor)) {
					ReplaceNavigateVRMap(*player, *armor);
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* currentWorldspace = player->GetWorldspace();
		if (currentWorldspace) {
			lastExteriorWorldspace_ = currentWorldspace;
		}

		const auto* effectiveWorldspace =
			currentWorldspace ? currentWorldspace : lastExteriorWorldspace_;
		const auto* definition = registry_->Find(effectiveWorldspace);

		const auto rightHand = player->GetEquippedObject(false) == controller_;
		const auto leftHand = player->GetEquippedObject(true) == controller_;
		const std::string_view hand =
			rightHand ? "right"sv : (leftHand ? "left"sv : "unknown"sv);

		if (!event->equipped) {
			pendingDefinition_ = nullptr;
			EquipmentMutationGuard mutationGuard(equipmentMutationInProgress_);
			if (mutationGuard) {
				ClearActiveMap(*player);
			}
			logger::info("NavigateVR controller unequipped; last observed hand={}.", hand);
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!definition) {
			pendingDefinition_ = nullptr;
			logger::info(
				"NavigateVR controller equipped in {} hand; no registered map for worldspace {:08X}.",
				hand,
				effectiveWorldspace ? effectiveWorldspace->GetFormID() : 0);
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!currentWorldspace && !definition->useForInteriors) {
			pendingDefinition_ = nullptr;
			logger::info(
				"Map {} matched cached worldspace but is disabled for interiors.",
				definition->id);
			return RE::BSEventNotifyControl::kContinue;
		}

		if (definition->ownershipRequired &&
			(!definition->ownershipItem || player->GetItemCount(definition->ownershipItem) < 1)) {
			pendingDefinition_ = nullptr;
			logger::info(
				"Map {} matched worldspace {:08X}, but its ownership item is missing.",
				definition->id,
				effectiveWorldspace->GetFormID());
			return RE::BSEventNotifyControl::kContinue;
		}

		pendingDefinition_ = definition;
		pendingLeftHand_ = leftHand;
		logger::info(
			"NavigateVR controller equipped in {} hand; queued map {} for worldspace {:08X}.",
			hand,
			definition->id,
			effectiveWorldspace->GetFormID());
		return RE::BSEventNotifyControl::kContinue;
	}

	void EquipEventSink::ClearActiveMap(RE::PlayerCharacter& player)
	{
		if (!activeMap_) {
			return;
		}

		if (auto* equipManager = RE::ActorEquipManager::GetSingleton()) {
			equipManager->UnequipObject(
				std::addressof(player),
				activeMap_,
				nullptr,
				1,
				nullptr,
				false,
				true,
				false,
				true);
		}

		if (player.GetItemCount(activeMap_) > 0) {
			player.RemoveItem(
				activeMap_,
				1,
				RE::ITEM_REMOVE_REASON::kRemove,
				nullptr,
				nullptr);
		}

		logger::info("Removed active registered map {:08X}.", activeMap_->GetFormID());
		activeMap_ = nullptr;
	}

	void EquipEventSink::EquipRegisteredMap(
		RE::PlayerCharacter& player,
		const MapDefinition& definition,
		const bool leftHand)
	{
		auto* selectedMap = leftHand ? definition.leftMap : definition.rightMap;
		if (!selectedMap) {
			logger::error("Registered map {} has no resolved armor for the active hand.", definition.id);
			return;
		}

		if (activeMap_ == selectedMap) {
			return;
		}
		if (activeMap_) {
			ClearActiveMap(player);
		}

		if (player.GetItemCount(selectedMap) < 1) {
			player.AddObjectToContainer(selectedMap, nullptr, 1, nullptr);
		}

		auto* equipManager = RE::ActorEquipManager::GetSingleton();
		if (!equipManager) {
			logger::error("ActorEquipManager is unavailable.");
			return;
		}

		equipManager->EquipObject(
			std::addressof(player),
			selectedMap,
			nullptr,
			1,
			nullptr,
			false,
			true,
			false,
			true);

		activeMap_ = selectedMap;
		logger::info(
			"Equipped registered map {} ({:08X}) in {} hand.",
			definition.id,
			selectedMap->GetFormID(),
			leftHand ? "left" : "right");
	}

	void EquipEventSink::ReplaceNavigateVRMap(
		RE::PlayerCharacter& player,
		RE::TESObjectARMO& navigateVRMap)
	{
		if (!pendingDefinition_) {
			return;
		}

		const auto controllerStillEquipped =
			player.GetEquippedObject(pendingLeftHand_) == controller_;
		if (!controllerStillEquipped) {
			pendingDefinition_ = nullptr;
			return;
		}

		const auto slot = navigateVRMap.GetSlotMask();
		const auto expectedSlot = pendingLeftHand_ ?
			RE::BGSBipedObjectForm::BipedObjectSlot::kModNeck :
			RE::BGSBipedObjectForm::BipedObjectSlot::kModMouth;
		if (!slot.any(expectedSlot)) {
			return;
		}

		auto* selectedMap = pendingLeftHand_ ?
			pendingDefinition_->leftMap :
			pendingDefinition_->rightMap;
		if (!selectedMap) {
			logger::error("Registered map {} has no resolved armor for the active hand.", pendingDefinition_->id);
			pendingDefinition_ = nullptr;
			return;
		}
		if (selectedMap == std::addressof(navigateVRMap)) {
			logger::info(
				"NavigateVR already equipped registered map {} ({:08X}); no replacement needed.",
				pendingDefinition_->id,
				selectedMap->GetFormID());
			pendingDefinition_ = nullptr;
			return;
		}

		EquipmentMutationGuard mutationGuard(equipmentMutationInProgress_);
		if (!mutationGuard) {
			return;
		}
		EquipRegisteredMap(player, *pendingDefinition_, pendingLeftHand_);
		logger::info(
			"Replaced NavigateVR armor {:08X} with registered map {} ({:08X}).",
			navigateVRMap.GetFormID(),
			pendingDefinition_->id,
			selectedMap->GetFormID());
		pendingDefinition_ = nullptr;
	}

}
