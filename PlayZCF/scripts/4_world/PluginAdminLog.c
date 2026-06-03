//! Adds PlayZGameLabsReportItemInteractFromActionEnd; non-continuous actions do not use vanilla OnContinouousAction.
//! Source Found: CF/game-plugin-dayz/Scripts/4_World/4_Hooking/!PluginAdminLog.c (ItemInteract + IsMonitoredAction)
//! Omitted when GAMELABS is not defined (avoids GameLabs API types on non-GameLabs builds).
#ifdef GAMELABS
modded class PluginAdminLog
{
	void PlayZGameLabsReportItemInteractFromActionEnd(ActionData action_data)
	{
		if (!action_data)
			return;
		if (!GetGame().IsServer())
			return;
		if (!GetGameLabs())
			return;
		if (!GetGameLabs().IsStatReportingEnabled())
			return;

		int st = action_data.m_State;
		if (st == UA_CANCEL)
			return;
		if (st == UA_INTERRUPT)
			return;
		if (st == UA_FAILED)
			return;

		if (!action_data.m_Action)
			return;
		if (action_data.m_Action.IsInherited(ActionContinuousBase))
			return;

		string actionName = action_data.m_Action.Type().ToString();
		if (!GetGameLabs().IsMonitoredAction(actionName))
			return;

		string item = "";
		string target = "";
		if (action_data.m_Target)
		{
			Object targetObject;
			Class.CastTo(targetObject, action_data.m_Target.GetObject());
			if (targetObject)
			{
				target = targetObject.GetType();
			}
			else
			{
				Class.CastTo(targetObject, action_data.m_Target.GetParent());
				if (targetObject)
				{
					target = targetObject.GetType();
				}
			}
		}
		if (action_data.m_MainItem)
		{
			ItemBase itemBase = ItemBase.Cast(action_data.m_MainItem);
			if (itemBase)
			{
				item = itemBase.GetType();
			}
		}

		PlayerBase player = PlayerBase.Cast(action_data.m_Player);
		if (!player)
			return;

		_LogPlayerEx logObjectPlayer = new _LogPlayerEx(player);
		_Payload_ItemInteract payload = new _Payload_ItemInteract(logObjectPlayer, item, target, actionName);
		GetGameLabs().GetApi().ItemInteract(new _Callback(), payload);
	}
}
#endif
