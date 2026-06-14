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

### Pack PBOs

Mikero mirror: `P:\PlayZ_Server\` (map P: to this tree or edit `SOURCE_ROOT` in `.vscode/scripts/Build-PlayZ_Server.bat`).

| Action | Steps |
|--------|--------|
| **All server addons** | `Ctrl+Shift+P` → **Tasks: Run Task** → **PlayZ: Build Server PBOs (all)** |
| **One addon** | **Tasks: Run Task** → **PlayZ: Build Server PBO (one addon)** → pick addon |

Output defaults to `@PlayZ_Server` on the Sakhal CFTools deployment (`MOD_OUT` in the batch file). Default build task (`Ctrl+Shift+B`) still packs **client** PBOs only.

## Current sub-mods

- `PlayZLogs` — server-side telemetry/logging (damage, people, vehicles, weather, base-building, actions, admin, AI engagement).
- `PlayZAntiCheat` — ballistic, camera, speed, and teleport detection. Requires `PlayZ_Client/PlayZAntiCheatClient` on `-mod=` (RPC stub). Config: `$profile:PlayZ/AntiCheat.json`.

  **Discord webhooks (four channels):**
  - `DiscordWebhookUrl` — ballistic anomalies
  - `DiscordCameraWebhookUrl` — camera anomalies (falls back to ballistic URL)
  - `DiscordMovementWebhookUrl` — speed + teleport anomalies (falls back to ballistic URL)
  - `DiscordKeybindWebhookUrl` — suspicious keybind presses (falls back to ballistic URL)

  **Movement detection** (spot-check RPC, same interval as camera):
  - `EnableSpeedChecks` — server-only implied speed; 3 consecutive over-cap samples → kick
  - `EnableTeleportChecks` — client-corroborated position jumps; critical tier (≥ `TeleportCriticalMeters`, default 150 m) instant kick; suspicious tier (≥ 75 m) needs 4 consecutive samples
  - `EnableMovementKick` — kick (not ban) with reason `SPEEDHACK SUSPICION REPORTED` (main-menu warning dialog via client RPC; no in-game notification)
  - `ExcludedSteamIds64` — admin Steam64 IDs skipped for all checks

  **Keybind detection** (client `MissionGameplay`, rate-limited RPC):
  - `EnableKeybindChecks` — log + Discord when monitored keys are pressed (navigation/editing keys, numpad ops, brackets/backslash, Print Screen, F2–F12 with Left/Right Shift/Alt/Ctrl and Ctrl+Shift combos)
  - Client compacts all presses into **at most one RPC per second** per player; per-key counts cap at `999+`
  - `KeybindAlertCooldownSeconds` — server-side minimum seconds between log + webhook per player (default 1; blocks modified-client floods)
  - `KeybindMaxPayloadChars` — truncate oversized RPC payloads (default 256)
  - `ExcludedKeybindSteamIds64` — Steam64 IDs skipped for keybind alerts only; does **not** skip ballistic/camera/movement. `ExcludedSteamIds64` admins are auto-skipped for keybind too (one-way — do not duplicate admin IDs here)
  - Alert-only (no kick)

  Ship with `EnableSpeedChecks: 0` until staging validation; set dedicated webhook URLs on the server profile only (do not commit live webhook URLs to the public repo).
- `PlayZCF` — CFTools / GameLabs integration. Server-only by construction: every file is wrapped in `#ifdef GAMELABS` and the mod lists `"GameLabs_Scripts"` (server-only) in `requiredAddons[]`. Hooks into `GLProcessKill`, `ActionManagerServer.OnActionEnd`, `PluginAdminLog`, `MissionServer.OnInit`, and `EEInit`/`EEDelete` on a curated set of `House` / `ItemBase` types to register GameLabs map events and upstream PlayerDeath / ItemInteract payloads.

## Adding a new server-only sub-mod

1. Confirm it has **zero** client-observable side effects (no synced state, no UI, no entity configs).
2. Grep `PlayZ_Client/` for references to its classes/statics. If any exist, either inline those references with `if (GetGame().IsServer())` guards that don't depend on the class existing, or keep the caller in `PlayZ_Server/` too.
3. Give it its own `config.cpp` + `CfgPatches.<Name>` + `CfgMods.<Name>` entry.
4. Pack as its own PBO and add to `-servermod=` chain.
