//! Omitted when GAMELABS is not defined (client / non-GameLabs script builds).
#ifdef GAMELABS
modded class House
{
	ref _Event m_PlayZCF_EventInstance;

	override void EEInit()
	{
		super.EEInit();
		if (!GetGame().IsServer()) return;
		if (!GetGameLabs()) return;

		string typeName = GetType();
		string iconName = "";
		string displayName = "";

		// StaticCivilianCamp
		if (typeName == "StaticObj_Slum_Roof6")
		{
			iconName = "couch";
			displayName = "Civilian Camp";
		}
		// StaticBox
		else if (typeName == "StaticObj_Misc_BoxWooden")
		{
			iconName = "box";
			displayName = "Supply Box";
		}
		// StaticChair
		else if (typeName == "StaticObj_Misc_Chair_Camp1")
		{
			iconName = "chair";
			displayName = "Camp Chair";
		}
		// StaticTable
		else if (typeName == "StaticObj_Misc_Table_Camp")
		{
			iconName = "table";
			displayName = "Camp Table";
		}
		// StaticAmbulance
		else if (typeName == "Land_Wreck_S1023_Medic_Beige_DE")
		{
			iconName = "ambulance";
			displayName = "Ambulance Wreck";
		}
		// StaticAirplaneCrate
		else if (typeName == "StaticObj_Misc_SupplyBox2_DE" || typeName == "StaticObj_Misc_SupplyBox3_DE")
		{
			iconName = "dropbox";
			displayName = "Airplane Crate";
		}
		// StaticBoatMilitary
		else if (typeName == "StaticObj_PatrolBoat_Military_DE")
		{
			iconName = "ship";
			displayName = "Military Boat";
		}
		// StaticMedDrop
		else if (typeName == "Misc_cargo_cont_net1")
		{
			iconName = "dropbox";
			displayName = "Medical Drop";
		}
		// StaticLifeboat
		else if (typeName == "sea_oilrig_lifeboat")
		{
			iconName = "ship";
			displayName = "Lifeboat";
		}

		if (iconName == "") return;

		vector position = GetPosition();
		if (position[0] <= 0 && position[1] <= 0 && position[2] <= 0) return;

		m_PlayZCF_EventInstance = new _Event(typeName, iconName, this, displayName);
		GetGameLabs().RegisterEventRadiusExclusive(m_PlayZCF_EventInstance, 40.0);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (!GetGame().IsServer()) return;
		if (!GetGameLabs()) return;
		if (!m_PlayZCF_EventInstance) return;
		GetGameLabs().RemoveEvent(m_PlayZCF_EventInstance);
	}
};
#endif
