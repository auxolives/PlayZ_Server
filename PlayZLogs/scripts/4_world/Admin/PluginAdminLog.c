modded class PluginAdminLog
{
	override void PlayerTeleportedLog(notnull PlayerBase player, vector startPos, vector targetPos, string reason)
	{
		super.PlayerTeleportedLog(player, startPos, targetPos, reason);
		
		if (GetGame().IsServer())
		{
			PlayZLogData_Admin data = new PlayZLogData_Admin();
			data.InitAdmin(player, startPos, targetPos, reason);
			PlayZLogger.LogAdmin("PlayerTeleported", data);
		}
	}
}
