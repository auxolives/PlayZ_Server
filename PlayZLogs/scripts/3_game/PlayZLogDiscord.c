class PlayZLogWebhookMessage
{
	string Url;
	string Payload;
}

class PlayZLogRestCallback : RestCallback
{
	protected PlayZLogWebhookDispatcher m_Dispatcher;

	void PlayZLogRestCallback(PlayZLogWebhookDispatcher dispatcher)
	{
		m_Dispatcher = dispatcher;
	}

	override void OnSuccess(string data, int dataSize)
	{
		m_Dispatcher.OnRequestFinished();
	}

	override void OnError(int errorCode)
	{
		if (errorCode != 8)
			Print(string.Format("[PlayZLogs] Webhook POST failed with error code %1", errorCode));
		m_Dispatcher.OnRequestFinished();
	}

	override void OnTimeout()
	{
		Print("[PlayZLogs] Webhook POST timed out");
		m_Dispatcher.OnRequestFinished();
	}
}

class PlayZLogWebhookDispatcher
{
	protected ref array<ref PlayZLogWebhookMessage> m_Queue = new array<ref PlayZLogWebhookMessage>;
	protected bool m_IsProcessing;
	protected static ref PlayZLogWebhookDispatcher s_Instance;

	static PlayZLogWebhookDispatcher Get()
	{
		if (!s_Instance)
			s_Instance = new PlayZLogWebhookDispatcher();
		return s_Instance;
	}

	void Enqueue(string url, string payload)
	{
		if (url == "" || payload == "")
			return;

		ref PlayZLogWebhookMessage message = new PlayZLogWebhookMessage();
		message.Url = url;
		message.Payload = payload;
		m_Queue.Insert(message);
		ProcessQueue();
	}

	void ProcessQueue()
	{
		if (m_IsProcessing || m_Queue.Count() == 0)
			return;

		if (!GetGame() || !GetGame().IsServer())
		{
			m_Queue.Clear();
			return;
		}

		ref PlayZLogWebhookMessage nextMessage = m_Queue[0];
		m_Queue.RemoveOrdered(0);

		RestApi api = GetRestApi();
		if (!api)
			api = CreateRestApi();

		if (!api)
		{
			Print("[PlayZLogs] Unable to acquire RestApi for webhook dispatch");
			return;
		}

		RestContext ctx = api.GetRestContext(nextMessage.Url);
		if (!ctx)
		{
			Print("[PlayZLogs] Unable to obtain RestContext for webhook URL");
			return;
		}

		m_IsProcessing = true;
		ctx.SetHeader("application/json");
		RestCallback cb = new PlayZLogRestCallback(this);
		ctx.POST(cb, "", nextMessage.Payload);
	}

	void OnRequestFinished()
	{
		m_IsProcessing = false;
		if (m_Queue.Count() > 0)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ProcessQueue, 500, false);
	}
}

class PlayZLogDiscordEmbedField
{
	string name;
	string value;
	bool inline;
}

class PlayZLogDiscordEmbed
{
	string title;
	string description;
	int color;
	ref array<ref PlayZLogDiscordEmbedField> fields;
}

class PlayZLogDiscordWebhookPayload
{
	string username;
	ref array<ref PlayZLogDiscordEmbed> embeds;
}

class PlayZLogDiscord
{
	static string FormatFloat(float value, int decimals)
	{
		float pow = Math.Pow(10.0, decimals);
		float rounded = Math.Round(value * pow) / pow;

		string text = rounded.ToString();
		if (decimals <= 0)
			return text;

		int dotIndex = text.IndexOf(".");
		if (dotIndex == -1)
		{
			text = text + ".";
			dotIndex = text.Length() - 1;
		}

		while (text.Length() - dotIndex - 1 < decimals)
			text = text + "0";

		return text;
	}

	static string FormatPosition(vector pos)
	{
		return string.Format("(%1, %2, %3)", FormatFloat(pos[0], 2), FormatFloat(pos[1], 2), FormatFloat(pos[2], 2));
	}

	static void AddEmbedField(ref array<ref PlayZLogDiscordEmbedField> fields, string name, string value, bool inline = false)
	{
		if (!fields)
			fields = new array<ref PlayZLogDiscordEmbedField>();

		ref PlayZLogDiscordEmbedField field = new PlayZLogDiscordEmbedField();
		field.name = name;
		field.value = value;
		field.inline = inline;
		fields.Insert(field);
	}

	static string BuildEmbedPayload(string username, string title, string description, int color, ref array<ref PlayZLogDiscordEmbedField> fields)
	{
		JsonSerializer serializer = new JsonSerializer();
		ref PlayZLogDiscordWebhookPayload payload = new PlayZLogDiscordWebhookPayload();
		payload.username = username;
		payload.embeds = new array<ref PlayZLogDiscordEmbed>();

		if (!fields)
			fields = new array<ref PlayZLogDiscordEmbedField>();

		ref PlayZLogDiscordEmbed embed = new PlayZLogDiscordEmbed();
		embed.title = title;
		embed.description = description;
		embed.color = color;
		embed.fields = fields;
		payload.embeds.Insert(embed);

		string json;
		if (!serializer.WriteToString(payload, false, json))
			return "";

		return json;
	}

	static void Send(string webhookUrl, string title, string description, ref array<ref PlayZLogDiscordEmbedField> fields, int color = 0x3498DB)
	{
		if (webhookUrl == "")
			return;

		string payload = BuildEmbedPayload("PlayZ Logs", title, description, color, fields);
		if (payload != "")
			PlayZLogWebhookDispatcher.Get().Enqueue(webhookUrl, payload);
	}
}
