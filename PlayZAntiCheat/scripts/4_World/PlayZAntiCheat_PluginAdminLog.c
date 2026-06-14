modded class PluginAdminLog
{
	override void PlayerTeleportedLog(notnull PlayerBase player, vector startPos, vector targetPos, string reason)
	{
		super.PlayerTeleportedLog(player, startPos, targetPos, reason);

		if (!GetGame() || !GetGame().IsServer())
			return;

		PlayZAntiCheatPlayerMonitor monitor = PlayZAntiCheatPlayerMonitor.GetInstance();
		if (monitor)
			monitor.OnEngineTeleport(player);
	}
}
