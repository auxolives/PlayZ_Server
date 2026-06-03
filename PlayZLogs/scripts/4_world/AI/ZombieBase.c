modded class ZombieBase
{
	string m_PlayZUID;

	void ZombieBase()
	{
		if (GetGame().IsServer())
			m_PlayZUID = PlayZEntityIDProvider.NextInfectedID();
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		
		if (GetGame().IsServer())
		{
			if (ammo != "" && ammo.IndexOf("Dummy_") != -1) return;
			if (GetHealth("", "Health") <= 0) return;
			if (PlayZLogger.ShouldSkipAIHitWithoutPlayerSource(source))
				return;

			float damageValue = 0;
			if (damageResult)
				damageValue = damageResult.GetHighestDamage("Health");
			
			PlayZLogData_Damage data = new PlayZLogData_Damage();
			data.Init(source, this, dmgZone, ammo, damageValue);
			if (data.target) data.target.m_Id = m_PlayZUID;
			PlayZLogger.LogAIInfected("Hit", data);
		}
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		
		if (GetGame().IsServer())
		{
			if (PlayZLogger.ShouldSkipAIDeathWithoutPlayerKiller(killer))
				return;

			PlayZLogData_Infected data = new PlayZLogData_Infected();
			data.InitInfected(this);
			PlayZLogger.LogAIInfected("Death", data);
		}
	}
}
