For TinyRcon's !mute and !unmute (Mute player, Unmute player buttons and context menu commands) admin-level commands 
to work correctly you need to use the patched Call of duty 2 multiplayer game server files, 
in particular you need to use the appropriate version of CoD2MP_s_patched.exe from one of the v1.x folders
for hosting your Call of duty 2 multiplayer v1.x game server.

The !spec pid, !nospec admin-level commands (Spectate player button and context menu commands) work 
with the English version of Call of duty 2 multiplayer. 
The different versions of my patched CoD2MP_s_patched.exe files support the !spec, !nospec commands (Spectate player button and context menu command).
I haven't yet tested the !spec pid and !nospec commands with the other localized language versions of Call of duty 2 multiplayer.

Players using TinyRcon client without the correct rcon password can type in the Administrator prompt: !geoinfo on
to receive from private TinyRcon server additional information about online players (pid, country, city, country flag, is_player_muted).
For example the !spec command requires a player's pid number so that your TinyRcon client, TinyRcon private client or private TinyRcon server 
can execute the minimum required number of (left or right) mouse button clicks in your Call of duty 2 multiplayer game to find the actual player 
whose pid number is the same as the one you provided as argument to the !spec command.
If your using the regular TinyRcon client as a normal user without the correct rcon password then you'll need to execute the !geoinfo on 
command in the Administrator prompt to see the actual pid numbers of all online players.

Also make sure to add 'set g_muted_guids "123456789"' (without '') to your Call of duty 2 game server's server.cfg file.
