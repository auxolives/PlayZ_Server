class PlayZVehicleLogState
{
	float m_LastLogTime;
	string m_UID;

	void PlayZVehicleLogState()
	{
		m_LastLogTime = 0;
		m_UID = "";
	}
}

class PlayZVehicleRegistry
{
	static ref set<Transport> m_AllVehicles = new set<Transport>();
	static ref map<Transport, ref PlayZVehicleLogState> m_VehicleStates = new map<Transport, ref PlayZVehicleLogState>();

	static PlayZVehicleLogState GetOrCreateState(Transport transport)
	{
		if (!transport)
			return null;

		PlayZVehicleLogState state;
		if (m_VehicleStates.Find(transport, state))
			return state;

		state = new PlayZVehicleLogState();
		state.m_LastLogTime = GetGame().GetTime();
		state.m_UID = PlayZEntityIDProvider.NextVehicleID();
		m_VehicleStates.Set(transport, state);
		return state;
	}

	static void RemoveState(Transport transport)
	{
		if (!transport || !m_VehicleStates)
			return;
		m_VehicleStates.Remove(transport);
	}
}
