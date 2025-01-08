For TinyRcon's !mute and !unmute (Mute player, Unmute player buttons and context menu commands) admin-level commands 
to work correctly you need to use the patched Call of duty 2 multiplayer game server files, 
in particular you need to use the appropriate version of CoD2MP_s_patched.exe from one of the v1.x folders
for running your Call of duty 2 multiplayer v1.x game server.

The !spec pid, !nospec admin-level commands (Spectate player button and context menu commands) work 
with the English version of Call of duty 2 multiplayer. 
The different versions of my patched CoD2MP_s_patched.exe files support the !spec, !nospec commands (Spectate player button and context menu command).
I haven't yet tested it with the other existing language version of the game.

Players using TinyRcon client without the correct rcon password can type in the Administrator prompt: '!geoinfo on'
(without '') to receive additional information about online players (pid, country, city, country flag, is_player_muted) 
from private TinyRcon server.

Also make sure to add 'set g_muted_guids "123456789"' (without '') to your Call of duty 2 game server's server.cfg file.