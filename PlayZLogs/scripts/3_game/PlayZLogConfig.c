// Server-only logging configuration for PlayZLogs.
//
// As of the Logs/ + Logs/Index/ removal, PlayZLogs writes ONLY to
// `$profile:PlayZ/LogsByPlayer/<date>/[<hour>/]player_<playerId>.jsonl`.
// The central events.jsonl stream and the compact index stream no longer
// exist, so every knob that tuned them has been removed:
//   - m_EnableCentralJsonl, m_EnableIndexStream, m_EnableDomainPartitions
//   - m_EnableAutoHourlyPartitions + m_AutoHourlyOn/OffEventsPerSecond
//   - m_DamageHitOmitIndexStream
// Remaining flags apply to the player-centric stream only.
class PlayZLogConfig
{
	static const string CONFIG_ROOT = "$profile:PlayZ/";
	static const string CONFIG_PATH = "$profile:PlayZ/Log.json";

	// Domain enables
	bool m_EnableLogWeather = false;
	bool m_EnableLogPeople = true;
	bool m_EnableLogDamage = true;
	bool m_EnableLogBaseBuilding = true;
	bool m_EnableLogRecipes = true;
	bool m_EnableLogActions = true;
	bool m_EnableLogAdmin = true;
	bool m_EnableLogPlacement = true;
	bool m_EnableLogAI = true;
	bool m_LogAIPlayerEngagementOnly = true;

	// Player-centric stream (the only remaining sink). Mirror writes the
	// same event into the target player's file when actor != target.
	bool m_EnablePlayerCentricLogs = true;
	bool m_EnablePlayerCentricMirrorTarget = true;

	// Optional hour-level sub-directory beneath the date folder. Off by
	// default because daily grouping is sufficient for DayZ event volume.
	bool m_EnableHourlyPartitions = false;

	// Snapshot cadence (seconds) — used by MissionServer scheduling callbacks.
	int m_PlayerSnapshotInterval = 15;
	int m_VehicleSnapshotInterval = 15;
	int m_VehicleStationarySnapshotInterval = 1800;
	int m_WeatherLogInterval = 60;

	// Rate limits. Global cap is the hard ceiling; per-domain caps prevent
	// one noisy domain from starving the others.
	int m_GlobalMaxEventsPerSecond = 300;
	int m_MaxEventsPerSecondPeople = 120;
	int m_MaxEventsPerSecondDamage = 120;
	int m_MaxEventsPerSecondVehicle = 80;
	int m_MaxEventsPerSecondBaseBuilding = 80;
	int m_MaxEventsPerSecondRecipes = 40;
	int m_MaxEventsPerSecondActions = 80;
	int m_MaxEventsPerSecondAdmin = 20;
	int m_MaxEventsPerSecondPlacement = 40;
	int m_MaxEventsPerSecondWeather = 10;
	int m_MaxEventsPerSecondAI = 80;

	// Damage-hit dedup: 0 = log every hit (default); >0 = min ms between
	// identical (source, target, zone, ammo) tuples. The multi-layer-hit
	// fix (vest -> top -> arm -> torso from one bullet) relies on the key
	// including zone+ammo so legitimate multi-zone hits are NEVER collapsed.
	bool m_DamageHitMinimalPayload = true;
	int m_DamageHitMinIntervalMs = 0;
	bool m_DamageHitOmitPlayerCentricRecords = false;

	void PlayZLogConfig()
	{
	}

	void Load()
	{
		if (FileExist(CONFIG_PATH))
		{
			JsonFileLoader<PlayZLogConfig>.JsonLoadFile(CONFIG_PATH, this);
		}
		else
		{
			SetDefaults();
			Save();
		}
	}

	void Save()
	{
		if (!FileExist(CONFIG_ROOT))
		{
			MakeDirectory(CONFIG_ROOT);
		}
		JsonFileLoader<PlayZLogConfig>.JsonSaveFile(CONFIG_PATH, this);
	}

	void SetDefaults()
	{
		m_EnableLogWeather = false;
		m_EnableLogPeople = true;
		m_EnableLogDamage = true;
		m_EnableLogBaseBuilding = true;
		m_EnableLogRecipes = true;
		m_EnableLogActions = true;
		m_EnableLogAdmin = true;
		m_EnableLogPlacement = true;
		m_EnableLogAI = true;
		m_LogAIPlayerEngagementOnly = true;

		m_EnablePlayerCentricLogs = true;
		m_EnablePlayerCentricMirrorTarget = true;
		m_EnableHourlyPartitions = true;

		m_PlayerSnapshotInterval = 15;
		m_VehicleSnapshotInterval = 15;
		m_VehicleStationarySnapshotInterval = 1800;
		m_WeatherLogInterval = 60;

		m_GlobalMaxEventsPerSecond = 300;
		m_MaxEventsPerSecondPeople = 120;
		m_MaxEventsPerSecondDamage = 120;
		m_MaxEventsPerSecondVehicle = 80;
		m_MaxEventsPerSecondBaseBuilding = 80;
		m_MaxEventsPerSecondRecipes = 40;
		m_MaxEventsPerSecondActions = 80;
		m_MaxEventsPerSecondAdmin = 20;
		m_MaxEventsPerSecondPlacement = 40;
		m_MaxEventsPerSecondWeather = 10;
		m_MaxEventsPerSecondAI = 80;

		m_DamageHitMinimalPayload = true;
		m_DamageHitMinIntervalMs = 0;
		m_DamageHitOmitPlayerCentricRecords = false;
	}
}
