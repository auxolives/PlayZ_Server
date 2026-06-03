class PlayZLogData_Base : Managed
{
	string ToJson() { return "{}"; }
	string GetActorId() { return ""; }
	string GetTargetId() { return ""; }
	string GetTrackId() { return ""; }

	static string EscapeJson(string raw)
	{
		if (raw == "")
			return "";

		// Keep escaping parser-safe for Enforce in 3_Game.
		// Newline/control escaping is enough for current log fields.
		raw.Replace("\n", "\\n");
		raw.Replace("\r", "\\r");
		raw.Replace("\t", "\\t");
		return raw;
	}
}

class PlayZLogData_Position : PlayZLogData_Base
{
	float x;
	float y;
	float z;
	
	void Init(vector pos)
	{
		x = pos[0];
		y = pos[1];
		z = pos[2];
	}

	override string ToJson()
	{
		return string.Format("{\"x\":%1,\"y\":%2,\"z\":%3}", x, y, z);
	}
}

class PlayZLogData_Entity : PlayZLogData_Base
{
	string type;
	ref PlayZLogData_Position pos;
	float health;
	string m_Id = "";
	string m_Name = "";
	
	void Init(Managed obj)
	{
		if (!obj)
			return;

		Object worldObj = Object.Cast(obj);
		if (!worldObj)
			return;

		type = worldObj.GetType();
		Man man = Man.Cast(worldObj);
		if (man && man.GetIdentity())
		{
			m_Id = man.GetIdentity().GetId();
			m_Name = man.GetIdentity().GetName();
		}

		// During inventory juncture / location transitions, native transform and
		// some detail reads can fault while the script handle still appears valid.
		bool safeDetail;
		safeDetail = true;
		EntityAI entAi = EntityAI.Cast(worldObj);
		if (entAi)
		{
			if (entAi.IsSetForDeletion())
				safeDetail = false;
		}
		if (worldObj.IsDamageDestroyed())
			safeDetail = false;

		if (safeDetail)
		{
			pos = new PlayZLogData_Position();
			if (pos)
				pos.Init(worldObj.GetPosition());

			if (worldObj.HasDamageSystem())
				health = worldObj.GetHealth("", "");
			else
				health = -1.0;

		}
		else
		{
			health = -1.0;
		}
	}

	override string ToJson()
	{
		string posJson = "null";
		if (pos) posJson = pos.ToJson();
		
		string json = string.Format("{\"type\":\"%1\",\"pos\":%2,\"health\":%3", type, posJson, health);
		if (m_Id != "")
		{
			json += string.Format(",\"id\":\"%1\",\"name\":\"%2\"", m_Id, m_Name);
		}
		json += "}";

		return json;
	}

	override string GetActorId()
	{
		if (m_Id != "") return m_Id;
		return "";
	}
}

class PlayZLogData_Damage : PlayZLogData_Base
{
	ref PlayZLogData_Entity source;
	ref PlayZLogData_Entity target;
	string zone;
	string ammo;
	float damage;
	bool m_MinimalPayload;
	string m_SourceType;
	string m_TargetType;
	string m_SourceId;
	string m_TargetId;
	string m_SourceName;
	string m_TargetName;
	
	void Init(EntityAI pSource, EntityAI pTarget, string pZone, string pAmmo, float pDamage)
	{
		m_MinimalPayload = false;
		if (pSource)
		{
			source = new PlayZLogData_Entity();
			source.Init(pSource);

			Man sourceRootPlayer = pSource.GetHierarchyRootPlayer();
			if (sourceRootPlayer && sourceRootPlayer.GetIdentity())
			{
				source.m_Id = sourceRootPlayer.GetIdentity().GetId();
				source.m_Name = sourceRootPlayer.GetIdentity().GetName();
			}
		}
		
		if (pTarget)
		{
			target = new PlayZLogData_Entity();
			target.Init(pTarget);

			Man targetRootPlayer = pTarget.GetHierarchyRootPlayer();
			if (targetRootPlayer && targetRootPlayer.GetIdentity())
			{
				target.m_Id = targetRootPlayer.GetIdentity().GetId();
				target.m_Name = targetRootPlayer.GetIdentity().GetName();
			}
			else
			{
				EntityAI targetRootEntity = pTarget.GetHierarchyRoot();
				Man targetRootMan = Man.Cast(targetRootEntity);
				if (targetRootMan && targetRootMan.GetIdentity())
				{
					target.m_Id = targetRootMan.GetIdentity().GetId();
					target.m_Name = targetRootMan.GetIdentity().GetName();
				}
			}
		}
		
		zone = pZone;
		ammo = pAmmo;
		damage = pDamage;
	}

	void InitMinimal(EntityAI pSource, EntityAI pTarget, string pZone, string pAmmo, float pDamage)
	{
		m_MinimalPayload = true;
		source = null;
		target = null;
		m_SourceType = "";
		m_TargetType = "";
		m_SourceId = "";
		m_TargetId = "";
		m_SourceName = "";
		m_TargetName = "";
		zone = pZone;
		ammo = pAmmo;
		damage = pDamage;

		if (pSource)
		{
			Object pSourceObj = Object.Cast(pSource);
			if (pSourceObj)
				m_SourceType = pSourceObj.GetType();

			Man sourceRootPlayer = pSource.GetHierarchyRootPlayer();
			if (sourceRootPlayer && sourceRootPlayer.GetIdentity())
			{
				m_SourceId = sourceRootPlayer.GetIdentity().GetId();
				m_SourceName = sourceRootPlayer.GetIdentity().GetName();
			}
		}

		if (pTarget)
		{
			Object pTargetObj = Object.Cast(pTarget);
			if (pTargetObj)
				m_TargetType = pTargetObj.GetType();

			Man targetRootPlayer = pTarget.GetHierarchyRootPlayer();
			if (targetRootPlayer && targetRootPlayer.GetIdentity())
			{
				m_TargetId = targetRootPlayer.GetIdentity().GetId();
				m_TargetName = targetRootPlayer.GetIdentity().GetName();
			}
			else
			{
				EntityAI targetRootEntity = pTarget.GetHierarchyRoot();
				Man targetRootMan = Man.Cast(targetRootEntity);
				if (targetRootMan && targetRootMan.GetIdentity())
				{
					m_TargetId = targetRootMan.GetIdentity().GetId();
					m_TargetName = targetRootMan.GetIdentity().GetName();
				}
			}
		}
	}

	override string ToJson()
	{
		if (m_MinimalPayload)
		{
			string eSrcType = m_SourceType;
			eSrcType = PlayZLogData_Base.EscapeJson(eSrcType);
			string eSrcId = m_SourceId;
			eSrcId = PlayZLogData_Base.EscapeJson(eSrcId);
			string eSrcName = m_SourceName;
			eSrcName = PlayZLogData_Base.EscapeJson(eSrcName);
			string eTgtType = m_TargetType;
			eTgtType = PlayZLogData_Base.EscapeJson(eTgtType);
			string eTgtId = m_TargetId;
			eTgtId = PlayZLogData_Base.EscapeJson(eTgtId);
			string eTgtName = m_TargetName;
			eTgtName = PlayZLogData_Base.EscapeJson(eTgtName);

			return string.Format("{\"source\":{\"type\":\"%1\",\"id\":\"%2\",\"name\":\"%3\"},\"target\":{\"type\":\"%4\",\"id\":\"%5\",\"name\":\"%6\"},\"zone\":\"%7\",\"ammo\":\"%8\",\"damage\":%9}",
				eSrcType,
				eSrcId,
				eSrcName,
				eTgtType,
				eTgtId,
				eTgtName,
				zone,
				ammo,
				damage);
		}

		string sourceJson = "null";
		if (source) sourceJson = source.ToJson();
		string targetJson = "null";
		if (target) targetJson = target.ToJson();
		
		return string.Format("{\"source\":%1,\"target\":%2,\"zone\":\"%3\",\"ammo\":\"%4\",\"damage\":%5}", sourceJson, targetJson, zone, ammo, damage);
	}

	override string GetActorId()
	{
		if (m_MinimalPayload)
			return m_SourceId;
		if (source) return source.GetActorId();
		return "";
	}

	override string GetTargetId()
	{
		if (m_MinimalPayload)
			return m_TargetId;
		if (target) return target.GetActorId();
		return "";
	}
}

class PlayZLogData_Weather : PlayZLogData_Base
{
	float overcast;
	float rain;
	float fog;
	float windSpeed;
	float snowfall;
	string scenario;
	
	float vfdMin, vfdMax; // Volumetric Fog Distance
	float vfhMin, vfhMax; // Volumetric Fog Height
	
	void Init(Weather weather)
	{
		if (weather)
		{
			overcast = weather.GetOvercast().GetActual();
			rain = weather.GetRain().GetActual();
			fog = weather.GetFog().GetActual();
			windSpeed = weather.GetWindSpeed();
		}
	}

	override string ToJson()
	{
		string vfdStr = string.Format("[%1,%2]", vfdMin, vfdMax);
		string vfhStr = string.Format("[%1,%2]", vfhMin, vfhMax);

		return string.Format("{\"overcast\":%1,\"rain\":%2,\"fog\":%3,\"windSpeed\":%4,\"snowfall\":%5,\"scenario\":\"%6\",\"vfd\":%7,\"vfh\":%8}", 
			overcast, rain, fog, windSpeed, snowfall, scenario, vfdStr, vfhStr);
	}
}

class PlayZLogData_VehicleSnapshot : PlayZLogData_Base
{
	string type;
	string uid;
	ref PlayZLogData_Position pos;
	float speed;
	ref array<string> crew;
	
	void Init(Transport vehicle, string pUid = "")
	{
		if (vehicle)
		{
			type = vehicle.GetType();
			uid = pUid;
			pos = new PlayZLogData_Position();
			pos.Init(vehicle.GetPosition());
			
			Car car = Car.Cast(vehicle);
			if (car)
				speed = car.GetSpeedometer();
			else
				speed = GetVelocity(vehicle).Length() * 3.6;

			crew = new array<string>;
			int crewSize = vehicle.CrewSize();
			for (int i = 0; i < crewSize; i++)
			{
				Human crewMember = vehicle.CrewMember(i);
				if (crewMember && crewMember.GetIdentity())
				{
					crew.Insert(crewMember.GetIdentity().GetName() + " (" + crewMember.GetIdentity().GetId() + ")");
				}
			}
		}
	}

	override string ToJson()
	{
		string posJson = "null";
		if (pos) posJson = pos.ToJson();
		
		string crewJson = "[";
		if (crew)
		{
			for (int i = 0; i < crew.Count(); i++)
			{
				crewJson += "\"" + crew.Get(i) + "\"";
				if (i < crew.Count() - 1) crewJson += ",";
			}
		}
		crewJson += "]";
		
		return string.Format("{\"type\":\"%1\",\"uid\":\"%2\",\"pos\":%3,\"speed\":%4,\"crew\":%5}", type, uid, posJson, speed, crewJson);
	}
}
