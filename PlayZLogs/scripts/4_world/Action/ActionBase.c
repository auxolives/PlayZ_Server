modded class ActionBase
{
	override void Start(ActionData action_data)
	{
		super.Start(action_data);
		
		if (GetGame().IsServer())
		{
			PlayZLogData_Action data = new PlayZLogData_Action();
			data.InitAction(action_data);
			PlayZLogger.LogAction("ActionStart", data);
		}
	}
}
