Scriptname NVRDG_TIF_GrantMap extends TopicInfo Hidden

MiscObject Property MapItem Auto
String Property AcquisitionMessage Auto

Function Fragment_0(ObjectReference akSpeakerRef)
	Actor player = Game.GetPlayer()

	If player.GetItemCount(MapItem) > 0
		Return
	EndIf

	player.AddItem(MapItem, 1)
	Debug.Notification(AcquisitionMessage)
EndFunction
