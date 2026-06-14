class PlayZStashDigRegistryEntry
{
	string cargoPid;
	string cargoType;
	string stashPid;
	string stashType;
	string diggerName;
	string diggerSteam64;
	float digInPosX;
	float digInPosY;
	float digInPosZ;
	int digInUnixTime;
	string toolType;
}

class PlayZStashDigRegistryFile
{
	ref array<ref PlayZStashDigRegistryEntry> m_Entries;

	void PlayZStashDigRegistryFile()
	{
		m_Entries = new array<ref PlayZStashDigRegistryEntry>();
	}
}

class PlayZStashDigRegistry
{
	static const string REGISTRY_PATH = "$profile:PlayZ/StashDigRegistry.json";
	static const float SAVE_DEBOUNCE_SEC = 5.0;

	protected static ref map<string, ref PlayZStashDigRegistryEntry> s_Entries;
	protected static bool s_Dirty;
	protected static bool s_SaveScheduled;

	static void Init()
	{
		if (!s_Entries)
			s_Entries = new map<string, ref PlayZStashDigRegistryEntry>();

		Load();
	}

	static void Shutdown()
	{
		if (s_Dirty)
			SaveNow();
	}

	static void Load()
	{
		if (!s_Entries)
			s_Entries = new map<string, ref PlayZStashDigRegistryEntry>();
		else
			s_Entries.Clear();

		if (!FileExist(REGISTRY_PATH))
			return;

		ref PlayZStashDigRegistryFile fileData = new PlayZStashDigRegistryFile();
		JsonFileLoader<PlayZStashDigRegistryFile>.JsonLoadFile(REGISTRY_PATH, fileData);

		if (!fileData || !fileData.m_Entries)
			return;

		int ttlDays = GetTtlDays();
		int nowUnix = PlayZLogUtcTime.NowUnixUtc();
		int ttlSeconds = ttlDays * 86400;

		foreach (ref PlayZStashDigRegistryEntry entry : fileData.m_Entries)
		{
			if (!entry || entry.cargoPid == "")
				continue;

			if (entry.digInUnixTime > 0 && nowUnix - entry.digInUnixTime > ttlSeconds)
				continue;

			s_Entries.Set(entry.cargoPid, entry);
		}

		s_Dirty = false;
	}

	static void RegisterEntry(PlayZStashDigRegistryEntry entry)
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		if (!entry || entry.cargoPid == "")
			return;

		if (!s_Entries)
			Init();

		s_Entries.Set(entry.cargoPid, entry);
		ScheduleSave();
	}

	static PlayZStashDigRegistryEntry Lookup(string cargoPid)
	{
		if (!s_Entries || cargoPid == "")
			return null;

		return s_Entries.Get(cargoPid);
	}

	static void Remove(string cargoPid)
	{
		if (!s_Entries || cargoPid == "")
			return;

		if (s_Entries.Contains(cargoPid))
		{
			s_Entries.Remove(cargoPid);
			ScheduleSave();
		}
	}

	protected static int GetTtlDays()
	{
		if (PlayZLogger.m_Config && PlayZLogger.m_Config.m_StashDigRegistryTtlDays > 0)
			return PlayZLogger.m_Config.m_StashDigRegistryTtlDays;

		return 14;
	}

	protected static void ScheduleSave()
	{
		s_Dirty = true;

		if (s_SaveScheduled || !GetGame())
			return;

		s_SaveScheduled = true;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SaveNowDebounced, SAVE_DEBOUNCE_SEC * 1000, false);
	}

	protected static void SaveNowDebounced()
	{
		s_SaveScheduled = false;
		if (s_Dirty)
			SaveNow();
	}

	protected static void SaveNow()
	{
		if (!s_Entries)
			return;

		PruneExpired();

		if (!FileExist(PlayZLogConfig.CONFIG_ROOT))
			MakeDirectory(PlayZLogConfig.CONFIG_ROOT);

		ref PlayZStashDigRegistryFile fileData = new PlayZStashDigRegistryFile();
		foreach (string cargoPid, ref PlayZStashDigRegistryEntry entry : s_Entries)
		{
			if (entry)
				fileData.m_Entries.Insert(entry);
		}

		JsonFileLoader<PlayZStashDigRegistryFile>.JsonSaveFile(REGISTRY_PATH, fileData);
		s_Dirty = false;
	}

	protected static void PruneExpired()
	{
		if (!s_Entries)
			return;

		int ttlDays = GetTtlDays();
		int nowUnix = PlayZLogUtcTime.NowUnixUtc();
		int ttlSeconds = ttlDays * 86400;

		array<string> expiredKeys = new array<string>();
		foreach (string cargoPid, ref PlayZStashDigRegistryEntry entry : s_Entries)
		{
			if (!entry)
			{
				expiredKeys.Insert(cargoPid);
				continue;
			}

			if (entry.digInUnixTime > 0 && nowUnix - entry.digInUnixTime > ttlSeconds)
				expiredKeys.Insert(cargoPid);
		}

		foreach (string expiredKey : expiredKeys)
		{
			s_Entries.Remove(expiredKey);
		}
	}
}
