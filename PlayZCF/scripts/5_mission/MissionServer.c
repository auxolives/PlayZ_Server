//! Registers extra GameLabs monitored action names (server). Omitted when GAMELABS is not defined.
#ifdef GAMELABS
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();

		if (GetGame().IsServer() && GetGameLabs())
		{
			GetGameLabs().AddMonitoredAction("ActionDigInStash");
			GetGameLabs().AddMonitoredAction("ActionDigOutStash");
			GetGameLabs().AddMonitoredAction("ActionOpenBarrel");
			GetGameLabs().AddMonitoredAction("ActionCloseBarrel");
			GetGameLabs().AddMonitoredAction("ActionOpenBarrelHoles");
			GetGameLabs().AddMonitoredAction("ActionCloseBarrelHoles");
			GetGameLabs().AddMonitoredAction("ActionToggleTentOpen");
			GetGameLabs().AddMonitoredAction("ActionPackTent");
			GetGameLabs().AddMonitoredAction("ActionGetInTransport");
			GetGameLabs().AddMonitoredAction("ActionGetOutTransport");
			GetGameLabs().AddMonitoredAction("ActionBuildShelter");
			GetGameLabs().AddMonitoredAction("ActionDeconstructShelter");
			GetGameLabs().AddMonitoredAction("ActionRaiseFlag");
			GetGameLabs().AddMonitoredAction("ActionLowerFlag");
			GetGameLabs().AddMonitoredAction("ActionPlaceObject");
			GetGameLabs().AddMonitoredAction("ActionDeployObject");
			GetGameLabs().AddMonitoredAction("ActionDeployHuntingTrap");
			GetGameLabs().AddMonitoredAction("ActionDigGardenPlot");
			GetGameLabs().AddMonitoredAction("ActionDismantleGardenPlot");
		}
	}
}
#endif
