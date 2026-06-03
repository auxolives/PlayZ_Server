class CfgPatches
{
	class PlayZLogs
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {
			"DZ_Data",
			"DZ_Scripts"
		};
	};
};
class CfgMods
{
	class PlayZLogs
	{
		hideName = 1;
		hidePicture = 1;
		name = "PlayZLogs";
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
		class gameScriptModule
		{
			value = "";
			files[] = {
				"PlayZ_Server/PlayZLogs/scripts/3_game"
			};
		};
		class worldScriptModule
		{
			value = "";
			files[] = {
				"PlayZ_Server/PlayZLogs/scripts/4_world"
			};
		};
		class missionScriptModule
		{
			value = "";
			files[] = {
				"PlayZ_Server/PlayZLogs/scripts/5_mission"
			};
		};
		};
	};
};
