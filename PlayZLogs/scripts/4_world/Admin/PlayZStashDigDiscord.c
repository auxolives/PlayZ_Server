enum PlayZStashDigOutMatch
{
	SAME_PLAYER = 0,
	DIFFERENT_PLAYER = 1,
	UNKNOWN_BURIAL = 2
}

class PlayZStashDigDiscord
{
	protected static const int COLOR_DIG_IN = 0x3498DB;
	protected static const int COLOR_SAME_PLAYER = 0x2ECC71;
	protected static const int COLOR_DIFFERENT_PLAYER = 0xFFA500;
	protected static const int COLOR_UNKNOWN_BURIAL = 0x95A5A6;

	static bool IsEnabled()
	{
		return GetGame() && GetGame().IsServer() && PlayZLogger.m_Config && PlayZLogger.m_Config.m_EnableStashDigDiscord && PlayZLogger.m_Config.m_DiscordStashWebhookUrl != "";
	}

	static void ReportDigIn(UndergroundStash stash, ItemBase cargo, PlayerBase player, ItemBase tool)
	{
		if (!IsEnabled() || !cargo || !player)
			return;

		string cargoPid = PlayZLogPersistentId.Format(cargo);
		if (cargoPid == "")
		{
			Print("[PlayZLogs] Stash dig-in Discord skipped: invalid cargo persistent ID");
			return;
		}

		PlayZStashDigRegistry.RegisterEntry(BuildDigInRegistryEntry(stash, cargo, player, tool));

		string diggerName;
		string diggerSteam64;
		ResolvePlayerIdentity(player, diggerName, diggerSteam64);

		ref array<ref PlayZLogDiscordEmbedField> fields = new array<ref PlayZLogDiscordEmbedField>();
		PlayZLogDiscord.AddEmbedField(fields, "Digger", diggerName, true);
		if (diggerSteam64 != "")
			PlayZLogDiscord.AddEmbedField(fields, "Steam64", diggerSteam64, true);
		PlayZLogDiscord.AddEmbedField(fields, "Cargo ID", cargoPid, false);
		PlayZLogDiscord.AddEmbedField(fields, "Cargo", cargo.GetType(), true);
		if (tool)
			PlayZLogDiscord.AddEmbedField(fields, "Tool", tool.GetType(), true);
		PlayZLogDiscord.AddEmbedField(fields, "Position", PlayZLogDiscord.FormatPosition(player.GetPosition()), false);

		if (stash)
		{
			string stashPid = PlayZLogPersistentId.Format(stash);
			if (stashPid != "")
				PlayZLogDiscord.AddEmbedField(fields, "Stash ID", stashPid, false);
			PlayZLogDiscord.AddEmbedField(fields, "Stash type", stash.GetType(), true);
		}

		string summary = BuildDigInSummary(cargo.GetType(), tool, player.GetPosition());
		PlayZLogDiscord.Send(PlayZLogger.m_Config.m_DiscordStashWebhookUrl, "Stash buried", summary, fields, COLOR_DIG_IN);
	}

	static void ReportDigOut(UndergroundStash stash, ItemBase cargo, PlayerBase player, ItemBase tool, PlayZStashDigRegistryEntry digInRecord, string cargoPid)
	{
		if (!IsEnabled() || !player)
			return;

		string diggerName;
		string diggerSteam64;
		ResolvePlayerIdentity(player, diggerName, diggerSteam64);

		PlayZStashDigOutMatch match = ResolveDigOutMatch(diggerSteam64, digInRecord);
		int color;
		string title;
		ResolveDigOutPresentation(match, title, color);

		ref array<ref PlayZLogDiscordEmbedField> fields = new array<ref PlayZLogDiscordEmbedField>();
		PlayZLogDiscord.AddEmbedField(fields, "Digger", diggerName, true);
		if (diggerSteam64 != "")
			PlayZLogDiscord.AddEmbedField(fields, "Steam64", diggerSteam64, true);

		if (cargoPid != "")
			PlayZLogDiscord.AddEmbedField(fields, "Cargo ID", cargoPid, false);

		if (cargo)
			PlayZLogDiscord.AddEmbedField(fields, "Cargo", cargo.GetType(), true);
		else
			PlayZLogDiscord.AddEmbedField(fields, "Cargo", "unknown", true);

		if (tool)
			PlayZLogDiscord.AddEmbedField(fields, "Tool", tool.GetType(), true);

		PlayZLogDiscord.AddEmbedField(fields, "Position", PlayZLogDiscord.FormatPosition(player.GetPosition()), false);

		if (stash)
			PlayZLogDiscord.AddEmbedField(fields, "Stash type", stash.GetType(), true);

		if (digInRecord)
		{
			vector digInPos;
			digInPos[0] = digInRecord.digInPosX;
			digInPos[1] = digInRecord.digInPosY;
			digInPos[2] = digInRecord.digInPosZ;
			string burialContext = string.Format("Buried by %1 (%2) on %3 at %4", digInRecord.diggerName, digInRecord.diggerSteam64, PlayZLogUtcTime.FormatUnixUtc(digInRecord.digInUnixTime), PlayZLogDiscord.FormatPosition(digInPos));
			PlayZLogDiscord.AddEmbedField(fields, "Burial context", burialContext, false);
		}

		string cargoType = "unknown";
		if (cargo)
			cargoType = cargo.GetType();

		string summary = BuildDigOutSummary(cargoType, tool, player.GetPosition(), match);
		PlayZLogDiscord.Send(PlayZLogger.m_Config.m_DiscordStashWebhookUrl, title, summary, fields, color);

		if (cargoPid != "")
			PlayZStashDigRegistry.Remove(cargoPid);
	}

	static void HandleDigInFinished(ActionData action_data)
	{
		if (!GetGame() || !GetGame().IsServer() || !action_data || !action_data.m_Player)
			return;

		PlayerBase player = PlayerBase.Cast(action_data.m_Player);
		if (!player)
			return;

		EntityAI buriedItem = EntityAI.Cast(action_data.m_Target.GetObject());
		if (!buriedItem)
			return;

		ItemBase cargo = ItemBase.Cast(buriedItem);
		if (!cargo)
			return;

		UndergroundStash stash = PlayZStashDigResolve.ResolveStashAfterDigIn(buriedItem, player.GetPosition());
		ItemBase tool = ItemBase.Cast(action_data.m_MainItem);

		ReportDigIn(stash, cargo, player, tool);
	}

	static void HandleDigOutFinished(ActionData action_data)
	{
		if (!GetGame() || !GetGame().IsServer() || !action_data || !action_data.m_Player)
			return;

		PlayerBase player = PlayerBase.Cast(action_data.m_Player);
		if (!player)
			return;

		UndergroundStash stash = PlayZStashDigResolve.ResolveStashFromTarget(action_data.m_Target);
		ItemBase cargo;
		if (stash)
			cargo = stash.GetStashedItem();

		string cargoPid = PlayZLogPersistentId.Format(cargo);
		PlayZStashDigRegistryEntry digInRecord;
		if (cargoPid != "")
			digInRecord = PlayZStashDigRegistry.Lookup(cargoPid);

		ItemBase tool = ItemBase.Cast(action_data.m_MainItem);
		ReportDigOut(stash, cargo, player, tool, digInRecord, cargoPid);
	}

	protected static void ResolvePlayerIdentity(PlayerBase player, out string name, out string steam64)
	{
		name = "Unknown";
		steam64 = "";

		if (!player)
			return;

		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return;

		name = identity.GetName();
		steam64 = identity.GetPlainId();
	}

	protected static PlayZStashDigOutMatch ResolveDigOutMatch(string diggerSteam64, PlayZStashDigRegistryEntry digInRecord)
	{
		if (!digInRecord || digInRecord.diggerSteam64 == "")
			return PlayZStashDigOutMatch.UNKNOWN_BURIAL;

		if (diggerSteam64 == "" || diggerSteam64 != digInRecord.diggerSteam64)
			return PlayZStashDigOutMatch.DIFFERENT_PLAYER;

		return PlayZStashDigOutMatch.SAME_PLAYER;
	}

	protected static void ResolveDigOutPresentation(PlayZStashDigOutMatch match, out string title, out int color)
	{
		switch (match)
		{
			case PlayZStashDigOutMatch.SAME_PLAYER:
				title = "Stash recovered — same player";
				color = COLOR_SAME_PLAYER;
				break;

			case PlayZStashDigOutMatch.DIFFERENT_PLAYER:
				title = "Stash recovered — different player";
				color = COLOR_DIFFERENT_PLAYER;
				break;

			default:
				title = "Stash recovered — unknown burial";
				color = COLOR_UNKNOWN_BURIAL;
				break;
		}
	}

	protected static string BuildDigInSummary(string cargoType, ItemBase tool, vector pos)
	{
		string line = "ActionDigInStash " + cargoType;
		if (tool)
			line = line + " with " + tool.GetType();
		line = line + " " + PlayZLogDiscord.FormatPosition(pos);
		return line;
	}

	protected static string BuildDigOutSummary(string cargoType, ItemBase tool, vector pos, PlayZStashDigOutMatch match)
	{
		string line = "ActionDigOutStash";
		if (cargoType != "" && cargoType != "unknown")
			line = line + " " + cargoType;
		if (tool)
			line = line + " with " + tool.GetType();
		line = line + " " + PlayZLogDiscord.FormatPosition(pos);

		if (match == PlayZStashDigOutMatch.DIFFERENT_PLAYER)
			line = line + " | not original burier";
		else if (match == PlayZStashDigOutMatch.UNKNOWN_BURIAL)
			line = line + " | no matching dig-in record";

		return line;
	}

	protected static PlayZStashDigRegistryEntry BuildDigInRegistryEntry(UndergroundStash stash, ItemBase cargo, PlayerBase player, ItemBase tool)
	{
		string diggerName;
		string diggerSteam64;
		ResolvePlayerIdentity(player, diggerName, diggerSteam64);

		ref PlayZStashDigRegistryEntry entry = new PlayZStashDigRegistryEntry();
		entry.cargoPid = PlayZLogPersistentId.Format(cargo);
		entry.cargoType = cargo.GetType();
		entry.stashPid = PlayZLogPersistentId.Format(stash);
		if (stash)
			entry.stashType = stash.GetType();
		entry.diggerName = diggerName;
		entry.diggerSteam64 = diggerSteam64;

		vector pos = player.GetPosition();
		entry.digInPosX = pos[0];
		entry.digInPosY = pos[1];
		entry.digInPosZ = pos[2];
		entry.digInUnixTime = PlayZLogUtcTime.NowUnixUtc();

		if (tool)
			entry.toolType = tool.GetType();

		return entry;
	}
}
