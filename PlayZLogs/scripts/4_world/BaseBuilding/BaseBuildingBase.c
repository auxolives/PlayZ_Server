modded class BaseBuildingBase
{
	override void OnPartBuiltServer(notnull Man player, string part_name, int action_id)
	{
		super.OnPartBuiltServer(player, part_name, action_id);
		
		PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
		logData.InitBaseBuilding(this, part_name, player);
		PlayZLogger.LogBaseBuilding("Built", logData);
	}
	
	override void OnPartDismantledServer(notnull Man player, string part_name, int action_id)
	{
		super.OnPartDismantledServer(player, part_name, action_id);
		
		PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
		logData.InitBaseBuilding(this, part_name, player);
		PlayZLogger.LogBaseBuilding("Dismantled", logData);
	}
	
	override void OnPartDestroyedServer(Man player, string part_name, int action_id, bool destroyed_by_connected_part = false)
	{
		super.OnPartDestroyedServer(player, part_name, action_id, destroyed_by_connected_part);
		
		PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
		logData.InitBaseBuilding(this, part_name, player);
		PlayZLogger.LogBaseBuilding("Destroyed", logData);
	}
	
	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);
		
		PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
		logData.InitBaseBuilding(this, slot_name, null);
		PlayZLogger.LogBaseBuilding("Attached", logData);
	}
	
	override void EEItemDetached(EntityAI item, string slot_name)
	{
		super.EEItemDetached(item, slot_name);
		
		PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
		logData.InitBaseBuilding(this, slot_name, null);
		PlayZLogger.LogBaseBuilding("Detached", logData);
	}
}
