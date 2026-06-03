//! Omitted when GAMELABS is not defined (client / non-GameLabs script builds).
#ifdef GAMELABS
modded class PlayerBase
{
	private PlayerBase PlayZCF_ResolveRootPlayer(Object sourceObject)
	{
		EntityAI sourceEntity;
		PlayerBase resolvedPlayer;

		if (!sourceObject) return NULL;

		resolvedPlayer = PlayerBase.Cast(sourceObject);
		if (resolvedPlayer) return resolvedPlayer;

		sourceEntity = EntityAI.Cast(sourceObject);
		if (!sourceEntity) return NULL;

		return PlayerBase.Cast(sourceEntity.GetHierarchyRootPlayer());
	}

	private EntityAI PlayZCF_AsEntity(Object sourceObject)
	{
		return EntityAI.Cast(sourceObject);
	}

	private bool PlayZCF_IsSpawnMenuSentinelPosition(vector position)
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

	override void GLProcessKill(Object killer)
	{
		vector playZcfDeathPos;
		bool playZcfSpawnMenuSentinel;
		string cftoolsId;
		_Payload_PlayerDeath payload;
		_LogPlayerEx logObjectMurderer;
		_LogPlayerEx logObjectPlayer;
		EntityAI weapon;
		EntityAI killerEntity;
		EntityAI lastDamageEntity;
		PlayerBase murderer;

		cftoolsId = GetGameLabs().GetPlayerUpstreamIdentity(this.GetPlainId());
		if (cftoolsId)
		{
			GLPlayerStatistics playerStats = GetGameLabs().GetPlayerStatisticsByCFToolsId(cftoolsId);
			if (this.StatGet("dist") >= playerStats.startingDistance)
			{
				playerStats.distance += (this.StatGet("dist") - playerStats.startingDistance);
			}
		}

		logObjectPlayer = new _LogPlayerEx(this);

		killerEntity = PlayZCF_AsEntity(killer);
		lastDamageEntity = this.gl_lastDamagingEntity;
		murderer = NULL;

		if (killer && (killer.IsWeapon() || killer.IsMeleeWeapon()))
		{
			weapon = PlayZCF_AsEntity(killer);
		}

		// Prefer root-player ownership so projectile/attachment parents are resolved correctly.
		murderer = PlayZCF_ResolveRootPlayer(killer);
		if (!murderer && weapon)
		{
			murderer = PlayerBase.Cast(weapon.GetHierarchyRootPlayer());
		}
		if (!murderer && lastDamageEntity)
		{
			murderer = PlayerBase.Cast(lastDamageEntity.GetHierarchyRootPlayer());
		}

		playZcfDeathPos = GetPosition();
		playZcfSpawnMenuSentinel = PlayZCF_IsSpawnMenuSentinelPosition(playZcfDeathPos);

		if (!playZcfSpawnMenuSentinel)
		{
			GetGameLabs().GetLogger().Debug(string.Format("PlayZCF.EEKilled(this=%1, killer=%2, weapon=%3, murderer=%4, lastDamageType=%5, lastDamageAmmo=%6, lastDamagingEntity=%7)", this, killer, weapon, murderer, this.gl_lastDamageType, this.gl_lastDamageAmmo, this.gl_lastDamagingEntity));
		}

		if (murderer && murderer != this)
		{
			string killType = "";
			string killDisplay = "";

			if (weapon)
			{
				killType = weapon.GetType();
				killDisplay = weapon.GetDisplayName();
			}
			else if (lastDamageEntity)
			{
				killType = lastDamageEntity.GetType();
				killDisplay = lastDamageEntity.GetDisplayName();
			}
			else if (killer)
			{
				killType = killer.GetType();
				killDisplay = killer.GetDisplayName();
			}

			logObjectMurderer = new _LogPlayerEx(murderer);
			payload = new _Payload_PlayerDeath(logObjectPlayer, logObjectMurderer, killType, killDisplay);
		}
		else if (killer && killer.IsInherited(ZombieBase))
		{
			payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Infected", killer.GetType());
		}
		else if (killer && killer.IsInherited(AnimalBase))
		{
			payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Animal", killer.GetDisplayName());
		}
		else if (this == killer)
		{
			if (weapon)
			{
				payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, killer.GetType(), killer.GetDisplayName());
			}
			else if (this.CommitedSuicide() || this.CommitedSuicide())
			{
				weapon = this.GetItemInHands();
				if (weapon) payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Suicide", weapon.GetType());
				else payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Suicide", "");
			}
			else
			{
				string refType = "";
				if (lastDamageEntity) refType = lastDamageEntity.GetType();

				// Preserve infected/animal attribution from delayed fatal ticks.
				if (lastDamageEntity && lastDamageEntity.IsInherited(ZombieBase))
				{
					payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Infected", refType);
				}
				else if (lastDamageEntity && lastDamageEntity.IsInherited(AnimalBase))
				{
					payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Animal", refType);
				}
				else
				{
					switch (this.gl_lastDamageType)
					{
						case DT_EXPLOSION:
						{
							payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Explosion", refType);
							break;
						}
						case DT_CUSTOM:
						{
							if (this.gl_lastDamageAmmo == "FallDamage") payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__FallDamage", refType);
							else payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Environment", refType);
							break;
						}
						default:
						{
							payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Environment", refType);
							break;
						}
					}
				}
			}
		}
		else if (lastDamageEntity && lastDamageEntity.IsInherited(ZombieBase))
		{
			payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Infected", lastDamageEntity.GetType());
		}
		else if (lastDamageEntity && lastDamageEntity.IsInherited(AnimalBase))
		{
			payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Animal", lastDamageEntity.GetType());
		}
		else if (killer)
		{
			payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Object", killer.GetType());
		}
		else
		{
			payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Environment", "");
		}

		if (playZcfSpawnMenuSentinel)
			return;

		GetGameLabs().GetApi().PlayerDeath(new _Callback(), payload);
	}
};
#endif
