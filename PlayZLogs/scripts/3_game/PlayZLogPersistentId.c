class PlayZLogPersistentId
{
	protected static const string HEX_DIGITS = "0123456789ABCDEF";

	static bool IsValid(int b1, int b2, int b3, int b4)
	{
		return b1 != 0 || b2 != 0 || b3 != 0 || b4 != 0;
	}

	static bool IsValidEntity(EntityAI entity)
	{
		if (!entity)
			return false;

		int b1, b2, b3, b4;
		entity.GetPersistentID(b1, b2, b3, b4);
		return IsValid(b1, b2, b3, b4);
	}

	static string FormatBlocks(int b1, int b2, int b3, int b4)
	{
		return IntToHexBlock(b1) + IntToHexBlock(b2) + IntToHexBlock(b3) + IntToHexBlock(b4);
	}

	static string Format(EntityAI entity)
	{
		if (!entity)
			return "";

		int b1, b2, b3, b4;
		entity.GetPersistentID(b1, b2, b3, b4);
		if (!IsValid(b1, b2, b3, b4))
			return "";

		return FormatBlocks(b1, b2, b3, b4);
	}

	protected static string IntToHexBlock(int value)
	{
		string result = "";
		for (int i = 7; i >= 0; i--)
		{
			int shift = i * 4;
			int nibble = (value >> shift) & 0xF;
			result = result + HEX_DIGITS.Substring(nibble, 1);
		}
		return result;
	}
}

class PlayZLogUtcTime
{
	static int NowUnixUtc()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);

		int a = (14 - month) / 12;
		int y = year + 4800 - a;
		int m = month + 12 * a - 3;
		int jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
		int unixDay = jdn - 2440588;
		return unixDay * 86400 + hour * 3600 + minute * 60 + second;
	}

	static string FormatUnixUtc(int unixTime)
	{
		if (unixTime <= 0)
			return "unknown";

		int dayIndex = unixTime / 86400;
		int timeOfDay = unixTime % 86400;
		if (timeOfDay < 0)
		{
			timeOfDay += 86400;
			dayIndex = dayIndex - 1;
		}

		int hour = timeOfDay / 3600;
		int minute = (timeOfDay % 3600) / 60;
		int second = timeOfDay % 60;

		int jdn = dayIndex + 2440588;
		int a = jdn + 32044;
		int b = (4 * a + 3) / 146097;
		int c = a - (146097 * b) / 4;
		int d = (4 * c + 3) / 1461;
		int e = c - (1461 * d) / 4;
		int m = (5 * e + 2) / 153;

		int day = e - (153 * m + 2) / 5 + 1;
		int month = m + 3 - 12 * (m / 10);
		int year = 100 * b + d - 4800 + m / 10;

		string monthText = month.ToStringLen(2);
		string dayText = day.ToStringLen(2);
		string hourText = hour.ToStringLen(2);
		string minuteText = minute.ToStringLen(2);
		string secondText = second.ToStringLen(2);

		return string.Format("%1-%2-%3 %4:%5:%6 UTC", year, monthText, dayText, hourText, minuteText, secondText);
	}
}
