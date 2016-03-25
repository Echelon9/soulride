# Lua API

The Soul Ride engine uses the Lua language (version 3.2, see http://www.lua.org)
as an embedded scripting language.

The scripting language can be accessed through Lua script files and the console.

## Soul Ride Engine Lua API

In addition to valid Lua expressions, the Soul Engine exposes additional
API access to certain internal functions and states.

| Lua API                                    | Internal Function        |
| ------------------------------------------ | ------------------------ |
| _ALERT(string)                             | lua_console_alert()      |
| ARGB(string)                               | ARGB_lua()               |
| ARGB(number, number, number, number)       | ARGB_lua()               |
| debug_watch(string)                        | DebugWatch_lua()         |
| game_get_current_run()                     | GetCurrentRun_lua()      |
| game_run_completed(number)                 | RunCompleted_lua()       |
| music_play_next()                          | PlayNext()               |
| overlay_play_movie(string, number, number) | PlayMovie_lua()          |
| overlay_stop_movie(number)                 | StopMovie_lua()          |
| print(string1, string2, ...)               | lua_console_print()      |
| qsquare_heap_info()                        | HeapInfo_lua()           |
| recording_get_mode()                       | GetMode_lua()            |
| recording_set_mode(number)                 | SetMode_lua()            |
| shadetable_reset(string)                   | ShadeTableReset_lua()    |
| terrain_rebuild_lightmaps()                | RebuildLightmaps_lua()   |
| ui_show_finish()                           | ShowFinish_lua()         |
| weather_recalc_sun_direction()             | RecalcSunDirection_lua() |
| weather_reset(string)                      | WeatherReset_lua()       |


## Lua Script Files

Locations that the engine will look for Lua script files to run are:

Upon engine startup:
  * startup.lua
  * data/preload.lua

Specific for each weather type:
  * data/clear.lua
  * data/cloudy.lua
  * data/sunset.lua
  * data/snowing.lua
  * data/whiteout.lua

Specific for each mountain:
  * data/..../preload.lua
  * data/..../postload.lua
