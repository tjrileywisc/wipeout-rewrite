# Player Profiles — Design

## Intent

Several people share one machine. Profiles let each of them keep their own
Hall of Fame times apart without re-typing a name every time, and add a bit
of personal customization (engine color) that makes split-screen races more
interesting and ships easier to tell apart.

## Goals

- Saved profiles, each with a 1–3 character code (same charset and entry
  rules as the Hall of Fame today) and an RGB engine color.
- Players pick a profile (or none) before a single player race and in the
  split-screen lobby.
- With a profile active:
  - Hall of Fame entries use the profile's code; no name entry.
  - The marker above the player's ship shows the code instead of "P1".."P4".
  - The engine plume, plume bloom glow, and engine light use the profile color.
- Without a profile: default engine color, "Pn" marker, Hall of Fame name
  entry exactly as today.
- A new game session starts with the last used single player profile selected.

## Non-goals

- Network races: remote players keep their server-generated names. A local
  profile still colors the local ship and its marker, nothing is sent over
  the network.
- Per-profile settings (controls, options) or per-profile statistics beyond
  the existing Hall of Fame.
- Importing/exporting profiles between machines.

## Anti-goals

- Making the split-screen racer selection page busier than one extra line per
  player column. Creating/editing/deleting profiles never happens there.
- Breaking existing `save.dat` files.

## Approaches considered

- **A. Select, create and edit inline in the race flow.** Lowest friction for
  a new player, but grows the race selection pages.
- **B. Manage in Options > Profiles; race flow only selects.** *Chosen* —
  keeps the multiplayer racer selection page to a fixed, small scope.
- **C. Like A, stored in a separate `profiles.dat`.** Extra load/save path for
  no current benefit.

## Design

### Data (game.h / game.c)

```c
#define NUM_PROFILES 8

typedef struct {
	bool used;
	char code[4];    // 1-3 chars from the hall of fame charset, NUL terminated
	uint8_t color[3]; // r, g, b
} profile_t;
```

Appended to the end of `save_t` (after `track_lighting`):

```c
profile_t profiles[NUM_PROFILES];
int last_profile; // single player; -1 = none
```

Add `offsetof(save_t, profiles)` to the accepted save sizes in `game.c`, and
default `last_profile = -1` with all slots unused.

Runtime: `int profile[INPUT_MAX_PLAYERS]` in `game_t` (per local player, -1 =
none). Set from `save.last_profile` at startup (validated: slot must be
used). Ships get the color via the local player index, i.e.
`g.profile[ship->player_index]`.

### Options > Profiles (main_menu.c)

New "PROFILES" button in the options page. The profiles page lists used
slots by code plus "NEW PROFILE" (hidden when all 8 slots are used).

- Selecting a profile opens its edit page: CODE, RED, GREEN, BLUE, DELETE.
- CODE opens a code entry page reusing the Hall of Fame letter selector
  (A–Z, 0–9, DEL, END; 1–3 characters). END with zero characters is not
  allowed (same as today).
- RED/GREEN/BLUE are toggles with 16 steps each, mapping to
  0, 17, 34 … 255.
- The edit page draws a spinning ship model with its plume in the current
  color as a live preview.
- DELETE asks for confirmation. Existing Hall of Fame entries keep the code
  (they are plain strings).
- NEW PROFILE goes straight to code entry, then the edit page; default color
  is the current default plume color.
- Codes must be unique among profiles (times are keyed by code). Entering a
  duplicate shows "CODE IN USE" and stays in entry.

The letter selector currently lives as static state in `ingame_menus.c`;
factor it into a small reusable function (e.g. in `ui.c` or a new
`name_entry.c`) used by both the Hall of Fame and profile code entry.

### Single player selection

A "SELECT PROFILE" page between START GAME and SELECT RACE TYPE. Lists used
profiles plus "NO PROFILE". The cursor starts on `save.last_profile` (or
NO PROFILE), so with a remembered profile one press continues. Choosing sets
`g.profile[0]` and `save.last_profile` (marks save dirty). If no profiles
exist the page is skipped.

### Split-screen lobby

Each player column gets one PROFILE line under the player label (shows code
or "NONE"):

- Player 1: new lobby row `LOBBY_ROW_PROFILE` after `LOBBY_ROW_RACER`;
  left/right cycles.
- Players 2–4: up/down on their pad cycles (left/right still cycles racer).
- A profile can be held by only one player at a time; cycling skips taken
  ones, joining starts on NONE. P1 starts on `g.profile[0]`.
- `split_screen_start()` copies the chosen profiles into `g.profile[]` in the
  same compacted order as the pilots. The lobby does not change
  `save.last_profile`.

### Hall of Fame (ingame_menus.c, race.c)

- `race.c` already picks the fastest local player for `g.race_time`; also
  record that player's index (`g.race_player`).
- If `g.profile[g.race_player]` is set: the table is shown with the new entry
  pre-filled with the profile's code and completed — no letter selector;
  SELECT/START continues to the same next step as today.
- Otherwise: name entry as today, pre-filled from `save.highscores_name`.

### Markers (hud.c)

`hud_draw_player_markers` uses the profile code when the ship's player has a
profile, else "Pn".

### Engine color (ship.c)

- `ship_init_exhaust_plume` / a new `ship_set_engine_color(ship, rgb)` set
  the plume vertex colors (alpha stays 140).
- `ship_draw_plume_glow` and the engine light use a per-ship color stored on
  `ship_t` instead of the file-level constants.
- Defaults (no profile) remain exactly today's three constants. Profile
  color: plume = color with alpha 140, glow = color, light = color / 255 *
  the default light's brightness (max component of the default, 1.2).
- Applied in race init after pilots are assigned to players; reset to
  defaults on returning to the menu (`ships_reset_exhaust_plumes`).

## Left to the builder

- Exact page layouts/positions, matching existing menu styling.
- Where the shared letter selector lives.
- Whether light brightness scaling needs tuning so dark colors still read.
