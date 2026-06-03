//! Bridges non-continuous actions to GameLabs ItemInteract (continuous actions already use PluginAdminLog.OnContinouousAction).
//! Source Found: scripts/4_World/Classes/UserActionsComponent/ActionManagerServer.c:184
//! Omitted when GAMELABS is not defined (client / non-GameLabs script builds).
#ifdef GAMELABS
modded class ActionManagerServer
{
	override void OnActionEnd()
	{
		if (m_CurrentActionData)
		{
			m_CurrentActionData.m_Action.ClearActionJuncture(m_CurrentActionData);

			PluginAdminLog adminLog = PluginAdminLog.Cast(GetPlugin(PluginAdminLog));
			if (adminLog)
			{
				adminLog.PlayZGameLabsReportItemInteractFromActionEnd(m_CurrentActionData);
			}

			super.OnActionEnd();
		}
	}
}
#endif
