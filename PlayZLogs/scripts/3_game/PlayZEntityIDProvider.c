// PlayZEntityIDProvider
// Mints unique short IDs for non-player entities (zombies, animals, vehicles).
// Counters reset on server restart — IDs are unique within a single session/log file.
class PlayZEntityIDProvider
{
	static int m_InfectedCounter = 0;
	static int m_AnimalCounter   = 0;
	static int m_VehicleCounter  = 0;

	static string NextInfectedID()
	{
		m_InfectedCounter = m_InfectedCounter + 1;
		return "Z-" + PlayZLogger.m_SessionSeed + "-" + m_InfectedCounter.ToStringLen(4);
	}

	static string NextAnimalID()
	{
		m_AnimalCounter = m_AnimalCounter + 1;
		return "A-" + PlayZLogger.m_SessionSeed + "-" + m_AnimalCounter.ToStringLen(4);
	}

	static string NextVehicleID()
	{
		m_VehicleCounter = m_VehicleCounter + 1;
		return "V-" + PlayZLogger.m_SessionSeed + "-" + m_VehicleCounter.ToStringLen(4);
	}
}
