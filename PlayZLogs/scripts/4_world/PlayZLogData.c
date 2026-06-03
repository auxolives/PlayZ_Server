class PlayZLogData_Player : PlayZLogData_Entity
{
	string id;
	string name;
	
	void InitPlayer(Managed srcObj)
	{
		Init(srcObj);
		PlayerBase player = PlayerBase.Cast(srcObj);
		if (player && player.GetIdentity())
		{
			id = player.GetIdentity().GetId();
			name = player.GetIdentity().GetName();
		}
		else if (player)
		{
			id = player.PlayZGetPersistentId();
			name = player.PlayZGetPersistentName();
		}
	}

	void InitPlayerWithIdentity(Managed srcObj, PlayerIdentity identity)
	{
		InitPlayer(srcObj);
		if (identity)
		{
			id = identity.GetId();
			name = identity.GetName();
		}
	}

	override string ToJson()
	{
		string posJson = "null";
		if (pos) posJson = pos.ToJson();
		string json = string.Format("{\"type\":\"%1\",\"pos\":%2,\"health\":%3,\"id\":\"%4\",\"name\":\"%5\"}", type, posJson, health.ToString(), id, name);

		return json;
	}

	override string GetActorId()
	{
		if (id != "") return id;
		return super.GetActorId();
	}

	override string GetTrackId()
	{
		if (id != "") return "P-" + id;
		return "";
	}
}

class PlayZLogData_Infected : PlayZLogData_Entity
{
	void InitInfected(Managed srcObj)
	{
		Init(srcObj);
		ZombieBase zombie = ZombieBase.Cast(srcObj);
		if (zombie && zombie.m_PlayZUID != "")
			m_Id = zombie.m_PlayZUID;
	}
}

class PlayZLogData_Animal : PlayZLogData_Entity
{
	void InitAnimal(Managed srcObj)
	{
		Init(srcObj);
		AnimalBase animal = AnimalBase.Cast(srcObj);
		if (animal && animal.m_PlayZUID != "")
			m_Id = animal.m_PlayZUID;
	}
}

class PlayZLogData_Item : PlayZLogData_Entity
{
	void InitItem(Managed srcObj)
	{
		Init(srcObj);
	}
}

class PlayZLogData_Vehicle : PlayZLogData_Entity
{
	float speed;
	float fuel;
	
	void InitVehicle(Managed srcObj)
	{
		Init(srcObj);

		Transport transport = Transport.Cast(srcObj);
		if (transport)
		{
			PlayZVehicleLogState state = PlayZVehicleRegistry.GetOrCreateState(transport);
			if (state && state.m_UID != "")
				m_Id = state.m_UID;
		}
		
		Car car = Car.Cast(srcObj);
		if (car)
			speed = car.GetSpeedometer();
		else
		{
			EntityAI ent = EntityAI.Cast(srcObj);
			if (ent) speed = GetVelocity(ent).Length() * 3.6;
		}

		if (car)
		{
			fuel = car.GetFluidFraction(CarFluid.FUEL);
		}
	}
}

class PlayZLogData_Firearm : PlayZLogData_Entity
{
	bool isJammed;
	int muzzleCount;
	
	void InitFirearm(Managed srcObj)
	{
		Init(srcObj);
		Weapon_Base weapon = Weapon_Base.Cast(srcObj);
		if (weapon)
		{
			isJammed = weapon.IsJammed();
			muzzleCount = weapon.GetMuzzleCount();
		}
	}
}

class PlayZLogData_Garden : PlayZLogData_Entity
{
	int slotCount;
	
	void InitGarden(Managed srcObj)
	{
		Init(srcObj);
		GardenBase garden = GardenBase.Cast(srcObj);
		if (garden)
		{
			slotCount = garden.GetGardenSlotsCount();
		}
	}
}

class PlayZLogData_Explosive : PlayZLogData_Entity
{
	bool armed;
	bool defused;
	
	void InitExplosive(Managed srcObj)
	{
		Init(srcObj);
		ExplosivesBase explosive = ExplosivesBase.Cast(srcObj);
		if (explosive)
		{
			armed = explosive.GetArmed();
			defused = explosive.GetDefused();
		}
	}
}

class PlayZLogData_BaseBuilding : PlayZLogData_Entity
{
	string part;
	ref PlayZLogData_Player player;
	
	void InitBaseBuilding(Managed srcObj, string pPart, Managed pPlayer)
	{
		Init(srcObj);
		part = pPart;
		if (pPlayer)
		{
			player = new PlayZLogData_Player();
			player.InitPlayer(pPlayer);
		}
	}
	
	override string ToJson()
	{
		string entityJson = super.ToJson();
		if (entityJson.Length() > 0)
			entityJson = entityJson.Substring(0, entityJson.Length() - 1);
		else
			entityJson = "{";
		
		string playerJson = "null";
		if (player) playerJson = player.ToJson();
		
		return string.Format("%1, \"part\":\"%2\", \"player\":%3}", entityJson, part, playerJson);
	}

	override string GetActorId()
	{
		if (player) return player.GetActorId();
		return super.GetActorId();
	}

	override string GetTargetId()
	{
		return super.GetActorId();
	}
}

class PlayZLogData_PlayerStats : PlayZLogData_Player
{
	float blood;
	float water;
	float energy;
	int bleedingSources;
	bool isBleeding;
	float sleepValue;
	float sanityValue;
	int hematomasCount;
	int bulletWounds;
	int stubWounds;
	float contusionValue;
	bool hasViscera;
	int sepsisLevel;
	int influenzaLevel;
	int poisonLevel;
	int biohazardLevel;
	int rabiesLevel;
	
	void InitPlayerStats(Managed srcObj)
	{
		InitPlayer(srcObj);
		sleepValue = -1;
		sanityValue = -1;
		hematomasCount = -1;
		bulletWounds = -1;
		stubWounds = -1;
		contusionValue = -1;
		hasViscera = false;
		sepsisLevel = -1;
		influenzaLevel = -1;
		poisonLevel = -1;
		biohazardLevel = -1;
		rabiesLevel = -1;

		PlayerBase player = PlayerBase.Cast(srcObj);
		if (player)
		{
			blood = player.GetHealth("GlobalHealth", "Blood");
			
			if (player.GetStatWater()) water = player.GetStatWater().Get();
			if (player.GetStatEnergy()) energy = player.GetStatEnergy().Get();

			bleedingSources = player.GetBleedingSourceCount();
			isBleeding = player.IsBleeding();

			if (player.GetTerjeStats())
			{
				sleepValue = player.GetTerjeStats().GetSleepingValue();
				sanityValue = player.GetTerjeStats().GetMindValue();

				hematomasCount = player.GetTerjeStats().GetHematomasCount();
				bulletWounds = player.GetTerjeStats().GetBulletWounds();
				stubWounds = player.GetTerjeStats().GetStubWounds();
				contusionValue = player.GetTerjeStats().GetContusionValue();
				hasViscera = player.GetTerjeStats().GetViscera();

				sepsisLevel = player.GetTerjeStats().GetSepsisLevel();
				influenzaLevel = player.GetTerjeStats().GetInfluenzaLevel();
				poisonLevel = player.GetTerjeStats().GetPoisonLevel();
				biohazardLevel = player.GetTerjeStats().GetBiohazardLevel();
				rabiesLevel = player.GetTerjeStats().GetRabiesLevel();
			}
		}
	}
	
	override string ToJson()
	{
		string playerJson = super.ToJson();
		if (playerJson.Length() > 0)
			playerJson = playerJson.Substring(0, playerJson.Length() - 1);
		else
			playerJson = "{";
		string json = playerJson;
		json += string.Format(",\"blood\":%1,\"water\":%2,\"energy\":%3", blood.ToString(), water.ToString(), energy.ToString());
		json += string.Format(",\"bleedingSources\":%1,\"isBleeding\":%2", bleedingSources.ToString(), isBleeding.ToString());
		json += string.Format(",\"terje\":{\"sleepValue\":%1,\"sanityValue\":%2", sleepValue.ToString(), sanityValue.ToString());
		json += string.Format(",\"wounds\":{\"hematomas\":%1,\"bulletWounds\":%2,\"stubWounds\":%3,\"contusionValue\":%4,\"viscera\":%5}",
			hematomasCount.ToString(), bulletWounds.ToString(), stubWounds.ToString(), contusionValue.ToString(), hasViscera.ToString());
		json += string.Format(",\"diseases\":{\"sepsisLevel\":%1,\"influenzaLevel\":%2,\"poisonLevel\":%3,\"biohazardLevel\":%4,\"rabiesLevel\":%5}}}",
			sepsisLevel.ToString(), influenzaLevel.ToString(), poisonLevel.ToString(), biohazardLevel.ToString(), rabiesLevel.ToString());
		return json;
	}
}

class PlayZLogData_Recipe : PlayZLogData_Base
{
	string recipe;
	ref PlayZLogData_Player player;
	ref array<ref PlayZLogData_Entity> ingredients;
	ref array<ref PlayZLogData_Entity> results;
	
	void InitRecipe(Managed pRecipeObj, Managed pPlayer, array<ItemBase> pIngredientsSorted, array<ItemBase> pResults)
	{
		RecipeBase recipe_obj = RecipeBase.Cast(pRecipeObj);
		if (recipe_obj) recipe = recipe_obj.GetName();
		player = new PlayZLogData_Player();
		player.InitPlayer(pPlayer);
		
		ingredients = new array<ref PlayZLogData_Entity>;
		if (pIngredientsSorted)
		{
			foreach (ItemBase ing : pIngredientsSorted)
			{
				if (ing)
				{
					PlayZLogData_Entity entIng = new PlayZLogData_Entity();
					entIng.Init(ing);
					ingredients.Insert(entIng);
				}
			}
		}
		
		results = new array<ref PlayZLogData_Entity>;
		if (pResults)
		{
			foreach (ItemBase res : pResults)
			{
				if (res)
				{
					PlayZLogData_Entity entRes = new PlayZLogData_Entity();
					entRes.Init(res);
					results.Insert(entRes);
				}
			}
		}
	}

	override string ToJson()
	{
		string playerJson = "null";
		if (player) playerJson = player.ToJson();
		
		string ingredientsJson = "[";
		if (ingredients)
		{
			for (int i = 0; i < ingredients.Count(); i++)
			{
				ingredientsJson += ingredients.Get(i).ToJson();
				if (i < ingredients.Count() - 1) ingredientsJson += ",";
			}
		}
		ingredientsJson += "]";

		string resultsJson = "[";
		if (results)
		{
			for (int j = 0; j < results.Count(); j++)
			{
				resultsJson += results.Get(j).ToJson();
				if (j < results.Count() - 1) resultsJson += ",";
			}
		}
		resultsJson += "]";
		
		string json = string.Format("{\"recipe\":\"%1\",\"player\":%2,\"ingredients\":%3,\"results\":%4}", recipe, playerJson, ingredientsJson, resultsJson);

		return json;
	}

	override string GetActorId()
	{
		if (player) return player.GetActorId();
		return "";
	}
}

class PlayZLogData_Action : PlayZLogData_Base
{
	string action;
	ref PlayZLogData_Player player;
	ref PlayZLogData_Entity target;
	ref PlayZLogData_Entity item;
	
	void InitAction(Managed pActionData)
	{
		ActionData action_data = ActionData.Cast(pActionData);
		if (action_data)
		{
			action = action_data.m_Action.ClassName();
			player = new PlayZLogData_Player();
			player.InitPlayer(action_data.m_Player);
			
			if (action_data.m_Target && action_data.m_Target.GetObject())
			{
				target = new PlayZLogData_Entity();
				target.Init(action_data.m_Target.GetObject());
			}
			
			if (action_data.m_MainItem)
			{
				item = new PlayZLogData_Entity();
				item.Init(action_data.m_MainItem);
			}
		}
	}

	override string ToJson()
	{
		string playerJson = "null";
		if (player) playerJson = player.ToJson();
		string targetJson = "null";
		if (target) targetJson = target.ToJson();
		string itemJson = "null";
		if (item) itemJson = item.ToJson();
		return string.Format("{\"action\":\"%1\",\"player\":%2,\"target\":%3,\"item\":%4}", action, playerJson, targetJson, itemJson);
	}

	override string GetActorId()
	{
		if (player) return player.GetActorId();
		return "";
	}

	override string GetTargetId()
	{
		if (target) return target.GetActorId();
		return "";
	}
}

class PlayZLogData_PlayerEvent : PlayZLogData_Player
{
	string eventName;
	string reason;
	
	void InitPlayerEvent(Managed srcObj, string pEvent, string pReason = "")
	{
		InitPlayer(srcObj);
		eventName = pEvent;
		reason = pReason;
	}
	
	override string ToJson()
	{
		string playerJson = super.ToJson();
		if (playerJson.Length() > 0)
			playerJson = playerJson.Substring(0, playerJson.Length() - 1);
		else
			playerJson = "{";
		return string.Format("%1,\"eventName\":\"%2\",\"reason\":\"%3\"}", playerJson, eventName, reason);
	}
}

class PlayZLogData_Placement : PlayZLogData_Base
{
	ref PlayZLogData_Player player;
	ref PlayZLogData_Entity item;
	
	void InitPlacement(Managed pPlayer, Managed pItem)
	{
		player = new PlayZLogData_Player();
		player.InitPlayer(pPlayer);
		item = new PlayZLogData_Entity();
		item.Init(pItem);
	}
	
	override string ToJson()
	{
		string playerJson = "null";
		if (player) playerJson = player.ToJson();
		string itemJson = "null";
		if (item) itemJson = item.ToJson();
		return string.Format("{\"player\":%1,\"item\":%2}", playerJson, itemJson);
	}
}

class PlayZLogData_ItemMove : PlayZLogData_Base
{
	ref PlayZLogData_Entity item;
	ref PlayZLogData_Entity oldOwner;
	ref PlayZLogData_Entity newOwner;
	ref PlayZLogData_Player oldRootPlayer;
	ref PlayZLogData_Player newRootPlayer;
	string fromLocation = "unknown";
	string toLocation = "unknown";
	string moveReason = "locationChanged";

	void InitItemMove(Managed pItem, Managed pOldOwner, Managed pNewOwner)
	{
		item = new PlayZLogData_Entity();
		item.Init(pItem);

		if (!pOldOwner) fromLocation = "ground";
		else fromLocation = "entity";

		if (!pNewOwner) toLocation = "ground";
		else toLocation = "entity";

		if (pOldOwner)
		{
			oldOwner = new PlayZLogData_Entity();
			oldOwner.Init(pOldOwner);
		}

		if (pNewOwner)
		{
			newOwner = new PlayZLogData_Entity();
			newOwner.Init(pNewOwner);
		}

		EntityAI oldOwnerEnt = EntityAI.Cast(pOldOwner);
		if (oldOwnerEnt)
		{
			Man oldRoot = oldOwnerEnt.GetHierarchyRootPlayer();
			if (oldRoot)
			{
				oldRootPlayer = new PlayZLogData_Player();
				oldRootPlayer.InitPlayer(oldRoot);
			}
		}

		EntityAI newOwnerEnt = EntityAI.Cast(pNewOwner);
		if (newOwnerEnt)
		{
			Man newRoot = newOwnerEnt.GetHierarchyRootPlayer();
			if (newRoot)
			{
				newRootPlayer = new PlayZLogData_Player();
				newRootPlayer.InitPlayer(newRoot);
			}
		}
	}

	void InitCargoMove(Managed pItem, Managed pOwner, string reason)
	{
		moveReason = reason;
		item = new PlayZLogData_Entity();
		item.Init(pItem);

		EntityAI ownerEnt = EntityAI.Cast(pOwner);
		if (ownerEnt)
		{
			newOwner = new PlayZLogData_Entity();
			newOwner.Init(ownerEnt);

			Man ownerRoot = ownerEnt.GetHierarchyRootPlayer();
			if (ownerRoot)
			{
				newRootPlayer = new PlayZLogData_Player();
				newRootPlayer.InitPlayer(ownerRoot);
			}
		}

		if (reason == "cargoAdded")
		{
			fromLocation = "vicinityOrOther";
			toLocation = "playerCargo";
		}
		else if (reason == "cargoRemoved")
		{
			fromLocation = "playerCargo";
			toLocation = "unknown";
		}
		else
		{
			fromLocation = "playerCargo";
			toLocation = "playerCargo";
		}
	}

	override string ToJson()
	{
		string itemJson = "null";
		if (item) itemJson = item.ToJson();
		string oldOwnerJson = "null";
		if (oldOwner) oldOwnerJson = oldOwner.ToJson();
		string newOwnerJson = "null";
		if (newOwner) newOwnerJson = newOwner.ToJson();
		string oldRootJson = "null";
		if (oldRootPlayer) oldRootJson = oldRootPlayer.ToJson();
		string newRootJson = "null";
		if (newRootPlayer) newRootJson = newRootPlayer.ToJson();
		return string.Format("{\"moveReason\":\"%1\",\"fromLocation\":\"%2\",\"toLocation\":\"%3\",\"item\":%4,\"oldOwner\":%5,\"newOwner\":%6,\"oldRootPlayer\":%7,\"newRootPlayer\":%8}", moveReason, fromLocation, toLocation, itemJson, oldOwnerJson, newOwnerJson, oldRootJson, newRootJson);
	}

	override string GetActorId()
	{
		if (newRootPlayer) return newRootPlayer.GetActorId();
		if (oldRootPlayer) return oldRootPlayer.GetActorId();
		return "";
	}

	override string GetTargetId()
	{
		if (oldRootPlayer && newRootPlayer && oldRootPlayer.GetActorId() != newRootPlayer.GetActorId())
		{
			return oldRootPlayer.GetActorId();
		}
		return "";
	}

	override string GetTrackId()
	{
		string actor = GetActorId();
		if (actor != "") return "P-" + actor;
		return "";
	}
}

class PlayZLogData_Admin : PlayZLogData_Base
{
	ref PlayZLogData_Player player;
	ref PlayZLogData_Position startPos;
	ref PlayZLogData_Position targetPos;
	string reason;
	
	void InitAdmin(Managed pPlayer, vector pStartPos, vector pTargetPos, string pReason)
	{
		player = new PlayZLogData_Player();
		player.InitPlayer(pPlayer);
		startPos = new PlayZLogData_Position();
		startPos.Init(pStartPos);
		targetPos = new PlayZLogData_Position();
		targetPos.Init(pTargetPos);
		reason = pReason;
	}
	
	override string ToJson()
	{
		string playerJson = "null";
		if (player) playerJson = player.ToJson();
		string startPosJson = "null";
		if (startPos) startPosJson = startPos.ToJson();
		string targetPosJson = "null";
		if (targetPos) targetPosJson = targetPos.ToJson();
		return string.Format("{\"player\":%1,\"startPos\":%2,\"targetPos\":%3,\"reason\":\"%4\"}", playerJson, startPosJson, targetPosJson, reason);
	}

	override string GetActorId()
	{
		if (player) return player.GetActorId();
		return "";
	}
}
