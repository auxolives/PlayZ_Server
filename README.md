# PlayZ_Server

Server-only PlayZ mod sources. Packed as PBOs and loaded with `-servermod=` on the dedicated server only — not sent to clients.

## Sub-mods

| Folder | Role |
|--------|------|
| `PlayZLogs` | Server logging (combat, players, vehicles, base build, actions, admin) |
| `PlayZAntiCheat` | Server-side anti-cheat checks |
| `PlayZCF` | CFTools / GameLabs bridge (`#ifdef GAMELABS`) |

Each folder is one addon (`config.cpp` → own PBO).
