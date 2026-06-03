class CfgPatches
{
	class PlayZAntiCheat
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {
			"DZ_Data",
			"DZ_Scripts",
			"PlayZAntiCheatClient"
		};
	};
};
class CfgMods
{
	class PlayZAntiCheat
	{
		hideName = 1;
		hidePicture = 1;
		name = "PlayZAntiCheat";
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
					"PlayZ_Server/PlayZAntiCheat/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {
					"PlayZ_Server/PlayZAntiCheat/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {
					"PlayZ_Server/PlayZAntiCheat/scripts/5_Mission"
				};
			};
		};
	};
};
