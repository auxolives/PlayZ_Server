# PlayZ_Server

Server-only PlayZ sub-mods. Everything here is packed into PBOs that ship to the **dedicated server only** — never to clients.

## Rules

- Only `modded class` overrides and static/server utilities. **No new entity `class` configs** (anything with `scope=2; model=...;`) — those would fail to spawn on clients and cause desync.
- No `SetSynchDirty`, no NetSync vars. RPCs between server-only mods and clients are allowed but only via the **split-mod pattern** below — the client must carry a tiny shared stub that declares the matching RPC constants and handler.
- Every sub-mod here must have its own `config.cpp` with its own `CfgPatches.<Name>` entry so it packs as an independent PBO.
- `requiredAddons[]` may list engine addons (`DZ_Data`, `DZ_Scripts`), other `PlayZ_Server/` sub-mods, or `PlayZ_Client/` sub-mods (to pick up shared RPC constants and data types). The server has both chains loaded so this direction is safe. **The reverse is forbidden** — no `PlayZ_Client/` mod may require a `PlayZ_Server/` mod.

## Split-mod pattern (when a feature needs both sides)

Some server features must trigger code on the client — e.g. to sample `GetCurrentCameraPosition()` or any other client-only API. Keep the client footprint to the absolute minimum:

- `PlayZ_Client/<Feature>Client/` (client + server) — shared RPC constants and the client-side RPC handler that samples state and echoes it back. Nothing else.
- `PlayZ_Server/<Feature>/` (server only) — all logic, I/O, config, scheduling, detection. Lists `"<Feature>Client"` in `requiredAddons[]`.

## Deployment

Dedicated server launch:

```text
DayZServer_x64.exe -mod="@PlayZCore;..." -servermod="@PlayZLogs;@..." -config=serverDZ.cfg
```

Clients only receive the `-mod=` chain. The `-servermod=` chain is never advertised to clients, never downloaded, and never compiled into the client VM.

## Current sub-mods

- `PlayZLogs` — server-side telemetry/logging (damage, people, vehicles, weather, base-building, actions, admin, AI engagement).
- `PlayZAntiCheat` — ballistic anomaly detection and camera spot-check. Requires `PlayZ_Client/PlayZAntiCheatClient` on `-mod=` (RPC stub).
- `PlayZCF` — CFTools / GameLabs integration. Server-only by construction: every file is wrapped in `#ifdef GAMELABS` and the mod lists `"GameLabs_Scripts"` (server-only) in `requiredAddons[]`. Hooks into `GLProcessKill`, `ActionManagerServer.OnActionEnd`, `PluginAdminLog`, `MissionServer.OnInit`, and `EEInit`/`EEDelete` on a curated set of `House` / `ItemBase` types to register GameLabs map events and upstream PlayerDeath / ItemInteract payloads.

## Adding a new server-only sub-mod

1. Confirm it has **zero** client-observable side effects (no synced state, no UI, no entity configs).
2. Grep `PlayZ_Client/` for references to its classes/statics. If any exist, either inline those references with `if (GetGame().IsServer())` guards that don't depend on the class existing, or keep the caller in `PlayZ_Server/` too.
3. Give it its own `config.cpp` + `CfgPatches.<Name>` + `CfgMods.<Name>` entry.
4. Pack as its own PBO and add to `-servermod=` chain.
