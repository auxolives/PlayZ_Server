// RPC_PLAYZ_AC_CAMERA_REQUEST / RPC_PLAYZ_AC_CAMERA_RESPONSE are declared in
// PlayZ_Client/PlayZAntiCheatClient/scripts/3_Game/PlayZAntiCheatClient_Shared.c
// and made available here via CfgPatches requiredAddons = { "PlayZAntiCheatClient" }.

static const string PLAYZ_AC_PROFILE_DIR   = "$profile:PlayZ/";
static const string PLAYZ_AC_CONFIG_PATH   = PLAYZ_AC_PROFILE_DIR + "AntiCheat.json";
static const string PLAYZ_AC_LOG_PREFIX    = "[PlayZ AntiCheat]";
static const string PLAYZ_AC_MOVEMENT_KICK_REASON = "SPEEDHACK SUSPICION REPORTED";

class PlayZAntiCheatConfig
{
	bool EnableBallisticChecks = true;
	bool EnableCameraChecks = true;
	bool LogToServerConsole = true;
	bool LogToRPT = true;
	float BallisticSuspiciousDistance = 600.0;
	int BallisticMaxObstacleCount = 3;
	float BallisticRaycastRadius = 0.05;
	// Max allowed camera->body delta, measured from client-coherent sample.
	float CameraMaxSeparation = 10.0;
	// If the client-reported player pos disagrees with the server pos by more
	// than this many meters, treat as a lag glitch / untrusted sample and SKIP
	// (do not alert). Keeps honest lagging players out of logs without letting
	// a fully-fabricated client position through.
	float CameraMaxServerDriftMeters = 20.0;
	// Samples with camera->body delta larger than this are assumed to be
	// transition artifacts (loading screens, intro scene at <-146, 479, 46>,
	// map-edge glitches) rather than cheat signal, and are silently discarded
	// WITHOUT incrementing the consecutive-anomaly counter. Set to 0 to disable.
	float CameraMaxPlausibleSeparation = 300.0;
	// Grace window after spawn/respawn where samples are skipped (camera not
	// settled). Engine intro cam / character creation sequence can run well
	// past the OnClientReadyEvent tick on slow machines or poor networks, so
	// this needs to be generous enough to cover the worst-case transition.
	int CameraRespawnGraceSeconds = 20;
	// Require this many consecutive anomalous samples before firing an alert.
	int CameraConsecutiveAnomaliesToAlert = 2;
	int CameraSampleIntervalSeconds = 8;
	int CameraResponseTimeoutSeconds = 12;
	string DiscordWebhookUrl = "";
	string DiscordCameraWebhookUrl = "";
	// Steam64 IDs (identity.GetPlainId()) skipped by all anti-cheat checks.
	ref array<string> ExcludedSteamIds64 = new array<string>();
	// Steam64 IDs skipped by keybind alerts only. Does not affect other checks.
	// Admins in ExcludedSteamIds64 are skipped for keybind automatically (one-way; not recursive).
	ref array<string> ExcludedKeybindSteamIds64 = new array<string>();
	bool EnableSpeedChecks = false;
	bool EnableTeleportChecks = true;
	bool EnableMovementKick = true;
	float SpeedMaxFootMps = 9.0;
	float SpeedMaxVehicleMps = 100.0;
	float SpeedMaxSwimMps = 3.5;
	int SpeedConsecutiveAnomaliesToAlert = 3;
	float TeleportSuspiciousMeters = 75.0;
	float TeleportCriticalMeters = 150.0;
	float TeleportClientCorroborationRatio = 0.75;
	int TeleportConsecutiveAnomaliesToAlert = 4;
	int TeleportAlertCooldownSeconds = 60;
	float TeleportDesyncServerDeltaMeters = 100.0;
	float TeleportDesyncMaxClientDeltaMeters = 40.0;
	int EngineTeleportGraceSeconds = 5;
	int SpeedStartScreenExitGraceSeconds = 5;
	string DiscordMovementWebhookUrl = "";
	bool EnableKeybindChecks = true;
	int KeybindAlertCooldownSeconds = 1;
	int KeybindMaxPayloadChars = 256;
	string DiscordKeybindWebhookUrl = "";

	private static ref PlayZAntiCheatConfig s_Instance;

	static PlayZAntiCheatConfig Get()
	{
		if (!s_Instance)
		{
			s_Instance = new PlayZAntiCheatConfig();
			s_Instance.Load();
		}
		return s_Instance;
	}

	void Load()
	{
		if (!FileExist(PLAYZ_AC_PROFILE_DIR))
			MakeDirectory(PLAYZ_AC_PROFILE_DIR);

		if (!FileExist(PLAYZ_AC_CONFIG_PATH))
		{
			Save();
			return;
		}

		JsonFileLoader<PlayZAntiCheatConfig>.JsonLoadFile(PLAYZ_AC_CONFIG_PATH, this);

		if (!ExcludedSteamIds64)
			ExcludedSteamIds64 = new array<string>();

		if (!ExcludedKeybindSteamIds64)
			ExcludedKeybindSteamIds64 = new array<string>();
	}

	void Save()
	{
		JsonFileLoader<PlayZAntiCheatConfig>.JsonSaveFile(PLAYZ_AC_CONFIG_PATH, this);
	}
};

class PlayZAntiCheatLogger
{
	static void Info(string message)
	{
		if (GetGame().IsServer())
		{
			if (PlayZAntiCheatConfig.Get().LogToServerConsole)
				Print(string.Format("%1 %2", PLAYZ_AC_LOG_PREFIX, message));

			if (PlayZAntiCheatConfig.Get().LogToRPT)
				PrintToRPT(string.Format("%1 %2", PLAYZ_AC_LOG_PREFIX, message));
		}
	}

	static void Error(string message)
	{
		if (GetGame().IsServer())
		{
			Print(string.Format("%1 [ERROR] %2", PLAYZ_AC_LOG_PREFIX, message));
			PrintToRPT(string.Format("%1 [ERROR] %2", PLAYZ_AC_LOG_PREFIX, message));
		}
	}
};

class PlayZAntiCheatKickQueue
{
	protected static ref array<string> s_PendingUids;

	static void Enqueue(string uid)
	{
		if (uid == "")
			return;

		if (!s_PendingUids)
			s_PendingUids = new array<string>();

		if (s_PendingUids.Find(uid) > -1)
			return;

		s_PendingUids.Insert(uid);
	}

	static bool Dequeue(out string uid)
	{
		if (!s_PendingUids || s_PendingUids.Count() == 0)
			return false;

		uid = s_PendingUids[0];
		s_PendingUids.RemoveOrdered(0);
		return true;
	}
};

class PlayZAntiCheatWebhookMessage
{
	string Url;
	string Payload;
}

class PlayZAntiCheatRestCallback : RestCallback
{
	protected PlayZAntiCheatWebhookDispatcher m_Dispatcher;

	void PlayZAntiCheatRestCallback(PlayZAntiCheatWebhookDispatcher dispatcher)
	{
		m_Dispatcher = dispatcher;
	}

	override void OnSuccess(string data, int dataSize)
	{
		m_Dispatcher.OnRequestFinished();
	}

	override void OnError(int errorCode)
	{
		if (errorCode != 8)
			PlayZAntiCheatLogger.Error(string.Format("Webhook POST failed with error code %1", errorCode));
		m_Dispatcher.OnRequestFinished();
	}

	override void OnTimeout()
	{
		PlayZAntiCheatLogger.Error("Webhook POST timed out");
		m_Dispatcher.OnRequestFinished();
	}
};

class PlayZAntiCheatWebhookDispatcher
{
	protected ref array<ref PlayZAntiCheatWebhookMessage> m_Queue = new array<ref PlayZAntiCheatWebhookMessage>;
	protected bool m_IsProcessing;
	protected static ref PlayZAntiCheatWebhookDispatcher s_Instance;

	static PlayZAntiCheatWebhookDispatcher Get()
	{
		if (!s_Instance)
			s_Instance = new PlayZAntiCheatWebhookDispatcher();
		return s_Instance;
	}

	void Enqueue(string url, string payload)
	{
		if (url == "" || payload == "")
			return;

		ref PlayZAntiCheatWebhookMessage message = new PlayZAntiCheatWebhookMessage();
		message.Url = url;
		message.Payload = payload;
		m_Queue.Insert(message);
		ProcessQueue();
	}

	void ProcessQueue()
	{
		if (m_IsProcessing || m_Queue.Count() == 0)
			return;

		if (!GetGame() || !GetGame().IsServer())
		{
			m_Queue.Clear();
			return;
		}

		ref PlayZAntiCheatWebhookMessage nextMessage = m_Queue[0];
		m_Queue.RemoveOrdered(0);

		RestApi api = GetRestApi();
		if (!api)
		{
			api = CreateRestApi();
		}

		if (!api)
		{
			PlayZAntiCheatLogger.Error("Unable to acquire RestApi for webhook dispatch");
			return;
		}

		RestContext ctx = api.GetRestContext(nextMessage.Url);
		if (!ctx)
		{
			PlayZAntiCheatLogger.Error("Unable to obtain RestContext for webhook URL");
			return;
		}

		m_IsProcessing = true;

		ctx.SetHeader("application/json");
		RestCallback cb = new PlayZAntiCheatRestCallback(this);
		ctx.POST(cb, "", nextMessage.Payload);
	}

	void OnRequestFinished()
	{
		m_IsProcessing = false;
		if (m_Queue.Count() > 0)
		{
			// slight delay to avoid rate limits
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ProcessQueue, 500, false);
		}
	}
};

class PlayZDiscordEmbedField
{
	string name;
	string value;
	bool inline;
};

class PlayZDiscordEmbed
{
	string title;
	string description;
	int color;
	ref array<ref PlayZDiscordEmbedField> fields;
};

class PlayZDiscordWebhookPayload
{
	string username;
	ref array<ref PlayZDiscordEmbed> embeds;
};

class PlayZAntiCheatUtils
{
	static Man ResolvePlayer(Object obj)
	{
		Man player = Man.Cast(obj);
		if (player)
			return player;

		EntityAI entity = EntityAI.Cast(obj);
		while (entity)
		{
			player = Man.Cast(entity.GetHierarchyParent());
			if (player)
				return player;

			entity = EntityAI.Cast(entity.GetHierarchyParent());
		}

		return null;
	}

	static string GetIdentityName(Man player)
	{
		if (!player)
			return "Unknown";

		PlayerIdentity identity = player.GetIdentity();
		if (identity)
			return identity.GetName();

		return player.GetDisplayName();
	}

	static string GetIdentityId(Man player)
	{
		if (player)
		{
			PlayerIdentity identity = player.GetIdentity();
			if (identity)
				return identity.GetId();
		}
		return "Unknown";
	}

	static string GetSteamId64(Man player)
	{
		if (player)
		{
			PlayerIdentity identity = player.GetIdentity();
			if (identity)
				return identity.GetPlainId();
		}
		return "";
	}

	static bool IsPlayerExcluded(Man player)
	{
		ref array<string> excluded = PlayZAntiCheatConfig.Get().ExcludedSteamIds64;
		if (!excluded || excluded.Count() == 0)
			return false;

		string steam64 = GetSteamId64(player);
		if (steam64 == "")
			return false;

		return excluded.Find(steam64) > -1;
	}

	static bool IsPlayerKeybindExcluded(Man player)
	{
		// Global admin exclusion covers keybind — no need to duplicate Steam64 in both arrays.
		if (IsPlayerExcluded(player))
			return true;

		ref array<string> excluded = PlayZAntiCheatConfig.Get().ExcludedKeybindSteamIds64;
		if (!excluded || excluded.Count() == 0)
			return false;

		string steam64 = GetSteamId64(player);
		if (steam64 == "")
			return false;

		return excluded.Find(steam64) > -1;
	}

	static vector FlattenHorizontal(vector value)
	{
		value[1] = 0;
		return value;
	}

	static float HorizontalDistance(vector a, vector b)
	{
		return vector.Distance(FlattenHorizontal(a), FlattenHorizontal(b));
	}

	static string GetMovementWebhookUrl()
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		string webhook = cfg.DiscordMovementWebhookUrl;
		if (webhook == "")
			webhook = cfg.DiscordWebhookUrl;
		return webhook;
	}

	static string GetKeybindWebhookUrl()
	{
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		string webhook = cfg.DiscordKeybindWebhookUrl;
		if (webhook == "")
			webhook = cfg.DiscordWebhookUrl;
		return webhook;
	}

	static string FormatFloat(float value, int decimals)
	{
		float pow = Math.Pow(10.0, decimals);
		float rounded = Math.Round(value * pow) / pow;

		string text = rounded.ToString();
		if (decimals <= 0)
			return text;

		int dotIndex = text.IndexOf(".");
		if (dotIndex == -1)
		{
			text = text + ".";
			dotIndex = text.Length() - 1;
		}

		int fractionalDigits = text.Length() - dotIndex - 1;
		while (fractionalDigits < decimals)
		{
			text = text + "0";
			fractionalDigits++;
		}
		return text;
	}

	static string FormatVector(vector value, int decimals)
	{
		return string.Format("<%1, %2, %3>", FormatFloat(value[0], decimals), FormatFloat(value[1], decimals), FormatFloat(value[2], decimals));
	}

	static void AddEmbedField(ref array<ref PlayZDiscordEmbedField> fields, string name, string value, bool inline = false)
	{
		if (!fields)
			fields = new array<ref PlayZDiscordEmbedField>();

		ref PlayZDiscordEmbedField field = new PlayZDiscordEmbedField();
		field.name = name;
		field.value = value;
		field.inline = inline;
		fields.Insert(field);
	}

	static string BuildWebhookEmbed(string username, string title, string description, int color, ref array<ref PlayZDiscordEmbedField> fields)
	{
		JsonSerializer serializer = new JsonSerializer();
		ref PlayZDiscordWebhookPayload payload = new PlayZDiscordWebhookPayload();
		payload.username = username;
		payload.embeds = new array<ref PlayZDiscordEmbed>();

		if (!fields)
			fields = new array<ref PlayZDiscordEmbedField>();

		ref PlayZDiscordEmbed embed = new PlayZDiscordEmbed();
		embed.title = title;
		embed.description = description;
		embed.color = color;
		embed.fields = fields;

		payload.embeds.Insert(embed);

		string json;
		if (!serializer.WriteToString(payload, false, json))
		{
			PlayZAntiCheatLogger.Error("Failed to serialize webhook payload");
			return "";
		}

		return json;
	}
};

class PlayZAntiCheatObstacleReport
{
	int TotalCount;
	int TreeCount;
	int BushCount;
	int BuildingCount;
	int BaseBuildingCount;
	int TentCount;
	ref array<string> SampleTypes;
	protected ref array<Object> m_UniqueObjects;

	void PlayZAntiCheatObstacleReport()
	{
		SampleTypes = new array<string>();
		m_UniqueObjects = new array<Object>();
	}

	bool Register(Object obj)
	{
		if (!obj)
			return false;

		if (m_UniqueObjects.Find(obj) > -1)
			return false;

		if (obj.IsTree())
			TreeCount++;
		else if (obj.IsBush())
			BushCount++;
		else if (obj.IsBuilding())
			BuildingCount++;
		else if (obj.CanUseConstruction())
			BaseBuildingCount++;
		else if (obj.IsItemTent())
			TentCount++;
		else
			return false;

		m_UniqueObjects.Insert(obj);
		TotalCount++;

		if (SampleTypes.Count() < 5)
			SampleTypes.Insert(obj.GetType());

		return true;
	}

	string Summary()
	{
		if (TotalCount == 0)
			return "none";

		return string.Format("%1 | samples=%2", FormatCounts(), FormatSamples());
	}

	string FormatCounts()
	{
		return string.Format("trees=%1 bushes=%2 building=%3 basebuilding=%4 tents=%5", TreeCount, BushCount, BuildingCount, BaseBuildingCount, TentCount);
	}

	string FormatSamples()
	{
		if (SampleTypes.Count() == 0)
			return "none";

		string samples = "";
		int sampleCount = SampleTypes.Count();
		for (int i = 0; i < sampleCount; i++)
		{
			if (i > 0)
				samples = samples + ", ";
			samples = samples + SampleTypes[i];
		}
		return samples;
	}
};

class PlayZAntiCheatBallisticService
{
	protected ref array<ref RaycastRVResult> m_ResultsBuffer;
	protected ref array<Object> m_ExcludedBuffer;

	void PlayZAntiCheatBallisticService()
	{
		m_ResultsBuffer = new array<ref RaycastRVResult>();
		m_ExcludedBuffer = new array<Object>();
	}
	void OnProjectileStopped(ProjectileStoppedInfo info)
	{
		if (!info || !GetGame() || !GetGame().IsServer())
			return;

		ObjectCollisionInfo objectInfo = ObjectCollisionInfo.Cast(info);
		if (!objectInfo)
			return;

		ProcessObjectCollision(objectInfo);
	}

	protected void ProcessObjectCollision(ObjectCollisionInfo info)
	{
		if (!info)
			return;

		Object hitObj = info.GetHitObj();
		Man victim = Man.Cast(hitObj);
		if (!victim || !victim.IsPlayer())
			return;

		Man attacker = PlayZAntiCheatUtils.ResolvePlayer(info.GetSource());
		if (!attacker || attacker == victim)
			return;

		if (PlayZAntiCheatUtils.IsPlayerExcluded(attacker))
			return;

		vector attackerPos = attacker.GetPosition();
		vector impactPos = info.GetPos();

		float distance = vector.Distance(attackerPos, impactPos);
		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();

		bool suspiciousDistance = cfg.BallisticSuspiciousDistance > 0 && distance >= cfg.BallisticSuspiciousDistance;
		bool suspiciousObstacles = false;
		ref PlayZAntiCheatObstacleReport report;

		if (cfg.BallisticMaxObstacleCount >= 0)
		{
			report = EvaluateObstacles(attacker, hitObj, attackerPos, impactPos, cfg);
			if (report)
				suspiciousObstacles = report.TotalCount > 0 && report.TotalCount >= cfg.BallisticMaxObstacleCount;
		}

		if (!suspiciousDistance && !suspiciousObstacles)
			return;

		string attackerName = PlayZAntiCheatUtils.GetIdentityName(attacker);
		string victimName = PlayZAntiCheatUtils.GetIdentityName(victim);
		string attackerId = PlayZAntiCheatUtils.GetIdentityId(attacker);
		string victimId = PlayZAntiCheatUtils.GetIdentityId(victim);
		string distanceText = PlayZAntiCheatUtils.FormatFloat(distance, 1);

		string detectionTag;
		if (suspiciousDistance && suspiciousObstacles)
			detectionTag = "DISTANCE + OBSTACLES";
		else if (suspiciousDistance)
			detectionTag = "DISTANCE";
		else
			detectionTag = "OBSTACLES";

		string logLine = string.Format("Ballistic anomaly (%1): %2 (%3) -> %4 (%5) | distance=%6 m | ammo=%7 | hitPos=%8", detectionTag, attackerName, attackerId, victimName, victimId, distanceText, info.GetAmmoType(), PlayZAntiCheatUtils.FormatVector(impactPos, 2));

		if (report && report.TotalCount > 0)
			logLine = logLine + string.Format(" | obstacles=%1 | counts=%2 | samples=%3", report.TotalCount, report.FormatCounts(), report.FormatSamples());

		PlayZAntiCheatLogger.Info(logLine);

		string webhookUrl = cfg.DiscordWebhookUrl;
		if (webhookUrl != "")
		{
			string description = string.Format("Shooter: %1 (%2)\nTarget: %3 (%4)", attackerName, attackerId, victimName, victimId);
			ref array<ref PlayZDiscordEmbedField> fields = new array<ref PlayZDiscordEmbedField>();

			PlayZAntiCheatUtils.AddEmbedField(fields, "Distance", distanceText + " m");
			PlayZAntiCheatUtils.AddEmbedField(fields, "Ammo", info.GetAmmoType());
			PlayZAntiCheatUtils.AddEmbedField(fields, "Impact Pos", PlayZAntiCheatUtils.FormatVector(impactPos, 2));

			if (report && report.TotalCount > 0)
			{
				PlayZAntiCheatUtils.AddEmbedField(fields, "Obstacles", string.Format("%1", report.TotalCount));
				PlayZAntiCheatUtils.AddEmbedField(fields, "Counts", report.FormatCounts());
				PlayZAntiCheatUtils.AddEmbedField(fields, "Samples", report.FormatSamples());
			}

			string payload = PlayZAntiCheatUtils.BuildWebhookEmbed("PlayZ AntiCheat", ":rotating_light: " + detectionTag + " ANOMALY!", description, 0xFFA500, fields);
			if (payload == "")
				return;
			PlayZAntiCheatWebhookDispatcher.Get().Enqueue(webhookUrl, payload);
		}
	}

	protected PlayZAntiCheatObstacleReport EvaluateObstacles(Man attacker, Object target, vector from, vector to, PlayZAntiCheatConfig cfg)
	{
		ref PlayZAntiCheatObstacleReport report = new PlayZAntiCheatObstacleReport();

		RaycastRVParams rayInput = new RaycastRVParams(from, to, attacker, cfg.BallisticRaycastRadius);
		rayInput.type = ObjIntersectIFire;
		rayInput.flags = CollisionFlags.ALLOBJECTS;
		rayInput.sorted = true;

		m_ResultsBuffer.Clear();
		m_ExcludedBuffer.Clear();
		m_ExcludedBuffer.Insert(attacker);
		if (target)
			m_ExcludedBuffer.Insert(target);

		if (DayZPhysics.RaycastRVProxy(rayInput, m_ResultsBuffer, m_ExcludedBuffer))
		{
			int resultsCount = m_ResultsBuffer.Count();
			for (int i = 0; i < resultsCount; i++)
			{
				RaycastRVResult res = m_ResultsBuffer[i];
				if (!res)
					continue;

				Object obj = res.obj;
				if (!obj)
					obj = res.parent;

				if (!obj || obj == attacker || obj == target)
					continue;

				report.Register(obj);
			}
		}

		vector dir = to - from;
		float length = dir.Length();
		if (length <= 0.01)
			return report;

		dir.Normalize();

		vector marchStart = from + dir * 0.05;
		PhxInteractionLayers layerMask = PhxInteractionLayers.DEFAULT;
		layerMask = layerMask | PhxInteractionLayers.BUILDING | PhxInteractionLayers.DOOR | PhxInteractionLayers.FENCE | PhxInteractionLayers.VEHICLE | PhxInteractionLayers.ITEM_SMALL | PhxInteractionLayers.ITEM_LARGE | PhxInteractionLayers.FIREGEOM | PhxInteractionLayers.ROADWAY | PhxInteractionLayers.TERRAIN | PhxInteractionLayers.AI | PhxInteractionLayers.RAGDOLL | PhxInteractionLayers.RAGDOLL_NO_CHARACTER;

		int guard = 0;
		Object currentIgnore = attacker;
		while (guard < 15 && vector.Distance(marchStart, to) > 0.05)
		{
			Object hitObject;
			vector hitPos;
			vector hitNormal;
			float hitFraction;

			if (!DayZPhysics.RayCastBullet(marchStart, to, layerMask, currentIgnore, hitObject, hitPos, hitNormal, hitFraction))
				break;

			if (!hitObject)
				break;

			if (hitObject != attacker && hitObject != target)
				report.Register(hitObject);

			float advance = Math.Max(vector.Distance(marchStart, hitPos), 0.05);
			marchStart = hitPos + dir * Math.Max(advance * 0.1, 0.05);
			currentIgnore = hitObject;
			guard++;
		}

		return report;
	}
};

modded class DayZGame
{
	protected ref PlayZAntiCheatBallisticService m_PlayZACBallistics;

	PlayZAntiCheatBallisticService GetPlayZACBallistics()
	{
		if (!m_PlayZACBallistics)
			m_PlayZACBallistics = new PlayZAntiCheatBallisticService();

		return m_PlayZACBallistics;
	}

	override void OnAfterCreate()
	{
		super.OnAfterCreate();
		if (GetGame() && GetGame().IsServer())
			PlayZAntiCheatConfig.Get();
	}

	override void OnProjectileStopped(ProjectileStoppedInfo info)
	{
		super.OnProjectileStopped(info);
		if (GetGame().IsServer() && PlayZAntiCheatConfig.Get().EnableBallisticChecks)
			GetPlayZACBallistics().OnProjectileStopped(info);
	}

	override void OnProjectileStoppedInTerrain(TerrainCollisionInfo info)
	{
		super.OnProjectileStoppedInTerrain(info);
		if (GetGame().IsServer() && PlayZAntiCheatConfig.Get().EnableBallisticChecks)
			GetPlayZACBallistics().OnProjectileStopped(info);
	}

	override void OnProjectileStoppedInObject(ObjectCollisionInfo info)
	{
		super.OnProjectileStoppedInObject(info);
		if (GetGame().IsServer() && PlayZAntiCheatConfig.Get().EnableBallisticChecks)
			GetPlayZACBallistics().OnProjectileStopped(info);
	}
};
