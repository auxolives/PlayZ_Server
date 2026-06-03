class PlayZAntiCheatPlayerState
{
	vector LastPlayerPosition;     // server-side snapshot at time of last response
	vector LastCameraPosition;     // client-side camera at time of last response
	vector LastClientPlayerPos;    // client-side player pos at time of last response
	float LastRequestTime;
	float LastResponseTime;
	float LastAlertTime;
	float NextCheckTime;
	float SpawnGraceUntil;         // server time until which samples are ignored
	int   ConsecutiveAnomalies;    // N-in-a-row anomalies before alert fires
	bool  PendingCameraResponse;
};

class PlayZAntiCheatPlayerMonitor
{
	protected static ref PlayZAntiCheatPlayerMonitor s_Instance;
	protected ref map<string, ref PlayZAntiCheatPlayerState> m_PlayerStates;
	protected ref array<Man> m_PlayersBuffer;
	protected bool m_IsScheduled;

	void PlayZAntiCheatPlayerMonitor()
	{
		SetInstance(this);
		m_PlayerStates = new map<string, ref PlayZAntiCheatPlayerState>();
		m_PlayersBuffer = new array<Man>();
		Schedule();
	}

	static PlayZAntiCheatPlayerMonitor GetInstance()
	{
		return s_Instance;
	}

	static void SetInstance(PlayZAntiCheatPlayerMonitor instance)
	{
		s_Instance = instance;
	}

	protected void Schedule()
	{
		if (m_IsScheduled || !GetGame())
			return;

		m_IsScheduled = true;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestSamples, 500, true);
	}

	void OnPlayerConnected(PlayerBase player)
	{
		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		// Grace window while the client finishes loading and the camera settles.
		int graceSeconds = Math.Max(PlayZAntiCheatConfig.Get().CameraRespawnGraceSeconds, 0);
		state.SpawnGraceUntil = GetGame().GetTime() + graceSeconds * 1000;
		state.ConsecutiveAnomalies = 0;
		state.PendingCameraResponse = false;
	}

	void OnPlayerDeath(PlayerBase player)
	{
		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		// Re-apply grace when death transitions camera ownership/state.
		int graceSeconds = Math.Max(PlayZAntiCheatConfig.Get().CameraRespawnGraceSeconds, 0);
		state.SpawnGraceUntil = GetGame().GetTime() + graceSeconds * 1000;
		state.ConsecutiveAnomalies = 0;
		state.PendingCameraResponse = false;
	}

	void OnPlayerDisconnected(PlayerIdentity identity)
	{
		if (!identity)
			return;

		string uid = identity.GetId();
		if (uid != "" && m_PlayerStates.Contains(uid))
			m_PlayerStates.Remove(uid);
	}

	protected PlayZAntiCheatPlayerState EnsureState(PlayerBase player)
	{
		if (!player)
			return null;

		PlayZAntiCheatPlayerState state = player.GetPlayZACState();
		if (state)
			return state;

		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return null;

		string uid = identity.GetId();
		if (uid == "")
			return null;

		if (!m_PlayerStates.Find(uid, state))
		{
			state = new PlayZAntiCheatPlayerState();

			int interval = Math.Max(PlayZAntiCheatConfig.Get().CameraSampleIntervalSeconds, 1) * 1000;
			state.NextCheckTime = GetGame().GetTime() + Math.RandomInt(0, interval);

			m_PlayerStates.Set(uid, state);
		}

		player.SetPlayZACState(state);
		return state;
	}

	protected bool IsSpawnMenuSentinelPosition(vector position)
	{
		float epsilon = 0.01;
		if (Math.AbsFloat(position[0]) > epsilon)
			return false;
		if (Math.AbsFloat(position[1] - 1000.0) > epsilon)
			return false;
		if (Math.AbsFloat(position[2]) > epsilon)
			return false;
		return true;
	}

	// Player states where the camera legitimately separates from the body origin
	// (vehicle cockpit pivot, swim pose, restrain anim, etc.). Sampling here
	// produces noise, not signal, so we skip instead of risk a false alert.
	protected bool IsPlayerInCameraSafeState(PlayerBase player)
	{
		if (!player)
			return false;
		if (!player.IsAlive())
			return true;
		if (player.IsUnconscious())
			return true;
		if (player.IsInVehicle())
			return true;
		if (player.IsRestrained())
			return true;
		if (player.IsSwimming())
			return true;
		return false;
	}

	void RequestSamples()
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		if (!cfg.EnableCameraChecks)
			return;

		m_PlayersBuffer.Clear();
		GetGame().GetPlayers(m_PlayersBuffer);

		float now = GetGame().GetTime();
		int interval = Math.Max(cfg.CameraSampleIntervalSeconds, 1) * 1000;
		int responseTimeoutMs = Math.Max(cfg.CameraResponseTimeoutSeconds, 1) * 1000;
		int playerCount = m_PlayersBuffer.Count();

		for (int i = 0; i < playerCount; i++)
		{
			PlayerBase player = PlayerBase.Cast(m_PlayersBuffer.Get(i));
			if (!player)
				continue;

			PlayerIdentity identity = player.GetIdentity();
			if (!identity)
				continue;

			PlayZAntiCheatPlayerState state = EnsureState(player);
			if (!state)
				continue;

			if (state.PendingCameraResponse && state.LastRequestTime > 0 && now - state.LastRequestTime >= responseTimeoutMs)
			{
				// Timed-out request is NOT an anomaly — common under lag/disconnect.
				// Reset the pending flag and schedule a small retry delay.
				state.PendingCameraResponse = false;
				state.NextCheckTime = now + 1000;
			}

			if (IsPlayerInCameraSafeState(player))
			{
				state.PendingCameraResponse = false;
				state.NextCheckTime = now + interval;
				continue;
			}

			vector playerPos = player.GetPosition();
			if (IsSpawnMenuSentinelPosition(playerPos))
			{
				state.PendingCameraResponse = false;
				state.NextCheckTime = now + interval;
				continue;
			}

			if (state.SpawnGraceUntil > now)
				continue;

			if (state.NextCheckTime > now)
				continue;

			// Stagger the next check by adding a small jitter (+/- 10% of interval, capped at 2s)
			int jitter = Math.RandomInt(-(interval * 0.1), (interval * 0.1));
			jitter = Math.Clamp(jitter, -2000, 2000);

			state.NextCheckTime = now + interval + jitter;
			state.LastRequestTime = now;
			state.PendingCameraResponse = true;

			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(now);
			rpc.Send(player, RPC_PLAYZ_AC_CAMERA_REQUEST, true, identity);
		}
	}

	// cameraPos and clientPlayerPos are both client-side snapshots taken in the
	// SAME frame -> their delta is immune to RTT. We use that delta for the
	// anomaly verdict. serverPlayerPos (captured here, now) is used only as a
	// sanity bound: if the client-reported position disagrees with the server
	// by more than CameraMaxServerDriftMeters, we ignore the sample rather than
	// trust it and risk either false-positive (lag) or false-negative (spoof).
	//
	// flags is a bitfield (PLAYZ_AC_CAMERA_FLAG_*) populated client-side:
	//   PLAYZ_AC_CAMERA_FLAG_DETACHED -> a non-player Camera instance is
	//   rendering (admin freecam, spectator, cinematic, FreeDebugCamera,
	//   character-creation / intro). Per engine contract in Camera.c the
	//   player's own DayZPlayerCamera is NOT a Camera instance, so this
	//   flag is an authoritative "not a cheat signal, skip the sample".
	void OnCameraResponse(PlayerBase player, vector cameraPos, vector clientPlayerPos, float clientTime, int flags = 0)
	{
		if (!player)
			return;

		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		if (!cfg.EnableCameraChecks)
			return;

		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		float now = GetGame().GetTime();
		vector serverPlayerPos = player.GetPosition();
		float clientSeparation = vector.Distance(clientPlayerPos, cameraPos);
		float serverClientDrift = vector.Distance(clientPlayerPos, serverPlayerPos);

		state.LastPlayerPosition = serverPlayerPos;
		state.LastClientPlayerPos = clientPlayerPos;
		state.LastCameraPosition = cameraPos;
		state.LastResponseTime = now;
		state.PendingCameraResponse = false;

		bool isDetachedCamera = (flags & PLAYZ_AC_CAMERA_FLAG_DETACHED) != 0;

		if (state.SpawnGraceUntil > now)
			return;

		if (IsPlayerInCameraSafeState(player))
			return;

		if (IsSpawnMenuSentinelPosition(serverPlayerPos))
			return;

		if (IsSpawnMenuSentinelPosition(clientPlayerPos))
			return;

		// Absurdly large deltas are always a transition artifact (loading
		// screens, intro scenes at <-146, 479, 46>, map-edge teleport bugs)
		// rather than a cheat signal worth alerting on. Skip silently and
		// do NOT bump ConsecutiveAnomalies — otherwise a single huge glitch
		// stays in the counter and primes the next normal sample to alert.
		if (!isDetachedCamera && cfg.CameraMaxPlausibleSeparation > 0 && clientSeparation > cfg.CameraMaxPlausibleSeparation)
		{
			state.ConsecutiveAnomalies = 0;
			return;
		}

		// Untrustworthy sample: client's self-reported body is far from where
		// the server thinks the body is. Could be packet loss, teleport, or
		// a spoofed position. Either way, don't alert on it.
		if (cfg.CameraMaxServerDriftMeters > 0 && serverClientDrift > cfg.CameraMaxServerDriftMeters)
		{
			state.ConsecutiveAnomalies = 0;
			return;
		}

		if (clientSeparation <= cfg.CameraMaxSeparation)
		{
			state.ConsecutiveAnomalies = 0;
			return;
		}

		state.ConsecutiveAnomalies = state.ConsecutiveAnomalies + 1;

		int required = Math.Max(cfg.CameraConsecutiveAnomaliesToAlert, 1);
		if (state.ConsecutiveAnomalies < required)
			return;

		if (state.LastAlertTime > 0 && now - state.LastAlertTime < cfg.CameraSampleIntervalSeconds * 1000)
			return;

		state.LastAlertTime = now;
		state.ConsecutiveAnomalies = 0;

		string playerName = PlayZAntiCheatUtils.GetIdentityName(player);
		string playerId = PlayZAntiCheatUtils.GetIdentityId(player);
		string separationText = PlayZAntiCheatUtils.FormatFloat(clientSeparation, 2);
		string driftText = PlayZAntiCheatUtils.FormatFloat(serverClientDrift, 2);
		string logLine = string.Format("Camera anomaly: %1 (%2) | separation=%3 m (client-coherent) | serverDrift=%4 m | clientPlayerPos=%5 | serverPlayerPos=%6 | cameraPos=%7", playerName, playerId, separationText, driftText, PlayZAntiCheatUtils.FormatVector(clientPlayerPos, 2), PlayZAntiCheatUtils.FormatVector(serverPlayerPos, 2), PlayZAntiCheatUtils.FormatVector(cameraPos, 2));
		PlayZAntiCheatLogger.Info(logLine);

		string webhook = cfg.DiscordCameraWebhookUrl;
		if (webhook == "")
			webhook = cfg.DiscordWebhookUrl;

		if (webhook != "")
		{
			ref array<ref PlayZDiscordEmbedField> fields = new array<ref PlayZDiscordEmbedField>();
			PlayZAntiCheatUtils.AddEmbedField(fields, "Separation (client-coherent)", separationText + " m");
			PlayZAntiCheatUtils.AddEmbedField(fields, "Server vs client drift", driftText + " m");
			if (isDetachedCamera)
				PlayZAntiCheatUtils.AddEmbedField(fields, "Detached camera", "yes");
			PlayZAntiCheatUtils.AddEmbedField(fields, "Client player pos", PlayZAntiCheatUtils.FormatVector(clientPlayerPos, 2));
			PlayZAntiCheatUtils.AddEmbedField(fields, "Server player pos", PlayZAntiCheatUtils.FormatVector(serverPlayerPos, 2));
			PlayZAntiCheatUtils.AddEmbedField(fields, "Camera pos", PlayZAntiCheatUtils.FormatVector(cameraPos, 2));

			string description = string.Format("Player: %1 (%2)", playerName, playerId);
			string payload = PlayZAntiCheatUtils.BuildWebhookEmbed("PlayZ AntiCheat", ":rotating_light: CAMERA ANOMALY!", description, 0xE74C3C, fields);
			if (payload == "")
				return;
			PlayZAntiCheatWebhookDispatcher.Get().Enqueue(webhook, payload);
		}
	}
};
