// Server-side half of PlayZ AntiCheat's camera spot-check.
//
// Client-side request handling (RPC_PLAYZ_AC_CAMERA_REQUEST -> response) lives in
// PlayZ_Client/PlayZAntiCheatClient/scripts/4_World/PlayerBase.c and ships to both sides.
// This file only handles the RESPONSE that comes back to the server and forwards
// it to the player monitor.
//
// Response payload (see PlayZAntiCheatClient_Shared.c):
//   cameraPos       — vector, client GetCurrentCameraPosition()
//   localPlayerPos  — vector, client player.GetPosition() same frame
//   clientTime      — float,  client GetGame().GetTime()
//   flags           — int,    bitfield (PLAYZ_AC_CAMERA_FLAG_*)
modded class PlayerBase
{
	protected ref PlayZAntiCheatPlayerState m_PlayZACState;

	PlayZAntiCheatPlayerState GetPlayZACState() { return m_PlayZACState; }
	void SetPlayZACState(PlayZAntiCheatPlayerState state) { m_PlayZACState = state; }

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (rpc_type == RPC_PLAYZ_AC_CAMERA_RESPONSE)
		{
			HandlePlayZACCameraResponse(ctx);
		}
	}

	protected void HandlePlayZACCameraResponse(ParamsReadContext ctx)
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		vector cameraPos;
		vector localPlayerPos;
		float clientTime;
		int flags;

		if (!ctx.Read(cameraPos))
			return;
		if (!ctx.Read(localPlayerPos))
			return;
		if (!ctx.Read(clientTime))
			clientTime = 0;
		// Older clients that pre-date the flags field are tolerated by leaving flags at 0.
		if (!ctx.Read(flags))
			flags = 0;

		PlayZAntiCheatPlayerMonitor monitor = PlayZAntiCheatPlayerMonitor.GetInstance();
		if (monitor)
			monitor.OnCameraResponse(this, cameraPos, localPlayerPos, clientTime, flags);
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		if (!GetGame() || !GetGame().IsServer())
			return;

		PlayZAntiCheatPlayerMonitor monitor = PlayZAntiCheatPlayerMonitor.GetInstance();
		if (monitor)
			monitor.OnPlayerDeath(this);
	}
};
