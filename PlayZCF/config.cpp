class CfgPatches
{
	class PlayZCF
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {
			"DZ_Data",
			"DZ_Scripts",
			"GameLabs_Scripts"
		};
	};
};

class CfgMods
{
	class PlayZCF
	{
		hideName = 1;
		hidePicture = 1;
		name = "PlayZCF";
		credits = "";
		author = "Olivier";
		type = "mod";
		dependencies[] = {
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {
					"PlayZ_Server/PlayZCF/scripts/4_world"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {
					"PlayZ_Server/PlayZCF/scripts/5_mission"
				};
			};
		};
	};
};
