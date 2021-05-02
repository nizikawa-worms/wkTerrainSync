# wkTerrainSync v.1.1.0
_Worms Armageddon WormKit module that allows additional W:A terrains to be installed and selected in the Map editor. It superseded wkTerrain38 adding new features such as data synchronization for network games and sprite addition/replacement_

This module has been developed for WA version 3.8.1. It uses pattern scanning to find function offsets, so it might work with newer releases.

## What's new:
Version 1.1.0 introduces support for custom-sized random maps generated with built-in map generator. The maps can be scaled with 0.1x scale increments in both width and height dimensions up to 5.0x scale max.
The module modifies frontend code to add two dropdown menus under map preview with selectable width and height scale.
The scale is applicable only to .LEV maps - i.e. the maps that were generated with map generator and not altered since their generation. Drawing anything on the map preview will cause the map to be immediately converted to .BIT format and its scale reset to default.
Online play on custom sized maps is supported, but all players need to have wkTerrainSync v.1.1.0 (or newer) installed.
The generated maps must be converted to PNG format to be played online with people without wkTerrainSync or previous versions of it installed.

## Important notes:
The official WA maintainers have stated that this module WILL NOT BE SUPPORTED in upcoming versions (3.9.x and newer) of Worms Armageddon. Replays created with wkTerrainSync maps WILL NOT BE COMPATIBLE with WA 3.9.x+ out of the box, however it might be possible to convert the replays to a newer format or create a compatiblity layer wormkit module.
If you care about future preservation of your replay files, convert your maps into PNG format in map editor before playing on them.


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

## Usage:
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

## Building:
Use cmake Release or RelWithDebInfo build types with MSVC compiler. Debug build types will not work due to custom calling conventions in some hooked functions.
