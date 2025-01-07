# TinyRcon server, private TinyRcon server, TinyRcon client, private TinyRcon client and private TinyRcon client v2

TinyRcon is a simple, easy to use graphical application developed using modern C++ language features, the Win32 API, 
  the nlohmann custom json parser header only C++ library and the standalone version of the ASIO C++ networking library. 
  The overall TinyRcon application project is a Visual Studio solution which consists of 5 distinct projects: 

  ![TinyRcon client for authorized public admins](/screenshots/TinyRcon_client.jpg)  
 
  1. TinyRcon client which runs on an authorized game server admin's computer. It requires a correct rcon password to work with a running Call of duty game server 
   as well as to be able to log in to TinyRcon server. TinyRcon client communicates with both TinyRcon server and private TinyRcon server but it only needs the correct rcon password for sending rcon commands to configured game server and for communicating with TinyRcon server. An authorized regular admin cannot log in to TinyRcon server without the correct rcon password. Private TinyRcon server is used by regular (public) admins,  private admins and players for receiving additional information about maps, images, etc.).

   ---

   ![Private TinyRcon client for authorized private admins](/screenshots/Private_TinyRcon_client.jpg)
  
  2. Private TinyRcon client which runs on an authorized private admin's computer. This rcon tool doesn't need the correct rcon password at all to be able to execute admin level commands and send rcon commands to configured game server. It works without an actual rcon password by logging the private admin in to a private TinyRcon server using their unique username/password combination. The username/password information of registered private admin accounts is stored in a file named 'private_admins.cfg' which is located in the data folder of private TinyRcon server. Every private admin has their own username, country and unique password separated by \ characters on one line as shown in the following example:

     pol78\greece\6SiLL}DjTUmTgYA9<zDjNAM2yTFzXF>f
     In this example the private admin's username is pol78 and his password is "6SiLL}DjTUmTgYA9<zDjNAM2yTFzXF>f" (without "").

   ---

   ![Private TinyRcon client v2 for authorized private admins](/screenshots/Private_TinyRcon_client_v2.jpg)
 
  3. Private TinyRcon client v2 (using a rich text edit control instead of a grid control) is identical to private TinyRcon client except for a couple of differences regarding its main graphical user interface. It runs on an authorized private admin's computer. This rcon tool doesn't need the correct rcon password at all to be able to execute admin level commands. It works without an actual rcon password by logging the private admin in to a private TinyRcon server using their unique username/password combination. The username/password information of registered private admin accounts is stored in a file named 'private_admins.cfg' which is located in the data folder of private TinyRcon server. Every private admin has their own username, country and unique password separated by \ characters on one line as shown in the following example:

     pol78\greece\6SiLL}DjTUmTgYA9<zDjNAM2yTFzXF>f
     In this example the private admin's username is pol78 and his password is "6SiLL}DjTUmTgYA9<zDjNAM2yTFzXF>f" (without "").

   ---

   ![TinyRcon server for allowing admin level access to authorized admins](/screenshots/TinyRcon_server.jpg)
  
  4. TinyRcon server which can optionally be running on the same authorized public admin's computer as well, although that's not a requirement. 
     TinyRcon client connects to it and exchanges information with it. An authorized public admin needs the correct rcon password to be able to log in to TinyRcon server 
     and send valid rcon commands to configured game server.

   ---

   ![Private TinyRcon server for managing game server and allowing admin level access to authorized private admins](/screenshots/Private_TinyRcon_server.jpg)
  
  5. Private TinyRcon server which can optionally be running on the same authorized public admin's computer as well, although that's not a requirement. Both TinyRcon and private TinyRcon client exchange messages with private TinyRcon server. Private TinyRcon server looks almost identical to TinyRcon client. Apart from its main task of communicating with private TinyRcon clients and processing private admin level commands of logged in private admins as well as handling user level commands of players some of its additional services include processing custom user level commands like !s, !tp, !report, !unreport, !reports and sending available custom maps' relation information to both TinyRcon and private TinyRcon client as well.
    
**TinyRcon client** communicates with both **Public TinyRcon server** and **Private TinyRcon server** and **Private TinyRcon client** only communicates with **Private TinyRcon server** by exchanging short messages based upon the UDP transport protocol. TinyRcon clients are able to keep their temporary bans, IP bans, IP address range bans, city bans, 
   country bans, player name bans, etc. and other important information like protected player entries updated and synchronized among all TinyRcon clients by logging in 
   to the same TinyRcon server. The project is currently under active development and it is not a cross-platform GUI application at the moment.  
   I plan to to make it cross-platform in the nearby future.

---

## Installation:
Before building the whole Visual Studio solution which consists of the 5 different TinyRcon application related projects or building only those TinyRcon projects 
  that you are interested in you need to have Visual Studio 2022 (with the 'Desktop development with C++' workload and 'C++ ATL for latest v143 C++ (x86 & x64) build tools' individual component selected) installed on your computer. 
  You can also build the TinyRcon Visual Studio projects using the Visual Studio 2022 C++ Build Tools from a x86 Native Tools Command Prompt for VS 2022 or Developer Command Prompt for VS 2022.

1. Double click on TinyRconPrivateServerAndClientApplications.sln or launch Visual Studio 2022, click Open -> Project/Solution -> choose TinyRconPrivateServerAndClientApplications.sln and press the Open button.
2. Important: change the **build mode** to **Release** mode and set the **architecture type** to **x86**.
3. If you wish to build all TinyRcon application related projects right-click on Solution 'TinyRconPrivateServerAndClientApplications' (5 projects) and then choose 'Build Solution' command or just simply press F7.
  If you wish to only build a certain TinyRcon project select the desired Visual Studio project by left-clicking on its name in the Solution Explorer, right-click on the project's name and then choose the Build command from the context menu.

Every successfully built executable will be located in the root folder of the appropriate Visual Studio project. For example if you build the 'TinyRcon client' Visual Studio project there will be an executable file named TinyRcon.exe located inside of the TinyRconClient project's folder.

---

## Configuration:


Every TinyRcon client and TinyRcon server has a config folder which contains a json configuration file named 'tinyrcon.json'. 
Before launching any of the appropriate TinyRcon client applications (TinyRcon client, private TinyRcon client) and/or TinyRcon server applications 
   (TinyRcon server, private TinyRcon server) make sure to edit the appropriate 'tinyrcon.json' configuration file(s).
   

a. For **TinyRcon client** you need to configure the following important Call of duty 1/2/4/5 multiplayer game server and public/private TinyRcon server application related settings:
- your user name (username - change your user name from ^1Admin to your preferred user name. An authorized admin can still use the generic ^1Admin name but regular TinyRcon users/players without the correct rcon password should change their user name to their player name.)
- game server's name (game_server_name - game server's sv_hostname value, it's an optional setting, you can leave it at its default value: "CoDHost")
- game server's IP address (game_server_ip_address)
- game server's port (game_server_port)
- game server's rcon password (rcon_password - you can omit this setting if you intend to use TinyRcon client as a regular user/player)
- game server's private password (private_slot_password - optional setting, leave it empty if your Call of duty multiplayer game server does not use private slots)
- TinyRcon server's IP address (tiny_rcon_server_ip_address - used for communicating with regular TinyRcon clients used by authorized admins that have the correct rcon_password)
- TinyRcon server's port number (tiny_rcon_server_port - used for communicating with regular TinyRcon clients used by authorized admins that have the correct rcon_password. 
  Don't change it unless you need to. Leave it at its default value: 27015)
- Private TinyRcon server's IP address (private_tiny_rcon_server_ip_address - it is used by admins and players to receive additional information 
  (custom message, images, information about available custom maps, players' stats data, top players' stats data, executing !report, !unreport, !reports, !s, !tp commands, etc.) 
  for communicating with private TinyRcon server) 
  Private TinyRcon server's port number (private_tiny_rcon_server_port - it is used for communicating with private TinyRcon server.  Don't change it unless you need to. 
  Leave it at its default value: 27017)
  Regular users, players, too, can use TinyRcon client without having access to the game server's correct rcon_password. 
  They won't have access to any of the admin level user and rcon commands but they will still be able to execute regular user level commands.
  Players with no access to the correct rcon_password may use TinyRcon client as a general game server browser similar to Hlsw, CoD Rcon Tool, Mohaa's Autokick v1 
  or Call of duty's server browser game feature. 
  Players can execute all user level commands that don't need the correct rcon_password.
  
  ---
 b. For **private TinyRcon client** you need to configure the following important Call of duty 1/2/4/5 multiplayer game server and private TinyRcon server application related settings:
- your private admin account's user name (username - change your private admin account's user name from ^1Admin to your correct user name. An authorized private admin can log in to a private TinyRcon only if they use their unique private admin account's username and password credentials. In the example above a private admin's user name is pol78 and their password is "6SiLL}DjTUmTgYA9<zDjNAM2yTFzXF>f" without "") 
- game server's name (game_server_name - game server's sv_hostname value, it's an optional setting, you can leave it at its default value "CoDHost")
- game server's IP address (game_server_ip_address)
- game server's port (game_server_port)
- your private admin account's password (rcon_password - enter or copy/paste your private admin account's password here. In a private admin account's example above rcon_password should be set to "6SiLL}DjTUmTgYA9<zDjNAM2yTFzXF>f". Important: rcon_password in config\tinyrcon.json for private TinyRcon client has to be set to your private admin account's unique password.)
- game server's private password (private_slot_password - optional setting, leave it empty for private TinyRcon client)
- Private TinyRcon server's IP address (private_tiny_rcon_server_ip_address - it is used for communicating with private TinyRcon server)
- Private TinyRcon server's port number (private_tiny_rcon_server_port - it is used for communicating with private TinyRcon server. Don't change it unless you need to. 
  Leave it at its default value: 27017)
  
  ---
 c. For **TinyRcon server** you need to configure the following important Call of duty 1/2/4/5 multiplayer game server and TinyRcon server application related settings:
- user name (username - by default it is set to "^1Admin". You can leave it at its default value or change it to your preferred user name.)
- game server's name (game_server_name - you can leave it at its default setting ("CoDHost) or set it to Call of duty game server's title (sv_hostname))
- game server's IP address (game_server_ip_address - Call of duty game server's correct IP address)
- game server's port (game_server_port - Call of duty game server's port)
- game server's rcon password (rcon_password - Call of duty game server's correct rcon password)
- game server's private password (private_slot_password - leave it empty if your CoD multiplayer game server does not use private slots)
- TinyRcon server's IP address (tiny_rcon_server_ip_address - used for communicating with regular TinyRcon clients used by authorized admins that have the correct rcon password)
- TinyRcon server's port number (tiny_rcon_server_port - used for communicating with regular TinyRcon clients used by authorized admins that have the correct rcon password.  
  Don't change it unless you need to. Leave it at its default value: 27015)
- Private TinyRcon server's IP address (private_tiny_rcon_server_ip_address - it is used for communicating with private TinyRcon server)
- Private TinyRcon server's port number (private_tiny_rcon_server_port - it is used for communicating with private TinyRcon server.  Don't change it unless you need to. Leave it at its default value: 27017)
---
d. For **private TinyRcon server** you need to configure the following important Call of duty 1/2/4/5 multiplayer game server and private TinyRcon server application related settings:
- user name (username - by default it is set to "^1Admin". You can leave it at its default value or change it to your preferred user name.)
- game server's name (game_server_name - you can leave it at its default setting ("CoDHost) or set it to Call of duty game server's title (sv_hostname))
- game server's IP address (game_server_server_ip_address - Call of duty game server's correct IP address)
- game server's port (game_server_port - Call of duty game server's port)
- game server's rcon password (rcon_password - Call of duty game server's correct rcon password)
- game server's private password (private_slot_password - leave it empty if your CoD multiplayer game server does not use private slots)
- Private TinyRcon server's IP address (private_tiny_rcon_server_ip_address - used for communicating with private TinyRcon clients which are used by private admins that have to log in to their private admin accounts using their unique username/password login credentials. A private admin using their private TinyRcon client can execute via their private TinyRcon client any admin level user and rcon commands by having the commands sent to and executed in the private TinyRcon server which has access to the game server's correct rcon password. Private admins cannot retrieve the game server's rcon and private passwords and their private TinyRcon clients don't use the game server's correct rcon password at all.)
- Private TinyRcon server's port number (private_tiny_rcon_server_port - used for communicating with private TinyRcon clients which are used by private admins that have to log in to their private admin accounts using their unique username/password login credentials. Don't change it unless you need to. Leave it at its default value: 27015)
  
Both TinyRcon server and private TinyRcon server should be running on the same computer as both need the correct rcon password to work correctly. TinyRcon server does not send any rcon commands to the configured game server's IP:port address but private TinyRcon server does. Private TinyRcon server looks very similar to TinyRcon client and it works in an almost indentical way to regular TinyRcon client except for the fact that it can also process requests, commands received from successfully logged in authorized private TinyRcon clients.

---

### TinyRcon application configuration example:

1. In the first step you should configure your TinyRcon server by editing its tinyrcon.json located in the TinyRconServer\config folder.
   You need to specify at minimum the following settings: username, game_server_name, game_server_ip_address, game_server_port, rcon_password, 
   private_slot_password (optional), tiny_rcon_server_ip_address, tiny_rcon_server_port

2. In the second step you should configure your private TinyRcon server by editing its tinyrcon.json located in the TinyRconPrivateServer\config folder.
   You need to specify at minimum the following settings: username, game_server_name, game_server_ip_address, game_server_port, rcon_password, 
   private_slot_password (optional), tiny_rcon_server_ip_address, tiny_rcon_server_port, private_tinyrcon_server_ip_address, private_tinyrcon_server_port

3. In the third step you should configure your TinyRcon client by editing its tinyrcon.json located in the TinyRconClient\config folder.
   You need to specify at minimum the following settings: username, game_server_name, game_server_ip_address, game_server_port, rcon_password, 
   private_slot_password (optional), tiny_rcon_server_ip_address, tiny_rcon_server_port, private_tinyrcon_server_ip_address, private_tinyrcon_server_port

4. In the fourth step you should configure your private TinyRcon client by editing its tinyrcon.json located in the TinyRconPrivateClient\config folder.
   You need to specify at minimum the following settings: username, game_server_name, game_server_ip_address, game_server_port, rcon_password, 
   private_slot_password (optional), private_tinyrcon_server_ip_address, private_tinyrcon_server_port

Try to run both public and private TinyRcon server on the same host machine with the same IP address but with different ports (27015 and 27017 are the default ports for TinyRcon server and private TinyRcon server, respectively) as private TinyRcon server needs to communicate with TinyRcon server in order to handle certain commands, requests.

You can also run TinyRcon client as well as private TinyRcon client on the same host machine with the same IP address. I have tested this scenario multiple times. Both TinyRcon server and private TinyRcon server as well as TinyRcon client and private TinyRcon client can run on the same computer. That said however there is no need to do that as running TinyRcon server and private TinyRcon server on the same host machine is more than enough for an admin to be able to manage the configured game server. One of the reasons is simple. Private TinyRcon server sort of functions as both a TinyRcon server and client integrated into one application. It has the same graphical user interface as the regular TinyRcon client.
TinyRcon client should be used by authorized regular admins and private TinyRcon client (which works without the game server's correct rcon_password) should be used by private admins in a remote manner.

The !mute pid and !unmute pid commands at the moment only work on Call of duty 2 multiplayer game servers (v1.0, v1.01, v1.2 and v1.3) if you use my patched cod2mp_s.exe game server executable file for running your game server. It contains several patches and custom new functions that make muting and unmuting players possible based on their IP addresses without having to rely on existing custom mod features (custom .gsc mod file(s)).

Private TinyRcon server which can be used as a regular TinyRcon client for managing game servers has one additional unique feature. It can detect and automatically kick players that try to connect to the game server using VPN/proxy IP addresses. By default this feature is disabled. You can enable it by setting "*enable_automatic_vpn_proxy_ip_address_detection*" to true in private TinyRcon server's tinyrcon.json file. 
You can also change in private TinyRcon server's tinyrcon.json file the currently registered proxycheck.io key by signing up for a free or a premium account at *https://proxycheck.io/* and changing "*proxy_check_io_user_key*" to your custom key.

---

### Important notes:
- Public TinyRcon and private TinyRcon servers need the following 2 ports to be opened for UDP communication (Port Forwarding, Virtual Servers settings in Advanced Setup -> NAT): 27015 (UDP), 27017 (UDP).

- First extract TinyRconServer.zip and TinyRconPrivateServer.zip to a custom location on your computer or a server machine, preferrably in a folder path with no spaces in it. After that go in the config folder of both applications and configure the most important settings of both TinyRcon server and private TinyRcon server by editing their tinyrcon.json configuration files as explained above.