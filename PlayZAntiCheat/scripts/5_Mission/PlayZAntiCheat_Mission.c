modded class MissionServer
{
	protected ref PlayZAntiCheatPlayerMonitor m_PlayZACMonitor;

	override void OnInit()
	{
		super.OnInit();

		PlayZAntiCheatPlayerMonitor.SetInstance(null);

		if (GetGame().IsServer() && (PlayZAntiCheatConfig.Get().EnableCameraChecks || PlayZAntiCheatConfig.Get().EnableSpeedChecks || PlayZAntiCheatConfig.Get().EnableTeleportChecks))
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

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		PlayZAntiCheatProcessPendingKicks();
	}

	protected void PlayZAntiCheatProcessPendingKicks()
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		string uid;
		while (PlayZAntiCheatKickQueue.Dequeue(uid))
		{
			PlayerBase player = PlayZAntiCheatFindPlayerByUid(uid);
			if (!player)
				continue;

			PlayerIdentity identity = player.GetIdentity();
			if (!identity)
				continue;

			GetGame().SendLogoutTime(player, 0);
			PlayerDisconnected(player, identity, uid);
		}
	}

	protected PlayerBase PlayZAntiCheatFindPlayerByUid(string uid)
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		for (int i = 0; i < players.Count(); i++)
		{
			PlayerBase player = PlayerBase.Cast(players[i]);
			if (!player)
				continue;

			PlayerIdentity identity = player.GetIdentity();
			if (identity && identity.GetId() == uid)
				return player;
		}

		return null;
	}
};
