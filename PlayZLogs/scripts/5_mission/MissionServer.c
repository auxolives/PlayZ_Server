modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		PlayZLogger.Init();
		
		if (GetGame().IsServer())
		{
			int weatherInterval = 300000;
			
			if (PlayZLogger.m_Config) 
			{
				weatherInterval = PlayZLogger.m_Config.m_WeatherLogInterval * 1000;
			}

			if (!PlayZLogger.m_Config || PlayZLogger.m_Config.m_EnableLogWeather)
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.PlayZLogWeather, weatherInterval, true);
		}
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		PlayZLogger.OnUpdate(timeslice);
	}
	
	override void OnMissionFinish()
	{
		PlayZLogger.OnShutdown();
		super.OnMissionFinish();
	}
	
	void PlayZLogWeather()
	{
		if (GetGame().IsServer())
		{
			PlayZLogData_Weather data = new PlayZLogData_Weather();
			Weather weather = GetGame().GetWeather();
			
			data.Init(weather);
			data.scenario = PlayZConfig.m_CurrentScenarioName;
			data.snowfall = weather.GetSnowfall().GetActual();

			// Vol fog ranges: read PlayZConfig (synced by PlayZWeather on server), not world data.
			// m_CurrentScenario lives on modded SakhalData/ChernarusPlusData in PlayZWeather only.
			if (PlayZConfig.m_CurrentScenarioName != "")
			{
				data.vfdMin = PlayZConfig.m_LastVolFogDistTarget;
				data.vfdMax = PlayZConfig.m_LastVolFogDistTarget;
				data.vfhMin = PlayZConfig.m_LastVolFogHeightTarget;
				data.vfhMax = PlayZConfig.m_LastVolFogHeightTarget;
			}
			
			PlayZLogger.LogWeather("WeatherSnapshot", data);
		}
	}
	
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);

		if (player && identity)
		{
			player.PlayZSetPersistentIdentity(identity);
		}
		
		PlayZLogData_Player data = new PlayZLogData_Player();
		data.InitPlayerWithIdentity(player, identity);
		PlayZLogger.LogPeople("PlayerConnected", data);
	}

	override void InvokeOnDisconnect(PlayerBase player)
	{
		PlayZLogData_Player data = new PlayZLogData_Player();
		data.InitPlayer(player);
		PlayZLogger.LogPeople("PlayerDisconnected", data);

		super.InvokeOnDisconnect(player);
	}
}
