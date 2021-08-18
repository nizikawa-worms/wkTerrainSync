# wkTerrainSync v.1.2.1
_Worms Armageddon WormKit module that allows additional W:A terrains to be installed, selected in the Map editor and played online. In addition, the module allows for custom-sized maps to be generated with built-in map generator and custom .WAM missions to be played in both offline and online multiplayer games.
It superseded wkTerrain by introducing means of automatic terrain data synchronization, file transfers, more visual customization features and new gameplay features._

This module has been developed for WA version 3.8.1. It uses pattern scanning to find function offsets.

## Custom Mission Files:
Version 1.2.0 introduces support for custom .WAM files to be played in both offline and online multiplayer games.

A custom mission consists of 3 files:
- Map file in .img/.png/.bit format, for example: missionname.img
- WAM script with mission configuration with the same name as Map file, for example: missionname.wam
- [Optional] .WSC scheme to be used by the mission, with the same name as Map file, for example: missioname.wsc

More info regarding .WAM file format: https://worms2d.info/Worms_Armageddon_mission_file

### Usage
To play a mission, enter Map Editor and select .img/.png/.bit map file. If a .wam file with the same name exists, it will be automatically loaded. If .a .wsc scheme file with the same name exits, it will also be automatically loaded.

After exiting map editor, mission details will be displayed in chat (online multiplayer) or in a message box (offline multiplayer).
Add your teams to match what's displayed in mission description (in the exact same team order) and start the game. You don't need to set the number of worms in team and alliances - the module will do it automatically when you start the game.

If you've added less teams than the mission expects, CPU teams will be added automatically to match the requirement. If you add too many teams, they will be automatically removed.
To reset mission configuration and play a regular game simply generate or load a new map.

For your convenience the module will automatically copy default WA mission files to User/SavedLevels/Mission/WA/ folder (and slightly fix them).
Most WWP missions should be playable in WA - some might be broken because certain events are not present in WA.

By default, the mission files use an empty .WSC scheme file that is later populated by .WAM script when the mission begins - it will be labeled as [ Default mission scheme ]. This scheme can be edited by changing basic parameters such as turn time, round time, etc. It is also possible to enable 3.8 scheme extended options.
Mission files can also provide own .WSC scheme, it will be labeled as [ Mission-provided scheme ].
Note: not all scheme settings are preserved - some values are overwritten by .WAM file.

### Replay compatiblity
Custom mission replay files are fully supported by wkTerrainSync.

Unmodded WA has almost full support for such replays. Online multiplayer missions will raise checksum errors, because all teams will be assigned to the first player in the list. This will be probably fixed in future WA updates.
However, unmodded game will only playback mission replays containing Scheme files in version 1 format and those replays probably won't be supported in future WA updates.

### Extended WAM options
The module adds some custom fields to WAM file format:
```
[HumanTeam] or [CPUTeamX]
Ammo_SkipGo=1
Delay_SkipGo=2
Ammo_Surrender=3
Delay_Surrender=4

[CPUTeamX]
Optional = 1 ; a CPU team will not be spawned if no player team is added to cover this team
TeamNameValue=Team Name Text ; sets custom team name instead of one specified by TeamNameNumber
WormX_NameValue=Worm Name Text ; sets custom worm name instead of one specified by WormX_NameNumber

[EventXXXX]
TypeOfEvent=10 ; show text event
Text_String_Value=Custom text message to be displayed ; sets custom text message instead of one specified by Text_String_Index
```

### Lobby commands
The module implements some commands available in online lobby:
- /mission - show status of currently set mission
- mission attempts 5 - set attempt number of custom mission (used for activation gold/silver/bronze alternative events
- /mission reset - cancel currently loaded mission
- /terrains - send your wkTerrainSync version
- /terrains list - send your wkTerrainSync version and a list of installed terrains
- /terrains query - query wkTerrainSync version used by other players
- /terrains rescan - rescan terrain directory for new terrains
- /scale - show current map scale

## Custom map generator sizes:
Version 1.1.0 introduces support for custom-sized random maps generated with built-in map generator. The maps can be scaled with 0.1x scale increments in both width and height dimensions up to 5.0x scale max.
The module modifies frontend code to add two dropdown menus under map preview with selectable width and height scale.
The scale is applicable only to .LEV maps - i.e. the maps that were generated with map generator and not altered since their generation. Drawing anything on the map preview will cause the map to be immediately converted to .BIT format and its scale reset to default.
Online play on custom sized maps is supported, but all players need to have wkTerrainSync v.1.1.0 (or newer) installed.
The generated maps must be converted to PNG format to be played online with people without wkTerrainSync or previous versions of it installed.

## Custom terrains
wkTerrainSync allows for custom terrains to be selected in Map Editor and played in online games. Only the host player needs to have a custom terrain file - players joining the game will automatically download missing files from the host.

### Data synchronization features:
* Custom network protocol integrated with WA's original network code - this module works seamlessly with WormNAT2 and direct hosting
* Terrains are now referenced by their MD5 hash instead of ID - play online games regardless of your terrain list order and terrain directory names
* File transfers - missing terrain files are automatically downloaded from the host player
* Terrain metadata is embedded directly in .wagame replay files - replay files will work even when terrain list changes or files are renamed
* Map scale metadata is embedded directly in map type metadata - custom-scaled maps use 0xED on first byte (instead of normal 0x02) and map width scale, height scale and optional flags on the next three bytes.

### Terrain customization features:
* Terrain files can include additional parallax sprites - back2.spr displayed in far distance behind regular back.spr ; front.spr displayed in front of the map
* Removed sprite loader limitations when handling back2.spr and front.spr - sprites are no longer limited to 640x160px size and can be animated like regular sprites. For compatiblity, back.spr is loaded with normal loader - to use extendend loader rename the file as _back.spr
* Terrains can override any sprite in the game, including worm animations, weapon projectiles and clouds - place gfx0/spritename.spr (normal palette) and gfx1/spritename.spr (colorblind palette) within terrain.dir to override any sprite
* Terrains can override water.dir for custom water color and animations - place water.dir next to level.dir in terrain directory. Use https://worms2d.info/Water_color_editor to generate custom water.dir

### Usage:
* wkTerrainSync does not require any special configuration or user interaction.
* If you are joining an online game that uses a new terrain file that you currently don't have installed, you will see an "Invalid map file" in map thumbnail and multiple messages about terrain data download will appear in lobby chat. This means the terrain file is being downloaded from the host and a proper map thumbnail will appear once the terrain is downloaded. This should take few seconds depending on network speed.
* When generating a random map by clicking on map thumbnail, the game selects a random terrain from all available terrains.
    - To limit this selection to only custom terrains, hold CTRL key and click on the thumbnail.
    - To limit this selection to only default terrains, hold ALT key and click on the thumbnail.
* Online play with a mix of wkTerrain38 and wkTerrainSync players is not supported. In such case both wkTerrain38 and wkTerrainSync will refuse to load custom terrain maps and wkTerrainSync will print warning messages in lobby chat
* Downloaded terrain directories are stored as "Name #MD5checksum" to avoid file conflicts
* Incomplete file transfers are stored with .part suffix to prevent adding incomplete downloads to the terrain list
* To troubleshoot the module, enable dev console in .ini file and examine the logged messages
* To print the version of the module and a list of available terrains, use /terrains command in lobby

# Important notes:
The official WA maintainers have stated that this module WILL NOT BE SUPPORTED in upcoming versions (3.9.x and newer) of Worms Armageddon. Replays created with wkTerrainSync maps WILL NOT BE COMPATIBLE with WA 3.9.x+ out of the box, however it might be possible to convert the replays to a newer format or create a compatiblity layer wormkit module.
If you care about future preservation of your replay files, convert your maps into PNG format in map editor before playing on them.

# Build:
Use cmake Release or RelWithDebInfo build types with MSVC compiler. Debug build types will not work due to custom calling conventions in some hooked functions.

# DLL Exports:
Version 1.2.0 introduced experimental API for other wormkit modules. See Exports.h for list of available functions.