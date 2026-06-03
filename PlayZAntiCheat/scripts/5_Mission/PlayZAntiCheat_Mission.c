modded class MissionServer
{
	protected ref PlayZAntiCheatPlayerMonitor m_PlayZACMonitor;

	override void OnInit()
	{
		super.OnInit();

		PlayZAntiCheatPlayerMonitor.SetInstance(null);

		if (GetGame().IsServer() && PlayZAntiCheatConfig.Get().EnableCameraChecks)
		{
			m_PlayZACMonitor = new PlayZAntiCheatPlayerMonitor();
		}
	}

	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		if (m_PlayZACMonitor && player)
			m_PlayZACMonitor.OnPlayerConnected(player);
	}

	override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientRespawnEvent(identity, player);
		if (m_PlayZACMonitor && player)
			m_PlayZACMonitor.OnPlayerConnected(player);
	}

	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
		if (m_PlayZACMonitor && identity)
			m_PlayZACMonitor.OnPlayerDisconnected(identity);

		super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
	}

	PlayZAntiCheatPlayerMonitor GetPlayZAntiCheatMonitor()
	{
		return m_PlayZACMonitor;
	}
};
