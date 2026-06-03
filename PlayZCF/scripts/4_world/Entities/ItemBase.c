//! Omitted when GAMELABS is not defined (client / non-GameLabs script builds).
#ifdef GAMELABS
modded class ItemBase
{
	ref _Event m_PlayZCF_ItemEventInstance;

	override void EEInit()
	{
		super.EEInit();
		if (!GetGame().IsServer()) return;
		if (!GetGameLabs()) return;

		string typeName = GetType();
		string iconName = "";
		string displayName = "";

		if (typeName == "dzn_module_card")
		{
			iconName = "trophy";
			displayName = "Lantia Module Card";
		}
		else if (typeName == "dzn_module_lantia" || typeName == "dzn_module_surge" || typeName == "dzn_module_ext" || typeName == "dzn_module_ext2")
		{
			iconName = "trophy";
			displayName = "Lantia Module";
		}
		else if (typeName == "dzn_printer_filament_nylon" || typeName == "dzn_printer_filament_tpc" || typeName == "dzn_printer_filament_abs")
		{
			iconName = "trophy";
			displayName = "Printer Filament";
		}
		else if (typeName == "dzn_apsi")
		{
			iconName = "trophy";
			displayName = "APSI";
		}
		else if (typeName == "dzn_lehs_o2tank" || typeName == "dzn_lehs_battery" || typeName == "dzn_blueprint_lehs")
		{
			iconName = "trophy";
			displayName = "LEHS Component";
		}

		if (iconName == "") return;

		// Only map world-spawned items; skip inventory/attachments/in-hands.
		if (GetHierarchyParent()) return;

		vector position = GetPosition();
		if (position[0] <= 0 && position[1] <= 0 && position[2] <= 0) return;

		m_PlayZCF_ItemEventInstance = new _Event(typeName, iconName, this, displayName);
		GetGameLabs().RegisterEventRadiusExclusive(m_PlayZCF_ItemEventInstance, 2.0);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (!GetGame().IsServer()) return;
		if (!GetGameLabs()) return;
		if (!m_PlayZCF_ItemEventInstance) return;
		GetGameLabs().RemoveEvent(m_PlayZCF_ItemEventInstance);
	}
};
#endif
