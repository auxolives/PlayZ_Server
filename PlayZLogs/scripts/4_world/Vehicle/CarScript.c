modded class CarScript
{
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
}
