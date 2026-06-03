modded class PlayerBase
{
	protected ref Timer m_PlayZStatsTimer;
	protected string m_PlayZPersistentId = "";
	protected string m_PlayZPersistentName = "";

	void PlayZSetPersistentIdentity(PlayerIdentity identity)
	{
		if (!identity) return;
		m_PlayZPersistentId = identity.GetId();
		m_PlayZPersistentName = identity.GetName();
	}

	string PlayZGetPersistentId()
	{
		return m_PlayZPersistentId;
	}

	string PlayZGetPersistentName()
	{
		return m_PlayZPersistentName;
	}

	override void OnPlayerLoaded()
	{
		super.OnPlayerLoaded();
		
		if (GetGame().IsServer())
		{
			int interval = 60;
			if (PlayZLogger.m_Config) interval = PlayZLogger.m_Config.m_PlayerSnapshotInterval;

			m_PlayZStatsTimer = new Timer(CALL_CATEGORY_SYSTEM);
			m_PlayZStatsTimer.Run(interval, this, "PlayZLogStats", null, true);

			GetOnItemAddedIntoCargo().Remove(PlayZOnItemAddedIntoCargo);
			GetOnItemRemovedFromCargo().Remove(PlayZOnItemRemovedFromCargo);
			GetOnItemMovedInCargo().Remove(PlayZOnItemMovedInCargo);
			GetOnItemAddedIntoCargo().Insert(PlayZOnItemAddedIntoCargo);
			GetOnItemRemovedFromCargo().Insert(PlayZOnItemRemovedFromCargo);
			GetOnItemMovedInCargo().Insert(PlayZOnItemMovedInCargo);
		}
	}

	void PlayZOnItemAddedIntoCargo(EntityAI item, EntityAI owner)
	{
		if (!GetGame().IsServer() || !item) return;
		PlayZLogData_ItemMove data = new PlayZLogData_ItemMove();
		data.InitCargoMove(item, owner, "cargoAdded");
		PlayZLogger.LogPlacement("ItemCargoAdded", data);
	}

	void PlayZOnItemRemovedFromCargo(EntityAI item, EntityAI owner)
	{
		if (!GetGame().IsServer() || !item) return;
		PlayZLogData_ItemMove data = new PlayZLogData_ItemMove();
		data.InitCargoMove(item, owner, "cargoRemoved");
		PlayZLogger.LogPlacement("ItemCargoRemoved", data);
	}

	void PlayZOnItemMovedInCargo(EntityAI item, EntityAI owner)
	{
		if (!GetGame().IsServer() || !item) return;
		PlayZLogData_ItemMove data = new PlayZLogData_ItemMove();
		data.InitCargoMove(item, owner, "cargoMoved");
		PlayZLogger.LogPlacement("ItemCargoMoved", data);
	}

	void PlayZLogStats()
	{
		if (GetGame().IsServer() && IsAlive())
		{
			PlayZLogData_PlayerStats data = new PlayZLogData_PlayerStats();
			data.InitPlayerStats(this);
			PlayZLogger.LogPeople("StatsSnapshot", data);
		}
	}

	override void OnUnconsciousStart()
	{
		super.OnUnconsciousStart();
		
		if (GetGame().IsServer())
		{
			PlayZLogData_Player data = new PlayZLogData_Player();
			data.InitPlayer(this);
			PlayZLogger.LogPeople("UnconsciousStart", data);
		}
	}

	override void OnUnconsciousStop(int pCurrentCommandID)
	{
		super.OnUnconsciousStop(pCurrentCommandID);
		
		if (GetGame().IsServer())
		{
			PlayZLogData_Player data = new PlayZLogData_Player();
			data.InitPlayer(this);
			PlayZLogger.LogPeople("UnconsciousStop", data);
		}
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		
		if (GetGame().IsServer())
		{
			float damageValue = 0;
			if (damageResult)
				damageValue = damageResult.GetHighestDamage("Health");

			PlayZLogger.TryLogDamageHit(source, this, dmgZone, ammo, damageValue);
		}
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		
		if (GetGame().IsServer())
		{
			PlayZLogData_Entity data = new PlayZLogData_Entity();
			data.Init(this);
			PlayZLogger.LogPeople("Death", data);
		}
	}
}
