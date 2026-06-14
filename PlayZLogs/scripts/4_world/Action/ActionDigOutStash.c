modded class ActionDigOutStash
{
	override void OnFinishProgressServer(ActionData action_data)
	{
		PlayZStashDigDiscord.HandleDigOutFinished(action_data);
		super.OnFinishProgressServer(action_data);
	}
}
