Scriptname NVRDG_TIF_PurchaseMap extends TopicInfo Hidden

MiscObject Property MapItem Auto
MiscObject Property Gold001 Auto
Int Property Price = 100 Auto

Function Fragment_0(ObjectReference akSpeakerRef)
	Actor player = Game.GetPlayer()

	If player.GetItemCount(MapItem) > 0
		Return
	EndIf

	If player.GetItemCount(Gold001) < Price
		Debug.Notification("You do not have enough gold.")
		Return
	EndIf

	player.RemoveItem(Gold001, Price)
	player.AddItem(MapItem, 1)
	Debug.Notification("Map of Dayspring Canyon acquired.")
EndFunction
