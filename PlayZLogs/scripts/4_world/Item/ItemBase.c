modded class ItemBase
{
	override void OnItemLocationChanged(EntityAI old_owner, EntityAI new_owner)
	{
		super.OnItemLocationChanged(old_owner, new_owner);

		if (GetGame().IsServer())
		{
			PlayZLogData_ItemMove moveData = new PlayZLogData_ItemMove();
			moveData.InitItemMove(this, old_owner, new_owner);
			PlayZLogger.LogPlacement("ItemLocationChanged", moveData);
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
			
			// Log to BaseBuilding if it's a base building item
			if (this.IsInherited(BaseBuildingBase))
			{
				PlayZLogger.LogBaseBuilding("Destroyed", data);
			}
			else
			{
				PlayZLogger.LogDamage("Destroyed", data);
			}
		}
	}
}
