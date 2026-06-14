class PlayZAntiCheatPlayerState
{
	vector LastPlayerPosition;
	vector LastCameraPosition;
	vector LastClientPlayerPos;
	vector LastServerSamplePos;
	vector LastClientSamplePos;
	float LastRequestTime;
	float LastResponseTime;
	float LastAlertTime;
	float LastMovementAlertTime;
	float LastKeybindAlertTime;
	float LastMovementSampleTime;
	float NextCheckTime;
	float SpawnGraceUntil;
	float StartScreenExitGraceUntil;
	float EngineTeleportGraceUntil;
	int ConsecutiveAnomalies;
	int SpeedConsecutiveAnomalies;
	int TeleportConsecutiveAnomalies;
	bool PendingCameraResponse;
	bool WasInStartScreen;
	bool KickPending;
};

class PlayZAntiCheatKickService
{
	static void KickPlayer(PlayerBase player, string detectionTag)
	{
		if (!player || !GetGame() || !GetGame().IsServer())
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		if (!cfg.EnableMovementKick)
			return;

		if (PlayZAntiCheatUtils.IsPlayerExcluded(player))
			return;

		PlayZAntiCheatPlayerState state = player.GetPlayZACState();
		if (state && state.KickPending)
			return;

		if (state)
			state.KickPending = true;

		string playerName = PlayZAntiCheatUtils.GetIdentityName(player);
		string playerId = PlayZAntiCheatUtils.GetIdentityId(player);
		PlayZAntiCheatLogger.Info(string.Format("Kicking player %1 (%2) | reason=%3 | detection=%4", playerName, playerId, PLAYZ_AC_MOVEMENT_KICK_REASON, detectionTag));

		SendKickMessageRpc(player, PLAYZ_AC_MOVEMENT_KICK_REASON);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ExecuteKick, 500, false, player);
	}

	protected static void SendKickMessageRpc(PlayerBase player, string reason)
	{
		if (!player || reason == "")
			return;

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(reason);
		rpc.Send(player, RPC_PLAYZ_AC_KICK_MESSAGE, true, NULL);
	}

	static void ExecuteKick(PlayerBase player)
	{
		if (!player || !GetGame() || !GetGame().IsServer())
			return;

		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return;

		// MissionServer lives in 5_Mission — queue uid for mission-module disconnect handling.
		PlayZAntiCheatKickQueue.Enqueue(identity.GetId());
	}
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

	protected void ResetMovementCounters(PlayZAntiCheatPlayerState state)
	{
		if (!state)
			return;

		state.SpeedConsecutiveAnomalies = 0;
		state.TeleportConsecutiveAnomalies = 0;
	}

	protected void ResetMovementBaselines(PlayZAntiCheatPlayerState state, vector serverPos, vector clientPos, float now)
	{
		if (!state)
			return;

		state.LastServerSamplePos = PlayZAntiCheatUtils.FlattenHorizontal(serverPos);
		state.LastClientSamplePos = PlayZAntiCheatUtils.FlattenHorizontal(clientPos);
		state.LastMovementSampleTime = now;
	}

	void OnPlayerConnected(PlayerBase player)
	{
		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		int graceSeconds = Math.Max(PlayZAntiCheatConfig.Get().CameraRespawnGraceSeconds, 0);
		state.SpawnGraceUntil = GetGame().GetTime() + graceSeconds * 1000;
		state.ConsecutiveAnomalies = 0;
		state.PendingCameraResponse = false;
		state.LastMovementSampleTime = 0;
		state.WasInStartScreen = false;
		state.KickPending = false;
		ResetMovementCounters(state);
	}

	void OnPlayerDeath(PlayerBase player)
	{
		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		int graceSeconds = Math.Max(PlayZAntiCheatConfig.Get().CameraRespawnGraceSeconds, 0);
		state.SpawnGraceUntil = GetGame().GetTime() + graceSeconds * 1000;
		state.ConsecutiveAnomalies = 0;
		state.PendingCameraResponse = false;
		state.LastMovementSampleTime = 0;
		ResetMovementCounters(state);
	}

	void OnEngineTeleport(PlayerBase player)
	{
		if (!player)
			return;

		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		int graceSeconds = Math.Max(cfg.EngineTeleportGraceSeconds, 0);
		float now = GetGame().GetTime();
		state.EngineTeleportGraceUntil = now + graceSeconds * 1000;
		ResetMovementCounters(state);
		ResetMovementBaselines(state, player.GetPosition(), player.GetPosition(), now);
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

	protected bool IsPlayerInMovementSafeState(PlayerBase player, float now, PlayZAntiCheatPlayerState state)
	{
		if (IsPlayerInCameraSafeState(player))
			return true;
		if (player.IsClimbing())
			return true;
		if (player.IsClimbingLadder())
			return true;
		if (player.HasActiveTerjeStartScreen())
			return true;
		if (state && state.SpawnGraceUntil > now)
			return true;
		if (state && state.StartScreenExitGraceUntil > now)
			return true;
		if (state && state.EngineTeleportGraceUntil > now)
			return true;

		vector playerPos = player.GetPosition();
		if (IsSpawnMenuSentinelPosition(playerPos))
			return true;

		return false;
	}

	protected bool NeedsSpotCheckRpc()
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		if (cfg.EnableCameraChecks)
			return true;
		if (cfg.EnableSpeedChecks)
			return true;
		if (cfg.EnableTeleportChecks)
			return true;
		return false;
	}

	void RequestSamples()
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		if (!NeedsSpotCheckRpc())
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();

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

			if (PlayZAntiCheatUtils.IsPlayerExcluded(player))
				continue;

			PlayZAntiCheatPlayerState state = EnsureState(player);
			if (!state)
				continue;

			if (state.PendingCameraResponse && state.LastRequestTime > 0 && now - state.LastRequestTime >= responseTimeoutMs)
			{
				state.PendingCameraResponse = false;
				state.NextCheckTime = now + 1000;
			}

			if (IsPlayerInMovementSafeState(player, now, state))
			{
				state.PendingCameraResponse = false;
				state.NextCheckTime = now + interval;
				continue;
			}

			if (state.NextCheckTime > now)
				continue;

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

	void OnCameraResponse(PlayerBase player, vector cameraPos, vector clientPlayerPos, float clientTime, int flags = 0)
	{
		if (!player)
			return;

		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return;

		if (PlayZAntiCheatUtils.IsPlayerExcluded(player))
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		PlayZAntiCheatPlayerState state = EnsureState(player);
		if (!state)
			return;

		float now = GetGame().GetTime();
		vector serverPlayerPos = player.GetPosition();

		state.LastPlayerPosition = serverPlayerPos;
		state.LastClientPlayerPos = clientPlayerPos;
		state.LastCameraPosition = cameraPos;
		state.LastResponseTime = now;
		state.PendingCameraResponse = false;

		if (cfg.EnableCameraChecks)
			EvaluateCameraSample(player, cameraPos, clientPlayerPos, serverPlayerPos, now, state, flags);

		if (cfg.EnableSpeedChecks || cfg.EnableTeleportChecks)
			EvaluateMovementSample(player, clientPlayerPos, serverPlayerPos, now, state);
	}

	protected void EvaluateCameraSample(PlayerBase player, vector cameraPos, vector clientPlayerPos, vector serverPlayerPos, float now, PlayZAntiCheatPlayerState state, int flags)
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		float clientSeparation = vector.Distance(clientPlayerPos, cameraPos);
		float serverClientDrift = vector.Distance(clientPlayerPos, serverPlayerPos);
		bool isDetachedCamera = (flags & PLAYZ_AC_CAMERA_FLAG_DETACHED) != 0;

		if (state.SpawnGraceUntil > now)
			return;

		if (IsPlayerInCameraSafeState(player))
			return;

		if (IsSpawnMenuSentinelPosition(serverPlayerPos))
			return;

		if (IsSpawnMenuSentinelPosition(clientPlayerPos))
			return;

		if (!isDetachedCamera && cfg.CameraMaxPlausibleSeparation > 0 && clientSeparation > cfg.CameraMaxPlausibleSeparation)
		{
			state.ConsecutiveAnomalies = 0;
			return;
		}

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

	protected void EvaluateMovementSample(PlayerBase player, vector clientPlayerPos, vector serverPlayerPos, float now, PlayZAntiCheatPlayerState state)
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();

		bool inStartScreen = player.HasActiveTerjeStartScreen();
		if (state.WasInStartScreen && !inStartScreen)
		{
			int exitGraceSeconds = Math.Max(cfg.SpeedStartScreenExitGraceSeconds, 0);
			state.StartScreenExitGraceUntil = now + exitGraceSeconds * 1000;
			ResetMovementCounters(state);
			ResetMovementBaselines(state, serverPlayerPos, clientPlayerPos, now);
		}
		state.WasInStartScreen = inStartScreen;

		if (state.LastMovementSampleTime <= 0)
		{
			ResetMovementBaselines(state, serverPlayerPos, clientPlayerPos, now);
			return;
		}

		if (IsPlayerInMovementSafeState(player, now, state))
		{
			ResetMovementCounters(state);
			ResetMovementBaselines(state, serverPlayerPos, clientPlayerPos, now);
			return;
		}

		float elapsedMs = now - state.LastMovementSampleTime;
		if (elapsedMs <= 0)
		{
			ResetMovementBaselines(state, serverPlayerPos, clientPlayerPos, now);
			return;
		}

		float elapsedSeconds = elapsedMs / 1000.0;
		float serverDelta = PlayZAntiCheatUtils.HorizontalDistance(serverPlayerPos, state.LastServerSamplePos);
		float clientDelta = PlayZAntiCheatUtils.HorizontalDistance(clientPlayerPos, state.LastClientSamplePos);
		float serverClientDrift = vector.Distance(clientPlayerPos, serverPlayerPos);

		ResetMovementBaselines(state, serverPlayerPos, clientPlayerPos, now);

		if (cfg.EnableSpeedChecks)
			EvaluateSpeedSample(player, serverDelta, elapsedSeconds, now, state);

		if (cfg.EnableTeleportChecks)
			EvaluateTeleportSample(player, serverDelta, clientDelta, serverClientDrift, now, state);
	}

	protected void EvaluateSpeedSample(PlayerBase player, float serverDelta, float elapsedSeconds, float now, PlayZAntiCheatPlayerState state)
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		float speed = serverDelta / elapsedSeconds;
		float threshold;

		if (player.IsSwimming())
			threshold = cfg.SpeedMaxSwimMps;
		else if (player.IsInVehicle())
			threshold = cfg.SpeedMaxVehicleMps;
		else
			threshold = cfg.SpeedMaxFootMps;

		if (speed <= threshold)
		{
			state.SpeedConsecutiveAnomalies = 0;
			return;
		}

		state.SpeedConsecutiveAnomalies = state.SpeedConsecutiveAnomalies + 1;

		int required = Math.Max(cfg.SpeedConsecutiveAnomaliesToAlert, 1);
		if (state.SpeedConsecutiveAnomalies < required)
			return;

		state.SpeedConsecutiveAnomalies = 0;

		string playerName = PlayZAntiCheatUtils.GetIdentityName(player);
		string playerId = PlayZAntiCheatUtils.GetIdentityId(player);
		string logLine = string.Format("Speed anomaly: %1 (%2) | speed=%3 m/s | threshold=%4 | delta=%5 m | elapsed=%6 s", playerName, playerId, PlayZAntiCheatUtils.FormatFloat(speed, 2), PlayZAntiCheatUtils.FormatFloat(threshold, 1), PlayZAntiCheatUtils.FormatFloat(serverDelta, 2), PlayZAntiCheatUtils.FormatFloat(elapsedSeconds, 2));
		RaiseMovementDetection(player, "SPEED", ":rotating_light: SPEED ANOMALY!", logLine, serverDelta, 0, now, state, false);
	}

	protected bool IsTeleportCorroborated(float serverDelta, float clientDelta, PlayZAntiCheatConfig cfg)
	{
		if (serverDelta < cfg.TeleportSuspiciousMeters)
			return false;
		if (clientDelta < cfg.TeleportSuspiciousMeters)
			return false;
		if (serverDelta <= 0)
			return false;

		float ratio = clientDelta / serverDelta;
		if (ratio < cfg.TeleportClientCorroborationRatio)
			return false;

		return true;
	}

	protected bool ShouldSkipTeleportDesync(float serverDelta, float clientDelta, PlayZAntiCheatConfig cfg)
	{
		if (cfg.TeleportDesyncServerDeltaMeters <= 0)
			return false;
		if (serverDelta < cfg.TeleportDesyncServerDeltaMeters)
			return false;
		if (clientDelta >= cfg.TeleportDesyncMaxClientDeltaMeters)
			return false;
		return true;
	}

	protected void EvaluateTeleportSample(PlayerBase player, float serverDelta, float clientDelta, float serverClientDrift, float now, PlayZAntiCheatPlayerState state)
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();

		if (cfg.CameraMaxServerDriftMeters > 0 && serverClientDrift > cfg.CameraMaxServerDriftMeters)
		{
			ResetMovementCounters(state);
			state.TeleportConsecutiveAnomalies = 0;
			return;
		}

		if (ShouldSkipTeleportDesync(serverDelta, clientDelta, cfg))
		{
			state.TeleportConsecutiveAnomalies = 0;
			return;
		}

		if (!IsTeleportCorroborated(serverDelta, clientDelta, cfg))
		{
			state.TeleportConsecutiveAnomalies = 0;
			return;
		}

		bool isCritical = serverDelta >= cfg.TeleportCriticalMeters;
		if (isCritical)
		{
			state.TeleportConsecutiveAnomalies = 0;
			RaiseMovementDetection(player, "TELEPORT_CRITICAL", ":rotating_light: TELEPORT ANOMALY (CRITICAL)!", string.Format("Teleport anomaly (critical): %1 (%2) | serverDelta=%3 m | clientDelta=%4 m | drift=%5 m", PlayZAntiCheatUtils.GetIdentityName(player), PlayZAntiCheatUtils.GetIdentityId(player), PlayZAntiCheatUtils.FormatFloat(serverDelta, 2), PlayZAntiCheatUtils.FormatFloat(clientDelta, 2), PlayZAntiCheatUtils.FormatFloat(serverClientDrift, 2)), serverDelta, clientDelta, now, state, true);
			return;
		}

		state.TeleportConsecutiveAnomalies = state.TeleportConsecutiveAnomalies + 1;

		int required = Math.Max(cfg.TeleportConsecutiveAnomaliesToAlert, 1);
		if (state.TeleportConsecutiveAnomalies < required)
			return;

		state.TeleportConsecutiveAnomalies = 0;
		RaiseMovementDetection(player, "TELEPORT_CONSECUTIVE", ":rotating_light: TELEPORT ANOMALY (CONSECUTIVE)!", string.Format("Teleport anomaly (consecutive): %1 (%2) | serverDelta=%3 m | clientDelta=%4 m | drift=%5 m", PlayZAntiCheatUtils.GetIdentityName(player), PlayZAntiCheatUtils.GetIdentityId(player), PlayZAntiCheatUtils.FormatFloat(serverDelta, 2), PlayZAntiCheatUtils.FormatFloat(clientDelta, 2), PlayZAntiCheatUtils.FormatFloat(serverClientDrift, 2)), serverDelta, clientDelta, now, state, true);
	}

	protected void RaiseMovementDetection(PlayerBase player, string detectionTag, string embedTitle, string logLine, float serverDelta, float clientDelta, float now, PlayZAntiCheatPlayerState state, bool isTeleport)
	{
		if (isTeleport && state.LastMovementAlertTime > 0)
		{
			int cooldownMs = Math.Max(PlayZAntiCheatConfig.Get().TeleportAlertCooldownSeconds, 0) * 1000;
			if (now - state.LastMovementAlertTime < cooldownMs)
				return;
		}

		state.LastMovementAlertTime = now;
		PlayZAntiCheatLogger.Info(logLine);

		string webhook = PlayZAntiCheatUtils.GetMovementWebhookUrl();
		if (webhook != "")
		{
			string playerName = PlayZAntiCheatUtils.GetIdentityName(player);
			string playerId = PlayZAntiCheatUtils.GetIdentityId(player);
			ref array<ref PlayZDiscordEmbedField> fields = new array<ref PlayZDiscordEmbedField>();
			PlayZAntiCheatUtils.AddEmbedField(fields, "Server delta", PlayZAntiCheatUtils.FormatFloat(serverDelta, 2) + " m");
			if (clientDelta > 0)
				PlayZAntiCheatUtils.AddEmbedField(fields, "Client delta", PlayZAntiCheatUtils.FormatFloat(clientDelta, 2) + " m");
			PlayZAntiCheatUtils.AddEmbedField(fields, "Detection", detectionTag);

			string description = string.Format("Player: %1 (%2)", playerName, playerId);
			string payload = PlayZAntiCheatUtils.BuildWebhookEmbed("PlayZ AntiCheat", embedTitle, description, 0xFFA500, fields);
			if (payload != "")
				PlayZAntiCheatWebhookDispatcher.Get().Enqueue(webhook, payload);
		}

		PlayZAntiCheatKickService.KickPlayer(player, detectionTag);
	}
};
