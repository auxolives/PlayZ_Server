// Dig-out Discord/registry must hook ActionContinuousBase.OnFinishProgress, not
// modded ActionDigOutStash.OnFinishProgressServer. PlayZTerjeSkills (-mod) loads
// after PlayZLogs (-servermod) and replaces dig-out server finish without super,
// so a PlayZLogs ActionDigOutStash wrapper never runs.
modded class ActionContinuousBase
{
	override void OnFinishProgress(ActionData action_data)
	{
		if (GetGame().IsServer())
		{
			ActionDigOutStash digOutAction;
			if (Class.CastTo(digOutAction, this))
			{
				PlayZStashDigDiscord.HandleDigOutFinished(action_data);
			}
		}

		super.OnFinishProgress(action_data);
	}
}
