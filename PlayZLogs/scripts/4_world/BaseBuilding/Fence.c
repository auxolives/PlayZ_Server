modded class Fence
{
	override void OpenFence()
	{
		super.OpenFence();
		
		if (GetGame().IsServer())
		{
			PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
			logData.InitBaseBuilding(this, "Fence", null);
			PlayZLogger.LogBaseBuilding("Opened", logData);
		}
	}
	
	override void CloseFence()
	{
		super.CloseFence();
		
		if (GetGame().IsServer())
		{
			PlayZLogData_BaseBuilding logData = new PlayZLogData_BaseBuilding();
			logData.InitBaseBuilding(this, "Fence", null);
			PlayZLogger.LogBaseBuilding("Closed", logData);
		}
	}
}
