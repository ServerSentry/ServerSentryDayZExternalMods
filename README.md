# ServerSentry DayZ External Mods

Optional integration bridges for the ServerSentry DayZ mod. Each `ServerSentry*` subfolder containing a `config.cpp` is a separate addon. Install only bridges whose third-party dependencies are present; see each bridge's README for its requirements.

Vanilla `DigitalCodeLock` events are handled by the main ServerSentry mod. The optional `ServerSentryCodeLock` bridge is for the third-party RoomService/Wardog CodeLock mod and requires no custom changes to that mod.

## Packing

1. Select one bridge folder, including its `config.cpp`, as the addon source. Do not pack the repository root as a single addon.
2. Set the PBO prefix to the exact value in that folder's `$PBOPREFIX$.txt`. In Addon Builder, enter it explicitly under **Options > Addon prefix** (or pass `-prefix` on the command line). A prefix file alone does not configure every packing tool.
3. Ensure the packing tool includes all `.c` files under `scripts`, preserving their relative folders. In Addon Builder, include `*.c` in **List of files to copy directly**. Add that pattern without removing other existing patterns.
4. Put the resulting PBO in your mod's `addons` folder. Load the core mod, the required third-party mods, and the selected bridges on the server.

| Bridge | PBO prefix |
|---|---|
| BBP | `ServerSentryDayZExternalMods\ServerSentryBBP` |
| BreachingCharge | `ServerSentryDayZExternalMods\ServerSentryBreachingCharge` |
| CodeLock | `ServerSentryDayZExternalMods\ServerSentryCodeLock` |
| DrJones | `ServerSentryDayZExternalMods\ServerSentryDrJones` |
| Expansion | `ServerSentryDayZExternalMods\ServerSentryExpansion` |
| LB ATM | `ServerSentryDayZExternalMods\ServerSentryLBATM` |
| LB Groups | `ServerSentryDayZExternalMods\ServerSentryLBGroups` |
| LB Killfeed | `ServerSentryDayZExternalMods\ServerSentryLBKillfeed` |

These are fixed virtual paths, matching `CfgMods` in each `config.cpp`. Keep the same prefix when the checkout or ZIP extraction folder is named differently, for example `ServerSentryDayZExternalMods-develop` or `ServerSentry`. Renaming the PBO or its installed mod folder does not replace its internal prefix.

For example, the Expansion PBO must contain `scripts\4_World\hooks\ServerSentryExpansionMarketModule.c` beneath the prefix `ServerSentryDayZExternalMods\ServerSentryExpansion`. Its virtual path is then `ServerSentryDayZExternalMods/ServerSentryExpansion/scripts/4_World/hooks/ServerSentryExpansionMarketModule.c`, matching the config.

## Verify after rebuilding

Replace the affected bridge PBOs and restart the DayZ server. A `PBO Load` / `Loaded` message confirms that the archive was mounted; check script behavior as well:

- With LB Groups installed, the script log must register `shop.player.group.clanTag.color` and, unless `LB_DISABLE_CHAT` is defined, `shop.player.chatTag.assign`.
- With LB Killfeed installed, it must register `shop.player.killfeed.color`.
- With Expansion Market installed, test a purchase and a sale and check for the corresponding ServerSentry `market_transaction` events.

Existing PBOs must be rebuilt; updating the Git checkout alone does not update an installed server.
