class PlayZAntiCheatKeybindService
{
	protected static ref PlayZAntiCheatKeybindService s_Instance;

	static PlayZAntiCheatKeybindService Get()
	{
		if (!s_Instance)
			s_Instance = new PlayZAntiCheatKeybindService();
		return s_Instance;
	}

	protected PlayZAntiCheatPlayerState GetOrCreateState(PlayerBase player)
	{
		PlayZAntiCheatPlayerState state = player.GetPlayZACState();
		if (!state)
		{
			state = new PlayZAntiCheatPlayerState();
			player.SetPlayZACState(state);
		}
		return state;
	}

	void OnKeybindReport(PlayerBase player, string keysPressed)
	{
		if (!GetGame() || !GetGame().IsServer() || !player)
			return;

		PlayZAntiCheatConfig cfg = PlayZAntiCheatConfig.Get();
		if (!cfg.EnableKeybindChecks)
			return;

		if (PlayZAntiCheatUtils.IsPlayerKeybindExcluded(player))
			return;

		keysPressed.Trim();
		if (keysPressed == "")
			return;

		if (keysPressed.Length() > cfg.KeybindMaxPayloadChars)
			keysPressed = keysPressed.Substring(0, cfg.KeybindMaxPayloadChars);

		float now = GetGame().GetTime();
		PlayZAntiCheatPlayerState state = GetOrCreateState(player);
		int cooldownMs = Math.Max(cfg.KeybindAlertCooldownSeconds, 1) * 1000;
		if (state.LastKeybindAlertTime > 0 && now - state.LastKeybindAlertTime < cooldownMs)
			return;

		state.LastKeybindAlertTime = now;

		string playerName = PlayZAntiCheatUtils.GetIdentityName(player);
		string playerId = PlayZAntiCheatUtils.GetIdentityId(player);
		string steam64 = PlayZAntiCheatUtils.GetSteamId64(player);

		string logLine = string.Format("Suspicious keybind: %1 (%2) steam64=%3 | keys=%4", playerName, playerId, steam64, keysPressed);
		PlayZAntiCheatLogger.Info(logLine);

		string webhook = PlayZAntiCheatUtils.GetKeybindWebhookUrl();
		if (webhook == "")
			return;

		ref array<ref PlayZDiscordEmbedField> fields = new array<ref PlayZDiscordEmbedField>();
		PlayZAntiCheatUtils.AddEmbedField(fields, "Keys pressed", keysPressed);
		PlayZAntiCheatUtils.AddEmbedField(fields, "Steam64", steam64);
		PlayZAntiCheatUtils.AddEmbedField(fields, "Player ID", playerId);

		string description = string.Format("Player: **%1** (%2)", playerName, playerId);
		string payload = PlayZAntiCheatUtils.BuildWebhookEmbed("PlayZ AntiCheat", ":keyboard: SUSPICIOUS KEYBIND", description, 0x9B59B6, fields);
		if (payload != "")
			PlayZAntiCheatWebhookDispatcher.Get().Enqueue(webhook, payload);
	}
}
