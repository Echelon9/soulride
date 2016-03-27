# Configuration

The Soul Ride engine contains a number of tunable parameters and debug settings.
Broadly, this document refers to each as a 'configuration' setting.

These can be set in a number of different manners:
  * Console, during runtime;
  * Command line, at program start "soulride option1=value1 option2=value2 ...";
  * config.txt file; or
  * Lua script file, for example within a mountain's preload.lua

The Soul Ride engine adopts an integrated approach to configuration settings,
with all approaches plumbing the configuration through to one internal Lua
scripting engine state.

Accordingly all configurations can be dynamically modified at runtime via the
console.

## General

| Configuration                                  | Type        | Comments                                               |
| ---------------------------------------------- | ----------- | ------------------------------------------------------ |
| DefaultMountain                                | String      | Name of the mountain, e.g. "Jay_Peak" or "Mammoth"     |
| Language                                       | String      | Default "en"                                           |
| SkipIntro                                      | Boolean     |                                                        |

## Renderer

| Configuration                                  | Type        | Comments |
| ---------------------------------------------- | ----------- | -------- |
| ForceSoftwareRenderer                          | Boolean     |          |
| OGLCheckErrorLevel                             | Integer     |          |
| OGLDriverIndex                                 | Integer     |          |
| OGLLibOverride                                 | String      |          |
| OGLModeIndex                                   | Integer     |          |
| SoftwareRenderingOptions                       | Boolean     |          |

## Graphics

| Configuration                                  | Type        | Comments |
| ---------------------------------------------- | ----------- | -------- |
| BlitUVOffset                                   | Float       |          |
| BoarderShadow                                  | Boolean     |          |
| DetailMapping                                  | Boolean     |          |
| DiffuseLightingFactor                          | Float       |          |
| FigureWireframe                                | Boolean     |          |
| Fog                                            | Boolean     |          |
| Fullscreen                                     | Boolean     |          |
| MIPMapping                                     | Boolean     |          |
| MIPMapLODBias                                  | Float       |          |
| NoLighting                                     | Boolean     |          |
| NoTerrainTexture                               | Boolean     |          |
| Particles                                      | Boolean     |          |
| ShadowLightingFactor                           | Float       |          |
| SurfaceNodeBuildLimit                          | Integer     |          |
| TerrainMeshSlider                              | Float       |          |
| TerrainShadow                                  | Boolean     |          |
| TextureDetailSlider                            | Integer     |          |
| ViewAngle                                      | Float       |          |
| Wireframe                                      | Boolean     |          |
| ZSinglePass                                    | Boolean     |          |

## Weather and Environment

| Configuration                                  | Type        | Comments                                  |
| ---------------------------------------------- | ----------- | ----------------------------------------- |
| Clouds                                         | Boolean     | Enable cloud layer                        |
| Cloud0UVRepeat                                 | Float       |                                           |
| Cloud1UVRepeat                                 | Float       |                                           |
| Cloud0XSpeed                                   | Float       |                                           |
| Cloud0ZSpeed                                   | Float       |                                           |
| Cloud1XSpeed                                   | Float       |                                           |
| Cloud1ZSpeed                                   | Float       |                                           |
| FogDistance                                    | Float       | Set visibility distance                   |
| Snowfall                                       | Boolean     | Enable snow                               |
| SnowDensity                                    | Float       | Set snow density (flakes per cubic meter) |
| SunPhi                                         | Float       |                                           |
| SunTheta                                       | Float       |                                           |

## Recorder

| Configuration                                  | Type        | Comments |
| ---------------------------------------------- | ----------- | -------- |
| RecordMoviePath                                | String      |          |
| RecordMoviePause                               | Boolean     |          |
| SaveFramePPM                                   | String      |          |

## Audio

| Configuration                                  | Type        | Comments |
| ---------------------------------------------- | ----------- | -------- |
| CDAudioDrive                                   | String      |          |
| CDAudioDevice                                  | String      |          |
| CDAudioMountPoint                              | String      |          |
| CDAudioVolume                                  | Float       |          |
| MasterVolume                                   | Float       |          |
| Music                                          | Boolean     |          |
| SFXVolume                                      | Float       |          |
| Sound                                          | Boolean     |          |
| TestCDAIndex                                   | String      |          |

## Input Controllers

| Configuration                                  | Type        | Comments                                                               |
| ---------------------------------------------- | ----------- | ---------------------------------------------------------------------- |
| DirectInput                                    | Boolean     | (Windows only) Enable DirectInput to read the joystick                 |
| ForceMouseSteering                             | Boolean     | Force mouse steering, regardless of `Fullscreen` (see `MouseSteering`) |
| JoystickPort                                   | Integer     |                                                                        |
| JoystickRudder                                 | Boolean     |                                                                        |
| MouseSteering                                  | Boolean     | Enable mouse steering. Requires `Fullscreen` to be True as well        |
| MouseSteeringSensitivity                       | Float       |                                                                        |

## Helicam

| Configuration                                  | Type        | Comments |
| ---------------------------------------------- | ----------- | -------- |
| HelicamCenterX                                 | Float       |          |
| HelicamCenterZ                                 | Float       |          |
| HelicamDefaultAngle                            | Float       |          |

## Debug

| Configuration                                  | Type        | Comments                                          |
| ---------------------------------------------- | ----------- | ------------------------------------------------- |
| AllowAllRuns                                   | Boolean     | Unlock all runs on current mountain               |
| Camera                                         | Boolean     |                                                   |
| ConsoleHeight                                  | Integer     |                                                   |
| DebugSteer                                     | Integer     |                                                   |
| EnableJets                                     | Boolean     |                                                   |
| F4Pressed                                      | Boolean     |                                                   |
| LastUpdateBeforeRender                         | Boolean     |                                                   |
| LimitUpdateRate                                | Boolean     |                                                   |
| MultiplayerEnabled                             | Boolean     | Enable local multiplayer mode menu (incomplete)   |
| PerfTest                                       | Boolean     | Enable benchmark mode (requires PerfTestPath.dat) |
| PlayerCameraMode                               | Integer     |                                                   |
| RewindsAllowed                                 | Integer     |                                                   |
| ShowBoarderFeet                                | Boolean     | Show boarder's feet friction points               |
| ShowBoarderRays                                | Boolean     |                                                   |
| ShowCacheUsage                                 | Boolean     |                                                   |
| ShowCheckCode                                  | Boolean     |                                                   |
| ShowFrameRate                                  | Boolean     |                                                   |
| ShowRenderStats                                | Boolean     |                                                   |
| ShowViewerLocation                             | Boolean     |                                                   |
| Speedometer                                    | Boolean     |                                                   |
| UserFatigue                                    | Float       |                                                   |
| UserStability                                  | Float       |                                                   |

## Deprecated

| Configuration                                  | Type        | Comments                       |
| ---------------------------------------------- | ----------- | ------------------------------ |
| DisableDetailMapping                           | Boolean     | Use `DetailMapping` instead    |
| KeyRepeat                                      | Boolean     | Unutilized                     |
| OptikosPath                                    | String      | Related to 3D Optikos displays |

