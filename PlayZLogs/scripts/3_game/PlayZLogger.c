// PlayZLogs — server-only structured event logger.
//
// As of the Logs/ + Logs/Index/ removal, the ONLY output sink is the
// player-centric stream at:
//   $profile:PlayZ/LogsByPlayer/<yyyy-mm-dd>/[<HH>/]player_<playerId>.jsonl
//
// Every event is attributed to one (or two, if mirroring) player files
// based on actor / target. Non-player actors (Z-, A-, V-, P- IDs) are
// dropped by IsPlayerId(), so entity-only events with no player involvement
// produce no output. This is intentional: the central events.jsonl used to
// capture those, and it no longer exists.
//
// Rate limiting (global + per-domain) still applies before the record is
// appended, so a runaway domain can't flood disk.
class PlayZPartitionBuffer
{
	string m_Path;
	ref array<string> m_Lines;

	void PlayZPartitionBuffer(string path)
	{
		m_Path = path;
		m_Lines = new array<string>();
	}
}

class PlayZDomainCounter
{
	string m_Domain;
	int m_Count;

	void PlayZDomainCounter(string domain)
	{
		m_Domain = domain;
		m_Count = 0;
	}
}

class PlayZLogger
{
	static const string LOG_ROOT = "$profile:PlayZ/";
	static const string LOG_PLAYER_ROOT = "$profile:PlayZ/LogsByPlayer/";

	static ref PlayZLogConfig m_Config;
	static ref JsonSerializer m_Serializer = new JsonSerializer();

	static string m_SessionSeed = "";
	static bool m_IsShutdown = false;

	static ref array<ref PlayZDomainCounter> m_DomainCounter = new array<ref PlayZDomainCounter>();
	static ref array<ref PlayZPartitionBuffer> m_PartitionBuffers = new array<ref PlayZPartitionBuffer>();
	static int m_GlobalEventsInWindow = 0;
	static int m_WindowSecond = -1;
	static float m_FlushTimer = 0;
	static const float FLUSH_INTERVAL = 5.0; // Seconds between disk writes
	static const int MAX_BUFFER_SIZE = 50;   // Flush a partition if it hits this line count

	static ref map<string, int> s_DamageHitLastEmitMs;

	static void Init()
	{
		if (!FileExist(LOG_ROOT)) MakeDirectory(LOG_ROOT);
		if (!FileExist(LOG_PLAYER_ROOT)) MakeDirectory(LOG_PLAYER_ROOT);

		// 4-char hex session seed — keeps event IDs globally unique across restarts.
		m_SessionSeed = "";
		for (int i = 0; i < 4; i++)
		{
			m_SessionSeed += "0123456789ABCDEF".Substring(Math.RandomInt(0, 16), 1);
		}

		m_Config = new PlayZLogConfig();
		m_Config.Load();

		if (!s_DamageHitLastEmitMs)
			s_DamageHitLastEmitMs = new map<string, int>();
	}

	static void OnUpdate(float dt)
	{
		if (m_IsShutdown) return;

		m_FlushTimer += dt;
		if (m_FlushTimer >= FLUSH_INTERVAL)
		{
			FlushBuffer();
		}
	}

	static void FlushBuffer()
	{
		m_FlushTimer = 0;
		FlushPartitionBuffers();
	}

	static void FlushPartitionBuffers()
	{
		if (!m_PartitionBuffers || m_PartitionBuffers.Count() == 0) return;

		for (int i = 0; i < m_PartitionBuffers.Count(); i++)
		{
			PlayZPartitionBuffer pb = m_PartitionBuffers.Get(i);
			if (!pb || !pb.m_Lines || pb.m_Lines.Count() == 0) continue;

			FileHandle file = OpenFile(pb.m_Path, FileMode.APPEND);
			if (file != 0)
			{
				for (int j = 0; j < pb.m_Lines.Count(); j++)
				{
					FPrintln(file, pb.m_Lines.Get(j));
				}
				CloseFile(file);
			}
			pb.m_Lines.Clear();
		}
	}

	static void OnShutdown()
	{
		m_IsShutdown = true;
		FlushBuffer();
		FlushPartitionBuffers();
	}

	static void LogWeather(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogWeather)
			WriteLog("Weather", eventName, data);
	}

	static void LogPeople(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogPeople)
			WriteLog("People", eventName, data);
	}

	static void LogDamage(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogDamage)
			WriteLog("Damage", eventName, data);
	}

	static void TryLogDamageHit(EntityAI pSource, EntityAI pTarget, string dmgZone, string ammo, float damageValue)
	{
		if (!GetGame() || !GetGame().IsServer())
			return;
		if (!m_Config || !m_Config.m_EnableLogDamage)
			return;
		if (IsDamageHitThrottled(pSource, pTarget, dmgZone, ammo))
			return;
		if (!CanEmitForDomain("Damage"))
			return;

		PlayZLogData_Damage data = new PlayZLogData_Damage();
		if (m_Config.m_DamageHitMinimalPayload)
			data.InitMinimal(pSource, pTarget, dmgZone, ammo, damageValue);
		else
			data.Init(pSource, pTarget, dmgZone, ammo, damageValue);

		WriteLogImpl("Damage", "Hit", data, true);
	}

	// Key includes dmgZone + ammo so multi-layer hits from a single projectile
	// (vest -> top -> arm -> torso) are never collapsed into one log entry.
	// Only true duplicates on the same (source, target, zone, ammo) within
	// the configured window are deduped.
	static string BuildDamageThrottleKey(EntityAI pSource, EntityAI pTarget, string dmgZone, string ammo)
	{
		string sid = "";
		string tid = "";

		if (pSource)
		{
			Man sourceRootPlayer = pSource.GetHierarchyRootPlayer();
			if (sourceRootPlayer && sourceRootPlayer.GetIdentity())
				sid = sourceRootPlayer.GetIdentity().GetId();
		}

		if (pTarget)
		{
			Man targetRootPlayer = pTarget.GetHierarchyRootPlayer();
			if (targetRootPlayer && targetRootPlayer.GetIdentity())
				tid = targetRootPlayer.GetIdentity().GetId();
			else
			{
				EntityAI targetRootEntity = pTarget.GetHierarchyRoot();
				Man targetRootMan = Man.Cast(targetRootEntity);
				if (targetRootMan && targetRootMan.GetIdentity())
					tid = targetRootMan.GetIdentity().GetId();
			}
		}

		if (sid == "" && tid == "")
			return "";

		return sid + "|" + tid + "|" + dmgZone + "|" + ammo;
	}

	static bool IsDamageHitThrottled(EntityAI pSource, EntityAI pTarget, string dmgZone, string ammo)
	{
		if (!m_Config || m_Config.m_DamageHitMinIntervalMs <= 0)
			return false;
		if (!s_DamageHitLastEmitMs)
			s_DamageHitLastEmitMs = new map<string, int>();

		string key = BuildDamageThrottleKey(pSource, pTarget, dmgZone, ammo);
		if (key == "")
			return false;

		int now = GetGame().GetTime();
		int last = 0;
		if (s_DamageHitLastEmitMs.Find(key, last))
		{
			if (now - last < m_Config.m_DamageHitMinIntervalMs)
				return true;
		}

		return false;
	}

	static void RegisterDamageHitThrottleFromData(Managed data)
	{
		if (!m_Config || m_Config.m_DamageHitMinIntervalMs <= 0)
			return;
		if (!s_DamageHitLastEmitMs)
			s_DamageHitLastEmitMs = new map<string, int>();

		PlayZLogData_Damage dd = PlayZLogData_Damage.Cast(data);
		if (!dd)
			return;

		string actorId = dd.GetActorId();
		string targetId = dd.GetTargetId();
		if (actorId == "" && targetId == "")
			return;

		string key = actorId + "|" + targetId + "|" + dd.zone + "|" + dd.ammo;
		s_DamageHitLastEmitMs.Set(key, GetGame().GetTime());
	}

	static void LogBaseBuilding(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogBaseBuilding)
			WriteLog("BaseBuilding", eventName, data);
	}

	static void LogRecipe(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogRecipes)
			WriteLog("Recipes", eventName, data);
	}

	static void LogAction(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogActions)
			WriteLog("Actions", eventName, data);
	}

	static void LogAdmin(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogAdmin)
			WriteLog("Admin", eventName, data);
	}

	static void LogPlacement(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogPlacement)
			WriteLog("Placement", eventName, data);
	}

	static void LogAIInfected(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogAI)
			WriteLog("AIInfected", eventName, data);
	}

	static void LogAIAnimal(string eventName, Managed data)
	{
		if (m_Config && m_Config.m_EnableLogAI)
			WriteLog("AIAnimal", eventName, data);
	}

	static bool ShouldSkipAIHitWithoutPlayerSource(EntityAI source)
	{
		if (!m_Config || !m_Config.m_LogAIPlayerEngagementOnly)
			return false;
		if (!source)
			return true;
		if (source.GetHierarchyRootPlayer())
			return false;
		return true;
	}

	static bool ShouldSkipAIDeathWithoutPlayerKiller(Object killer)
	{
		if (!m_Config || !m_Config.m_LogAIPlayerEngagementOnly)
			return false;
		if (!killer)
			return false;
		EntityAI ke = EntityAI.Cast(killer);
		if (!ke)
			return true;
		if (ke.GetHierarchyRootPlayer())
			return false;
		return true;
	}

	static void WriteLog(string category, string eventName, Managed data)
	{
		WriteLogImpl(category, eventName, data, false);
	}

	static void WriteLogImpl(string category, string eventName, Managed data, bool rateLimitAlreadyApplied)
	{
		if (!rateLimitAlreadyApplied && !CanEmitForDomain(category))
			return;

		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);

		string timestamp = string.Format("%1-%2-%3T%4:%5:%6Z",
			year.ToStringLen(4), month.ToStringLen(2), day.ToStringLen(2),
			hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2));

		string dataJson = "{}";
		if (data)
		{
			PlayZLogData_Base logData = PlayZLogData_Base.Cast(data);
			if (logData)
			{
				dataJson = logData.ToJson();
			}
			else
			{
				if (!m_Serializer) m_Serializer = new JsonSerializer();
				m_Serializer.WriteToString(data, false, dataJson);
			}
		}

		WriteRecord(category, eventName, timestamp, dataJson, data);

		if (category == "Damage" && eventName == "Hit" && m_Config && m_Config.m_DamageHitMinIntervalMs > 0)
			RegisterDamageHitThrottleFromData(data);

		if (GetBufferedLineCount() >= MAX_BUFFER_SIZE)
		{
			FlushBuffer();
		}
	}

	static int GetBufferedLineCount()
	{
		int count = 0;
		if (m_PartitionBuffers)
		{
			for (int i = 0; i < m_PartitionBuffers.Count(); i++)
			{
				PlayZPartitionBuffer pb = m_PartitionBuffers.Get(i);
				if (pb && pb.m_Lines) count = count + pb.m_Lines.Count();
			}
		}
		return count;
	}

	static bool CanEmitForDomain(string category)
	{
		int secondNow = GetGame().GetTime() / 1000;
		if (secondNow != m_WindowSecond)
		{
			m_WindowSecond = secondNow;
			m_GlobalEventsInWindow = 0;
			if (!m_DomainCounter) m_DomainCounter = new array<ref PlayZDomainCounter>();
			for (int i = 0; i < m_DomainCounter.Count(); i++)
			{
				m_DomainCounter.Get(i).m_Count = 0;
			}
		}

		int globalMax = 300;
		if (m_Config) globalMax = m_Config.m_GlobalMaxEventsPerSecond;
		if (m_GlobalEventsInWindow >= globalMax) return false;

		PlayZDomainCounter c = GetOrCreateDomainCounter(category);
		int domainMax = GetDomainMaxPerSecond(category);
		if (c.m_Count >= domainMax) return false;

		m_GlobalEventsInWindow = m_GlobalEventsInWindow + 1;
		c.m_Count = c.m_Count + 1;
		return true;
	}

	static PlayZDomainCounter GetOrCreateDomainCounter(string category)
	{
		if (!m_DomainCounter) m_DomainCounter = new array<ref PlayZDomainCounter>();
		for (int i = 0; i < m_DomainCounter.Count(); i++)
		{
			PlayZDomainCounter c = m_DomainCounter.Get(i);
			if (c && c.m_Domain == category) return c;
		}

		PlayZDomainCounter created = new PlayZDomainCounter(category);
		m_DomainCounter.Insert(created);
		return created;
	}

	static int GetDomainMaxPerSecond(string category)
	{
		if (!m_Config) return 100;

		if (category == "People") return m_Config.m_MaxEventsPerSecondPeople;
		if (category == "Damage") return m_Config.m_MaxEventsPerSecondDamage;
		if (category == "Vehicle") return m_Config.m_MaxEventsPerSecondVehicle;
		if (category == "BaseBuilding") return m_Config.m_MaxEventsPerSecondBaseBuilding;
		if (category == "Recipes") return m_Config.m_MaxEventsPerSecondRecipes;
		if (category == "Actions") return m_Config.m_MaxEventsPerSecondActions;
		if (category == "Admin") return m_Config.m_MaxEventsPerSecondAdmin;
		if (category == "Placement") return m_Config.m_MaxEventsPerSecondPlacement;
		if (category == "Weather") return m_Config.m_MaxEventsPerSecondWeather;
		if (category == "AIInfected" || category == "AIAnimal") return m_Config.m_MaxEventsPerSecondAI;
		return 100;
	}

	static void WriteRecord(string category, string eventName, string timestamp, string dataJson, Managed data)
	{
		string subCategory = ResolveSubCategory(category, eventName);
		string eventCode = ResolveEventCode(eventName);
		string actorId = ExtractActorId(data);
		string targetId = ExtractTargetId(data);
		string eventId = BuildEventId();

		WritePlayerCentricRecords(eventName, timestamp, eventId, category, subCategory, eventCode, actorId, targetId, dataJson);
	}

	static void WritePlayerCentricRecords(string eventName, string timestamp, string eventId, string category, string subCategory, string eventCode, string actorId, string targetId, string dataJson)
	{
		if (!m_Config || !m_Config.m_EnablePlayerCentricLogs) return;

		if (category == "Damage" && eventName == "Hit" && m_Config.m_DamageHitOmitPlayerCentricRecords)
			return;

		if (IsPlayerId(actorId))
		{
			string actorPath = ResolvePlayerCentricPath(actorId);
			AppendPartitionRecord(actorPath, string.Format("{\"timestamp\":\"%1\",\"sessionId\":\"%2\",\"eventId\":\"%3\",\"category\":\"%4\",\"subCategory\":\"%5\",\"eventCode\":\"%6\",\"source\":\"server\",\"perspective\":\"actor\",\"actorId\":\"%7\",\"targetId\":\"%8\",\"data\":%9}",
				timestamp,
				PlayZLogData_Base.EscapeJson(m_SessionSeed),
				PlayZLogData_Base.EscapeJson(eventId),
				PlayZLogData_Base.EscapeJson(category),
				PlayZLogData_Base.EscapeJson(subCategory),
				PlayZLogData_Base.EscapeJson(eventCode),
				PlayZLogData_Base.EscapeJson(actorId),
				PlayZLogData_Base.EscapeJson(targetId),
				dataJson));
		}

		if (!m_Config.m_EnablePlayerCentricMirrorTarget) return;
		if (!IsPlayerId(targetId)) return;
		if (targetId == actorId) return;

		string targetPath = ResolvePlayerCentricPath(targetId);
		AppendPartitionRecord(targetPath, string.Format("{\"timestamp\":\"%1\",\"sessionId\":\"%2\",\"eventId\":\"%3\",\"category\":\"%4\",\"subCategory\":\"%5\",\"eventCode\":\"%6\",\"source\":\"server\",\"perspective\":\"target\",\"actorId\":\"%7\",\"targetId\":\"%8\",\"data\":%9}",
			timestamp,
			PlayZLogData_Base.EscapeJson(m_SessionSeed),
			PlayZLogData_Base.EscapeJson(eventId),
			PlayZLogData_Base.EscapeJson(category),
			PlayZLogData_Base.EscapeJson(subCategory),
			PlayZLogData_Base.EscapeJson(eventCode),
			PlayZLogData_Base.EscapeJson(actorId),
			PlayZLogData_Base.EscapeJson(targetId),
			dataJson));
	}

	static string BuildEventId()
	{
		int ms = GetGame().GetTime();
		int randomSeed = Math.RandomInt(0, 100000);
		return string.Format("%1-%2-%3", m_SessionSeed, ms, randomSeed);
	}

	static string BuildDateFolderUTC()
	{
		int year, month, day;
		GetYearMonthDayUTC(year, month, day);
		return string.Format("%1-%2-%3", year.ToStringLen(4), month.ToStringLen(2), day.ToStringLen(2));
	}

	static string BuildHourFolderUTC()
	{
		int hour, minute, second;
		GetHourMinuteSecondUTC(hour, minute, second);
		return hour.ToStringLen(2);
	}

	static void EnsureDirectory(string path)
	{
		if (!FileExist(path))
		{
			MakeDirectory(path);
		}
	}

	static string ResolvePlayerCentricPath(string playerId)
	{
		string dateFolder = BuildDateFolderUTC();
		string baseDir = LOG_PLAYER_ROOT + dateFolder + "/";
		EnsureDirectory(baseDir);

		if (m_Config && m_Config.m_EnableHourlyPartitions)
		{
			string hourFolder = BuildHourFolderUTC();
			baseDir = baseDir + hourFolder + "/";
			EnsureDirectory(baseDir);
		}

		return baseDir + "player_" + playerId + ".jsonl";
	}

	static bool IsPlayerId(string id)
	{
		if (id == "") return false;
		if (id.Length() > 2 && id.Substring(0, 2) == "Z-") return false;
		if (id.Length() > 2 && id.Substring(0, 2) == "A-") return false;
		if (id.Length() > 2 && id.Substring(0, 2) == "V-") return false;
		if (id.Length() > 2 && id.Substring(0, 2) == "P-") return false;
		return true;
	}

	static void AppendPartitionRecord(string path, string line)
	{
		PlayZPartitionBuffer pb = GetOrCreatePartitionBuffer(path);
		if (!pb || !pb.m_Lines) return;

		pb.m_Lines.Insert(line);
		if (pb.m_Lines.Count() >= MAX_BUFFER_SIZE)
		{
			FileHandle file = OpenFile(pb.m_Path, FileMode.APPEND);
			if (file != 0)
			{
				for (int i = 0; i < pb.m_Lines.Count(); i++)
				{
					FPrintln(file, pb.m_Lines.Get(i));
				}
				CloseFile(file);
			}
			pb.m_Lines.Clear();
		}
	}

	static PlayZPartitionBuffer GetOrCreatePartitionBuffer(string path)
	{
		if (!m_PartitionBuffers) m_PartitionBuffers = new array<ref PlayZPartitionBuffer>();

		for (int i = 0; i < m_PartitionBuffers.Count(); i++)
		{
			PlayZPartitionBuffer pb = m_PartitionBuffers.Get(i);
			if (pb && pb.m_Path == path)
				return pb;
		}

		PlayZPartitionBuffer created = new PlayZPartitionBuffer(path);
		m_PartitionBuffers.Insert(created);
		return created;
	}

	static string ResolveSubCategory(string category, string eventName)
	{
		if (category == "People")
		{
			if (eventName == "StatsSnapshot") return "Movement";
			if (eventName == "PlayerConnected" || eventName == "PlayerDisconnected" || eventName == "Death") return "Lifecycle";
			return "Combat";
		}
		if (category == "Actions") return "Interaction";
		if (category == "Recipes") return "Crafting";
		if (category == "Damage") return "Entity";
		if (category == "Vehicle") return "State";
		if (category == "BaseBuilding") return "State";
		if (category == "Admin") return "Moderation";
		if (category == "Placement") return "Placement";
		if (category == "Weather") return "State";
		if (category == "AIInfected") return "Infected";
		if (category == "AIAnimal") return "Animal";
		return "General";
	}

	static string ResolveEventCode(string eventName)
	{
		string code = eventName;
		if (eventName == "StatsSnapshot") code = "snapshot";
		else if (eventName == "WeatherSnapshot") code = "snapshot";
		else if (eventName == "Hit") code = "hit";
		else if (eventName == "Death") code = "death";
		else if (eventName == "Destroyed") code = "destroyed";
		else if (eventName == "Built") code = "built";
		else if (eventName == "Dismantled") code = "dismantled";
		else if (eventName == "Attached") code = "attached";
		else if (eventName == "Detached") code = "detached";
		else if (eventName == "Opened") code = "opened";
		else if (eventName == "Closed") code = "closed";
		else if (eventName == "Spawned") code = "spawned";
		else if (eventName == "Deleted") code = "deleted";
		else if (eventName == "PlayerConnected") code = "connect";
		else if (eventName == "PlayerDisconnected") code = "disconnect";
		else if (eventName == "UnconsciousStart") code = "unconsciousStart";
		else if (eventName == "UnconsciousStop") code = "unconsciousStop";
		else if (eventName == "DriverEnter") code = "driverEnter";
		else if (eventName == "DriverExit") code = "driverExit";
		else if (eventName == "RecipePerformed") code = "perform";
		else if (eventName == "PositionSnapshot") code = "snapshot";
		else if (eventName == "ActionStart") code = "start";
		else if (eventName == "PlayerTeleported") code = "teleport";

		return code;
	}

	static string ExtractActorId(Managed data)
	{
		PlayZLogData_Base baseData = PlayZLogData_Base.Cast(data);
		if (baseData)
		{
			string actor = baseData.GetActorId();
			if (actor != "") return actor;
		}

		PlayZLogData_Entity entityData = PlayZLogData_Entity.Cast(data);
		if (entityData && entityData.m_Id != "") return entityData.m_Id;

		PlayZLogData_Damage damageData = PlayZLogData_Damage.Cast(data);
		if (damageData && damageData.source && damageData.source.m_Id != "") return damageData.source.m_Id;

		return "";
	}

	static string ExtractTargetId(Managed data)
	{
		PlayZLogData_Base baseData = PlayZLogData_Base.Cast(data);
		if (baseData)
		{
			string target = baseData.GetTargetId();
			if (target != "") return target;
		}

		PlayZLogData_Damage damageData = PlayZLogData_Damage.Cast(data);
		if (damageData && damageData.target && damageData.target.m_Id != "") return damageData.target.m_Id;

		return "";
	}
}
