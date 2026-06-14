modded class ActionDigInStash
{
	override void OnFinishProgressServer(ActionData action_data)
	{
		super.OnFinishProgressServer(action_data);
		PlayZStashDigDiscord.HandleDigInFinished(action_data);
	}
}
