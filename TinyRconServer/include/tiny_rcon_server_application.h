#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>

#include "connection_manager.h"
#include "connection_manager_for_messages.h"
#include "game_server.h"

#include "tiny_rcon_utility_functions.h"

#include "tiny_rcon_handles.h"

#include "stats.h"

class tinyrcon_activities_stats
{
	size_t no_of_reports{};
	size_t no_of_warnings{};
	size_t no_of_autokicks{};
	size_t no_of_kicks{};
	size_t no_of_tempbans{};
	size_t no_of_guid_bans{};
	size_t no_of_ip_bans{};
	size_t no_of_ip_address_range_bans{};
	size_t no_of_city_bans{};
	size_t no_of_country_bans{};
	size_t no_of_name_bans{};
	size_t no_of_protected_ip_addresses{};
	size_t no_of_protected_ip_address_ranges{};
	size_t no_of_protected_cities{};
	size_t no_of_protected_countries{};
	size_t no_of_map_restarts{};
	size_t no_of_map_changes{};
	time_t start_time{};

public:
	time_t get_start_time() const noexcept
	{
		return start_time;
	}

	void set_start_time(const time_t new_start_time) noexcept
	{
		start_time = new_start_time;
	}

	size_t& get_no_of_reports() noexcept
	{
		return no_of_reports;
	}

	size_t& get_no_of_warnings() noexcept
	{
		return no_of_warnings;
	}

	size_t& get_no_of_kicks() noexcept
	{
		return no_of_kicks;
	}

	size_t& get_no_of_autokicks() noexcept
	{
		return no_of_autokicks;
	}

	size_t& get_no_of_tempbans() noexcept
	{
		return no_of_tempbans;
	}

	size_t& get_no_of_guid_bans() noexcept
	{
		return no_of_guid_bans;
	}

	size_t& get_no_of_ip_bans() noexcept
	{
		return no_of_ip_bans;
	}

	size_t& get_no_of_ip_address_range_bans() noexcept
	{
		return no_of_ip_address_range_bans;
	}

	size_t& get_no_of_city_bans() noexcept
	{
		return no_of_city_bans;
	}

	size_t& get_no_of_country_bans() noexcept
	{
		return no_of_country_bans;
	}

	size_t& get_no_of_name_bans() noexcept
	{
		return no_of_name_bans;
	}

	size_t& get_no_of_protected_ip_addresses() noexcept
	{
		return no_of_protected_ip_addresses;
	}

	size_t& get_no_of_protected_ip_address_ranges() noexcept
	{
		return no_of_protected_ip_address_ranges;
	}

	size_t& get_no_of_protected_cities() noexcept
	{
		return no_of_protected_cities;
	}

	size_t& get_no_of_protected_countries() noexcept
	{
		return no_of_protected_countries;
	}

	size_t& get_no_of_map_restarts() noexcept
	{
		return no_of_map_restarts;
	}

	size_t& get_no_of_map_changes() noexcept
	{
		return no_of_map_changes;
	}
};


#undef max

using std::string;

class tiny_rcon_server_application
{
	std::atomic<bool> is_connection_settings_valid{ true };
	bool is_log_file_open{};
	std::atomic<uint64_t> previous_downloaded_data_in_bytes{ 0ULL };
	std::atomic<uint64_t> previous_uploaded_data_in_bytes{ 0ULL };
	std::atomic<uint64_t> next_downloaded_data_in_bytes{ 0ULL };
	std::atomic<uint64_t> next_uploaded_data_in_bytes{ 0ULL };
	tiny_rcon_handles* app_handles;
	string username{ "^1Admin" };
	string game_name{ "cod2" }; // "cod1,cod2,cod4,cod5"
	string game_server_name{
	  "185.158.113.146:28995 CoD2 CTF"
	};
	string command_line_info;
	
	std::map<string, string> server_settings;

	std::map<std::string, std::pair<std::string, std::string>> available_rcon_to_full_map_names{
	  { "mp_breakout", { "Villers-Bocage, France", "Villers-Bocage, France" } },
	  { "mp_brecourt", { "Brecourt, France", "Brecourt, France" } },
	  { "mp_burgundy", { "Burgundy, France", "Burgundy, France" } },
	  { "mp_carentan", { "Carentan, France", "Carentan, France" } },
	  { "mp_dawnville", { "St. Mere Eglise, France", "St. Mere Eglise, France" } },
	  { "mp_decoy", { "El Alamein, Egypt", "El Alamein, Egypt" } },
	  { "mp_downtown", { "Moscow, Russia", "Moscow, Russia" } },
	  { "mp_farmhouse", { "Beltot, France", "Beltot, France" } },
	  { "mp_leningrad", { "Leningrad, Russia", "Leningrad, Russia" } },
	  { "mp_matmata", { "Matmata, Tunisia", "Matmata, Tunisia" } },
	  { "mp_railyard", { "Stalingrad, Russia", "Stalingrad, Russia" } },
	  { "mp_toujane", { "Toujane, Tunisia", "Toujane, Tunisia" } },
	  { "mp_trainstation", { "Caen, France", "Caen, France" } }
	};

	std::map<std::string, std::string> available_full_map_to_rcon_map_names{
	  { "Villers-Bocage, France", "mp_breakout" },
	  { "Brecourt, France", "mp_brecourt" },
	  { "Burgundy, France", "mp_burgundy" },
	  { "Carentan, France", "mp_carentan" },
	  { "St. Mere Eglise, France", "mp_dawnville" },
	  { "El Alamein, Egypt", "mp_decoy" },
	  { "Moscow, Russia", "mp_downtown" },
	  { "Beltot, France", "mp_farmhouse" },
	  { "Leningrad, Russia", "mp_leningrad" },
	  { "Matmata, Tunisia", "mp_matmata" },
	  { "Stalingrad, Russia", "mp_railyard" },
	  { "Toujane, Tunisia", "mp_toujane" },
	  { "Caen, France", "mp_trainstation" },
	};

	std::unordered_map<std::string, std::string> admin_messages{
	  { "user_defined_warn_msg", "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
	  { "user_defined_kick_msg", "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
	  { "user_defined_temp_ban_msg", "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for ^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: ^1{REASON}" },
	  { "user_defined_ban_msg", "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
	  { "user_defined_ip_ban_msg", "^7{PLAYERNAME} ^1you are being permanently banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
	  { "user_defined_city_ban_msg", "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}" },
	  { "user_defined_city_unban_msg", "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: ^5{CITY_NAME}" },
	  { "user_defined_enable_city_ban_feature_msg", "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities." },
	  {
		"user_defined_disable_city_ban_feature_msg",
		"^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities.",
	  },
	  {
		"user_defined_country_ban_msg",
		"^7Admin ^5{ADMINNAME} ^1has globally banned country: ^5{COUNTRY_NAME}",
	  },
	  { "user_defined_country_unban_msg", "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: ^5{COUNTRY_NAME}" },
	  { "user_defined_enable_country_ban_feature_msg", "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries." },
	  { "user_defined_disable_country_ban_feature_msg", "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries." },
	  { "automatic_remove_temp_ban_msg", "^1{ADMINNAME}: ^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been removed. ^5Reason of ban: ^1{REASON}" },
	  { "automatic_kick_temp_ban_msg", "^1{ADMINNAME}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}" },
	  { "automatic_kick_ip_ban_msg",
		"^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}" },
	  {
		"automatic_kick_city_ban_msg",
		"^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: ^5{CITY_NAME} ^7is being automatically ^1kicked.",
	  },
	  { "automatic_kick_country_ban_msg",
		"^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  ^5{COUNTRY_NAME} ^7is being automatically ^1kicked." }
	};

	game_server server;
	connection_manager cm;
	connection_manager_for_messages cm_for_messages;
	stats stats_data;

	std::queue<command_t> command_queue{};
	std::queue<message_t> message_queue{};
	std::vector<geoip_data> geoip_db;

	std::vector<std::shared_ptr<tiny_rcon_client_user>> users;
	std::map<std::string, std::shared_ptr<tiny_rcon_client_user>> name_to_user;

	std::unordered_map<std::string, std::string> tinyrcon_dict{
	  { "{ADMINNAME}", username },
	  { "{PLAYERNAME}", "" },
	  { "{REASON}", "not specified" }
	};

	std::unordered_map<std::string, time_t> sent_rcon_public_messages;
	std::unordered_map<std::string, bool> is_user_data_received;
	std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> command_handlers;
	std::unordered_map<std::string, std::function<void(const std::string&, time_t, const std::string&, const bool, const string&)>> message_handlers;


	std::string program_title{ "Welcome to TinyRcon" };
	std::string current_working_directory;
	std::string tinyrcon_config_file_path{ "config\\tinyrcon.json" };
	std::string temp_bans_file_path{ "data\\tempbans.txt" };
	std::string removed_temp_bans_file_path{ "data\\removed_tempbans.txt" };
	std::string ip_bans_file_path{ "data\\bans.txt" };
	std::string removed_ip_bans_file_path{ "data\\removed_bans.txt" };
	std::string ip_range_bans_file_path{ "data\\ip_range_bans.txt" };
	std::string removed_ip_range_bans_file_path{ "data\\removed_ip_range_bans.txt" };
	std::string banned_cities_file_path{ "data\\banned_cities.txt" };
	std::string removed_banned_cities_file_path{ "data\\removed_banned_cities.txt" };
	std::string banned_countries_file_path{ "data\\banned_countries.txt" };
	std::string removed_banned_countries_file_path{ "data\\removed_banned_countries.txt" };
	std::string banned_names_file_path{ "data\\banned_names.txt" };
	std::string removed_banned_names_file_path{ "data\\removed_banned_names.txt" };
	std::string protected_ip_addresses_file_path{ "data\\protected_ip_addresses.txt" };
	std::string protected_ip_address_ranges_file_path{ "data\\protected_ip_address_ranges.txt" };
	std::string protected_cities_file_path{ "data\\protected_cities.txt" };
	std::string protected_countries_file_path{ "data\\protected_countries.txt" };
	std::string users_data_file_path{ "data\\users.txt" };
	std::string ftp_download_site_ip_address{ "85.222.189.119" };
	std::string ftp_download_folder_path{ "tinyrcon" };
	std::string ftp_bans_folder_path{ "C:\\tinyrcon\\bans" };
	std::string ftp_download_file_pattern{ R"(^_U_TinyRcon[\._-]?v?(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})\.exe$)" };
	std::string plugins_geoIP_geo_dat_md5;
	std::ofstream log_file;
	std::mutex user_data_mutex{};
	std::recursive_mutex command_queue_mutex{};
	std::mutex message_queue_mutex{};
	std::string tiny_rcon_ftp_server_username;
	std::string tiny_rcon_ftp_server_password;
	std::string tiny_rcon_server_ip_address;
	int tiny_rcon_server_port{};
	tinyrcon_activities_stats tinyrcon_stats_data;


public:
	tiny_rcon_server_application() = default;
	~tiny_rcon_server_application() = default;

	void set_tiny_rcon_app_handles(tiny_rcon_handles* new_app_handles) {
		app_handles = new_app_handles;
	}

	void set_is_connection_settings_valid(const bool new_value) noexcept
	{
		is_connection_settings_valid.store(new_value);
	}

	bool get_is_connection_settings_valid() const noexcept
	{
		return is_connection_settings_valid.load();
	}

	inline const string& get_username() const noexcept
	{
		return username;
	}

	inline void set_username(string new_value)
	{
		remove_disallowed_character_in_string(new_value);
		username = std::move(new_value);
	}

	inline const string& get_game_name() const noexcept
	{
		return game_name;
	}

	inline void set_game_name(string new_value) noexcept
	{
		game_name = std::move(new_value);
	}

	inline const string& get_game_server_name() const noexcept
	{
		return game_server_name;
	}

	inline void set_game_server_name(string new_value) noexcept
	{
		game_server_name = std::move(new_value);
	}

	inline std::vector<geoip_data>& get_geoip_data() noexcept { return geoip_db; }

	inline connection_manager& get_connection_manager() noexcept {
		return cm;
	}

	inline connection_manager_for_messages& get_connection_manager_for_messages() noexcept
	{
		return cm_for_messages;
	}

	inline game_server& get_game_server() noexcept { return server; }

	inline std::unordered_map<std::string, std::string>&
		get_admin_messages() noexcept
	{
		return admin_messages;
	}

	inline const std::string& get_program_title() const noexcept
	{
		return program_title;
	}

	inline void set_program_title(std::string new_program_title) noexcept
	{
		program_title = std::move(new_program_title);
	}

	std::map<std::string, std::shared_ptr<tiny_rcon_client_user>>& get_name_to_user() noexcept
	{
		return name_to_user;
	}

	std::vector<std::shared_ptr<tiny_rcon_client_user>>& get_users() noexcept
	{
		return users;
	}

	std::shared_ptr<tiny_rcon_client_user>& get_user_for_name(const std::string& name, const string& ip_address)
	{
		string cleaned_name{ get_cleaned_user_name(name) };
		// player pd{};
		// convert_guid_key_to_country_name(cm_for_messages.get_geoip_data(), ip_address, pd);

		if (!name_to_user.contains(cleaned_name)) {
			users.emplace_back(std::make_shared<tiny_rcon_client_user>());
			users.back()->user_name = name;
			name_to_user.emplace(cleaned_name, users.back());
		}

		return name_to_user.at(cleaned_name);
	}

	bool is_user_admin(const std::string& name, const std::string& password) const noexcept
	{
		string cleaned_name{ get_cleaned_user_name(name) };
		// unsigned long guid_key{};
		// if (check_ip_address_validity(ip_address, guid_key)) {
		// player pd{};
		// convert_guid_key_to_country_name(cm_for_messages.get_geoip_data(), ip_address, pd);
		// if (cleaned_name == "admin") {
		//   cleaned_name += std::format("_{}", pd.country_name);
		// }
		// }

		return name_to_user.contains(cleaned_name) && server.get_rcon_password() == password;
	}

	std::unordered_map<std::string, std::string>& get_tinyrcon_dict() noexcept
	{
		return tinyrcon_dict;
	}

	std::string get_user_defined_warn_message()
	{
		return admin_messages["user_defined_warn_msg"];
	}

	std::string get_user_defined_kick_message()
	{
		return admin_messages["user_defined_kick_msg"];
	}

	std::string get_user_defined_tempban_message()
	{
		return admin_messages["user_defined_temp_ban_msg"];
	}

	std::string get_user_defined_ban_message()
	{
		return admin_messages["user_defined_ban_msg"];
	}

	std::string get_user_defined_ipban_message()
	{
		return admin_messages["user_defined_ip_ban_msg"];
	}

	std::string get_user_defined_city_ban_msg()
	{
		return admin_messages["user_defined_city_ban_msg"];
	}

	std::string get_user_defined_city_unban_msg()
	{
		return admin_messages["user_defined_city_unban_msg"];
	}

	std::string get_user_defined_enable_city_ban_feature_msg()
	{
		return admin_messages["user_defined_enable_city_ban_feature_msg"];
	}

	std::string get_user_defined_disable_city_ban_feature_msg()
	{
		return admin_messages["user_defined_disable_city_ban_feature_msg"];
	}

	std::string get_user_defined_country_ban_msg()
	{
		return admin_messages["user_defined_country_ban_msg"];
	}

	std::string get_user_defined_country_unban_msg()
	{
		return admin_messages["user_defined_country_unban_msg"];
	}

	std::string get_user_defined_enable_country_ban_feature_msg()
	{
		return admin_messages["user_defined_enable_country_ban_feature_msg"];
	}

	std::string get_user_defined_disable_country_ban_feature_msg()
	{
		return admin_messages["user_defined_disable_country_ban_feature_msg"];
	}

	std::string get_automatic_remove_temp_ban_msg()
	{
		return admin_messages["automatic_remove_temp_ban_msg"];
	}

	std::string get_automatic_kick_temp_ban_msg()
	{
		return admin_messages["automatic_kick_temp_ban_msg"];
	}

	std::string get_automatic_kick_ip_ban_msg()
	{
		return admin_messages["automatic_kick_ip_ban_msg"];
	}

	std::string get_automatic_kick_city_ban_msg()
	{
		return admin_messages["automatic_kick_city_ban_msg"];
	}

	std::string get_automatic_kick_country_ban_msg()
	{
		return admin_messages["automatic_kick_country_ban_msg"];
	}

	const std::string& get_current_working_directory() const noexcept
	{
		return current_working_directory;
	}

	void set_current_working_directory()
	{
		constexpr size_t max_path_limit{ 32768 };
		std::unique_ptr<char[]> exe_file_path{ std::make_unique<char[]>(max_path_limit) };
		if (const auto path_len = GetModuleFileNameA(nullptr, exe_file_path.get(), max_path_limit); path_len != 0) {
			std::string_view exe_file_path_sv(exe_file_path.get(), path_len);
			current_working_directory.assign(string(cbegin(exe_file_path_sv), cbegin(exe_file_path_sv) + exe_file_path_sv.rfind('\\') + 1));
		}
		else {
			std::filesystem::path entry("TinyRconServer.exe");
			current_working_directory.assign(entry.parent_path().string());
			if (!current_working_directory.empty() && current_working_directory.back() != '\\') {
				if ('/' == current_working_directory.back())
					current_working_directory.back() = '\\';
				else
					current_working_directory.push_back('\\');
			}
		}

		tinyrcon_config_file_path.assign(format("{}{}", current_working_directory, tinyrcon_config_file_path));
		users_data_file_path.assign(format("{}{}", current_working_directory, users_data_file_path));
		temp_bans_file_path.assign(format("{}{}", current_working_directory, temp_bans_file_path));
		ip_bans_file_path.assign(format("{}{}", current_working_directory, ip_bans_file_path));
		ip_range_bans_file_path.assign(format("{}{}", current_working_directory, ip_range_bans_file_path));
		banned_countries_file_path.assign(format("{}{}", current_working_directory, banned_countries_file_path));
		banned_cities_file_path.assign(format("{}{}", current_working_directory, banned_cities_file_path));
		banned_names_file_path.assign(format("{}{}", current_working_directory, banned_names_file_path));
		protected_ip_addresses_file_path.assign(format("{}{}", current_working_directory, protected_ip_addresses_file_path));
		protected_ip_address_ranges_file_path.assign(format("{}{}", current_working_directory, protected_ip_address_ranges_file_path));
		protected_cities_file_path.assign(format("{}{}", current_working_directory, protected_cities_file_path));
		protected_countries_file_path.assign(format("{}{}", current_working_directory, protected_countries_file_path));
		removed_temp_bans_file_path.assign(format("{}{}", current_working_directory, removed_temp_bans_file_path));
		removed_ip_bans_file_path.assign(format("{}{}", current_working_directory, removed_ip_bans_file_path));
		removed_ip_range_bans_file_path.assign(format("{}{}", current_working_directory, removed_ip_range_bans_file_path));
		removed_banned_countries_file_path.assign(format("{}{}", current_working_directory, removed_banned_countries_file_path));
		removed_banned_cities_file_path.assign(format("{}{}", current_working_directory, removed_banned_cities_file_path));
		removed_banned_names_file_path.assign(format("{}{}", current_working_directory, removed_banned_names_file_path));
	}

	const char* get_tinyrcon_config_file_path() const noexcept
	{
		return tinyrcon_config_file_path.c_str();
	}

	const char* get_users_data_file_path() const noexcept
	{
		return users_data_file_path.c_str();
	}

	const char* get_temp_bans_file_path() const noexcept
	{
		return temp_bans_file_path.c_str();
	}

	const char* get_ip_bans_file_path() const noexcept
	{
		return ip_bans_file_path.c_str();
	}

	const char* get_ip_range_bans_file_path() const noexcept
	{
		return ip_range_bans_file_path.c_str();
	}

	const char* get_banned_countries_file_path() const noexcept
	{
		return banned_countries_file_path.c_str();
	}

	const char* get_banned_cities_file_path() const noexcept
	{
		return banned_cities_file_path.c_str();
	}

	const char* get_banned_names_file_path() const noexcept
	{
		return banned_names_file_path.c_str();
	}

	const char* get_protected_ip_addresses_file_path() const noexcept
	{
		return protected_ip_addresses_file_path.c_str();
	}

	const char* get_protected_ip_address_ranges_file_path() const noexcept
	{
		return protected_ip_address_ranges_file_path.c_str();
	}

	const char* get_protected_cities_file_path() const noexcept
	{
		return protected_cities_file_path.c_str();
	}

	const char* get_protected_countries_file_path() const noexcept
	{
		return protected_countries_file_path.c_str();
	}

	const char* get_removed_temp_bans_file_path() const noexcept
	{
		return removed_temp_bans_file_path.c_str();
	}

	const char* get_removed_ip_bans_file_path() const noexcept
	{
		return removed_ip_bans_file_path.c_str();
	}

	const char* get_removed_ip_range_bans_file_path() const noexcept
	{
		return removed_ip_range_bans_file_path.c_str();
	}

	const char* get_removed_banned_cities_file_path() const noexcept
	{
		return removed_banned_cities_file_path.c_str();
	}

	const char* get_removed_banned_countries_file_path() const noexcept
	{
		return removed_banned_countries_file_path.c_str();
	}

	const char* get_removed_banned_names_file_path() const noexcept
	{
		return removed_banned_names_file_path.c_str();
	}

	const std::string& get_tiny_rcon_ftp_server_username() const noexcept
	{
		return tiny_rcon_ftp_server_username;
	}

	void set_tiny_rcon_ftp_server_username(string new_tiny_rcon_ftp_server_username) noexcept
	{
		tiny_rcon_ftp_server_username = std::move(new_tiny_rcon_ftp_server_username);
	}

	const std::string& get_tiny_rcon_ftp_server_password() const noexcept
	{
		return tiny_rcon_ftp_server_password;
	}

	void set_tiny_rcon_ftp_server_password(string new_tiny_rcon_ftp_server_password) noexcept
	{
		tiny_rcon_ftp_server_password = std::move(new_tiny_rcon_ftp_server_password);
	}

	const std::string& get_tiny_rcon_server_ip_address() const noexcept
	{
		return tiny_rcon_server_ip_address;
	}

	void set_tiny_rcon_server_ip_address(string new_tiny_rcon_server_ip_address) noexcept
	{
		tiny_rcon_server_ip_address = std::move(new_tiny_rcon_server_ip_address);
	}

	int get_tiny_rcon_server_port() const noexcept
	{
		return tiny_rcon_server_port;
	}

	void set_tiny_rcon_server_port(const int new_tiny_rcon_server_port) noexcept
	{
		tiny_rcon_server_port = new_tiny_rcon_server_port;
	}

	const std::string& get_ftp_download_site_ip_address() const noexcept
	{
		return ftp_download_site_ip_address;
	}

	void set_ftp_download_site_ip_address(string new_ftp_download_site) noexcept
	{
		ftp_download_site_ip_address = std::move(new_ftp_download_site);
	}

	const std::string& get_ftp_download_folder_path() const noexcept
	{
		return ftp_download_folder_path;
	}

	void set_ftp_download_folder_path(string new_ftp_download_folder_path) noexcept
	{
		ftp_download_folder_path = std::move(new_ftp_download_folder_path);
	}

	const std::string& get_ftp_bans_folder_path() const noexcept
	{
		return ftp_bans_folder_path;
	}

	void set_ftp_bans_folder_path(string new_ftp_bans_folder_path) noexcept
	{
		ftp_bans_folder_path = std::move(new_ftp_bans_folder_path);
	}

	const std::string& get_ftp_download_file_pattern() const noexcept
	{
		return ftp_download_file_pattern;
	}

	void set_ftp_download_file_pattern(string new_ftp_download_file_pattern) noexcept
	{
		ftp_download_file_pattern = std::move(new_ftp_download_file_pattern);
	}

	const std::string& get_plugins_geoIP_geo_dat_md5() const noexcept
	{
		return plugins_geoIP_geo_dat_md5;
	}

	void set_plugins_geoIP_geo_dat_md5(string new_plugins_geoIP_geo_dat_md5) noexcept
	{
		plugins_geoIP_geo_dat_md5 = std::move(new_plugins_geoIP_geo_dat_md5);
	}

	auto& get_available_rcon_to_full_map_names() noexcept
	{
		return available_rcon_to_full_map_names;
	}

	auto& get_available_full_map_to_rcon_map_names() noexcept
	{
		return available_full_map_to_rcon_map_names;
	}

	bool open_log_file(const char* file_path)
	{
		if (log_file.is_open()) {
			log_file.flush();
			log_file.close();
		}

		log_file.open(file_path, std::ios::app);
		is_log_file_open = !log_file.fail();
		return is_log_file_open;
	}

	void log_message(const std::string& message)
	{
		if (is_log_file_open) {
			log_file << message << std::flush;
		}
	}

	bool is_command_queue_empty()
	{
		std::lock_guard lg{ command_queue_mutex };
		return command_queue.empty();
	}

	inline void add_command_to_queue(std::vector<std::string> cmd, const command_type cmd_type, const bool wait_for_reply)
	{
		std::lock_guard lg{ command_queue_mutex };
		command_queue.emplace(std::move(cmd), cmd_type, wait_for_reply);
	}

	inline command_t get_command_from_queue()
	{
		std::lock_guard lg{ command_queue_mutex };
		command_t next_command = command_queue.front();
		command_queue.pop();
		return next_command;
	}

	inline void process_queue_command(command_t cmd)
	{
		std::lock_guard lg{ command_queue_mutex };
		if (cmd.type == command_type::user) {
			process_user_command(cmd.command);
		}
	}

	bool is_message_queue_empty()
	{
		std::lock_guard lg{ message_queue_mutex };
		return message_queue.empty();
	}

	inline void add_message_to_queue(message_t message)
	{
		std::lock_guard lg{ message_queue_mutex };
		message_queue.emplace(std::move(message));
	}

	inline message_t get_message_from_queue()
	{
		std::lock_guard lg{ message_queue_mutex };
		auto next_message = std::move(message_queue.front());
		message_queue.pop();
		return next_message;
	}

	const std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>>& get_command_handlers() const noexcept
	{
		return command_handlers;
	}

	void add_command_handler(std::vector<std::string> command_names, std::function<void(const std::vector<std::string>&)> command_handler)
	{
		if (command_names.size() > 1) {
			for (size_t i{}; i < command_names.size() - 1; ++i) {
				command_handlers.emplace(std::move(command_names[i]), command_handler);
			}
		}
		command_handlers.emplace(std::move(command_names.back()), std::move(command_handler));
	}

	std::pair<bool, const std::function<void(const std::vector<std::string>&)>&> get_command_handler(const std::string& command_name) const
	{
		static std::function<void(const std::vector<std::string>&)> unknown_command_handler{
		  [](const std::vector<std::string>&) {}
		};

		if (command_handlers.contains(command_name))
			return make_pair(true, command_handlers.at(command_name));
		return make_pair(false, unknown_command_handler);
	}

	const std::unordered_map<std::string, std::function<void(const std::string&, time_t, const std::string&, const bool, const string&)>>& get_message_handlers() const noexcept
	{
		return message_handlers;
	}

	void add_message_handler(std::string message_name, std::function<void(const std::string&, time_t, const std::string&, const bool, const string&)> message_handler)
	{
		message_handlers.emplace(std::move(message_name), std::move(message_handler));
	}

	const std::function<void(const std::string&, time_t, const std::string&, const bool, const string&)>& get_message_handler(const std::string& message_name) const
	{
		static std::function<void(const std::string&, time_t, const std::string&, const bool, const string&)> unknown_message_handler{
		  [](const std::string&, time_t, const std::string&, const bool, const string&) {}
		};

		if (message_handlers.contains(message_name))
			return message_handlers.at(message_name);
		return unknown_message_handler;
	}

	std::unordered_map<std::string, time_t>& get_sent_rcon_public_messages() noexcept
	{
		return sent_rcon_public_messages;
	}

	bool get_is_user_data_received_for_user(const std::string& name, const string& ip_address)
	{
		string cleaned_name{ get_cleaned_user_name(name) };
		player pd{};
		convert_guid_key_to_country_name(get_geoip_data(), ip_address, pd);

		if (name.find("Admin") != string::npos) {
			cleaned_name += std::format("_{}", pd.country_name);
		}

		if (!is_user_data_received.contains(cleaned_name)) {
			is_user_data_received.emplace(cleaned_name, false);
		}

		return is_user_data_received[cleaned_name];
	}

	void set_is_user_data_received_for_user(const std::string& name, const string& ip_address)
	{
		string cleaned_name{ get_cleaned_user_name(name) };
		player pd{};
		convert_guid_key_to_country_name(get_geoip_data(), ip_address, pd);

		if (name.find("Admin") != string::npos) {
			cleaned_name += std::format("_{}", pd.country_name);
		}

		if (!is_user_data_received.contains(cleaned_name)) {
			is_user_data_received.emplace(cleaned_name, true);
		}

		is_user_data_received[cleaned_name] = true;
	}

	tinyrcon_activities_stats& get_tinyrcon_stats_data() noexcept { return tinyrcon_stats_data; }

	std::string get_welcome_message(const std::string& user_name)
	{
		const size_t no_of_reports = tinyrcon_stats_data.get_no_of_reports();
		const size_t no_of_warnings = tinyrcon_stats_data.get_no_of_warnings();
		const size_t no_of_autokicks = tinyrcon_stats_data.get_no_of_autokicks();
		const size_t no_of_kicks = tinyrcon_stats_data.get_no_of_kicks();
		const size_t no_of_tempbans = tinyrcon_stats_data.get_no_of_tempbans();
		const size_t no_of_guid_bans = tinyrcon_stats_data.get_no_of_guid_bans();
		const size_t no_of_ip_bans = server.get_banned_ip_addresses_map().size();
		const size_t no_of_ip_address_range_bans = server.get_banned_ip_address_ranges_map().size();
		const size_t no_of_name_bans = server.get_banned_names_map().size();
		const size_t no_of_city_bans = server.get_banned_cities_set().size();
		const size_t no_of_country_bans = server.get_banned_countries_set().size();
		const size_t no_of_protected_ip_addresses = server.get_protected_ip_addresses().size();
		const size_t no_of_protected_ip_address_ranges = server.get_protected_ip_address_ranges().size();
		const size_t no_of_protected_cities = server.get_protected_cities().size();
		const size_t no_of_protected_countries = server.get_protected_countries().size();
		const size_t no_of_map_restarts = tinyrcon_stats_data.get_no_of_map_restarts();
		const size_t no_of_map_changes = tinyrcon_stats_data.get_no_of_map_changes();

		std::ostringstream oss;
		oss << std::format("^5\n\n^5Hi, welcome back, ^1admin ^7{}\n", user_name);
		oss << std::format("^5Number of registered ^1auto-kicks: ^3{}\n", no_of_autokicks);
		oss << std::format("^5Number of received ^1reports: ^3{}\n", no_of_reports);
		oss << std::format("^5Admins have so far...\n   ^1warned ^3{} ^5{},\n   ^1kicked ^3{} ^5{},\n   ^1temporarily banned ^3{} ^5{},\n", no_of_warnings, no_of_warnings != 1 ? "players" : "player", no_of_kicks, no_of_kicks != 1 ? "players" : "player", no_of_tempbans, no_of_tempbans != 1 ? "players" : "player");
		oss << std::format("   ^1permanently banned GUID {} ^5of ^3{} ^5{},\n   ^1banned IP {} ^5of ^3{} ^5{},\n   ^1banned IP address range(s) ^5of ^3{} ^5{},\n   ^1banned ^3{} ^1player {}^5,\n", no_of_guid_bans != 1 ? "keys" : "key", no_of_guid_bans, no_of_guid_bans != 1 ? "players" : "player", no_of_ip_bans != 1 ? "addresses" : "address", no_of_ip_bans, no_of_ip_bans != 1 ? "players" : "player", no_of_ip_address_range_bans, no_of_ip_address_range_bans != 1 ? "players" : "player", no_of_name_bans, no_of_name_bans != 1 ? "names" : "name");
		oss << std::format("   ^1banned {} ^5of ^3{} ^5{},\n   ^1banned {} ^5of ^3{} ^5{},\n   ^1protected IP {} ^5of ^3{} ^5{},\n", no_of_city_bans != 1 ? "cities" : "city", no_of_city_bans, no_of_city_bans != 1 ? "players" : "player", no_of_country_bans != 1 ? "countries" : "country", no_of_country_bans, no_of_country_bans != 1 ? "players" : "player", no_of_protected_ip_addresses != 1 ? "addresses" : "address", no_of_protected_ip_addresses, no_of_protected_ip_addresses != 1 ? "players" : "player");
		oss << std::format("   ^1protected IP address {} ^5of ^3{} ^5{},\n   ^1protected {} ^5of ^3{} ^5{},\n^1protected {} ^5of ^3{} ^5{},\n", no_of_protected_ip_address_ranges != 1 ? "ranges" : "range", no_of_protected_ip_address_ranges, no_of_protected_ip_address_ranges != 1 ? "players" : "player", no_of_protected_cities != 1 ? "cities" : "city", no_of_protected_cities, no_of_protected_cities != 1 ? "players" : "player", no_of_protected_countries != 1 ? "countries" : "country", no_of_protected_countries, no_of_protected_countries != 1 ? "players" : "player");
		oss << std::format("^5Admins executed ^3{} ^1map restarts ^5and ^3{} ^1map changes^5.\n\n^7", no_of_map_restarts, no_of_map_changes);
		return oss.str();
	}

	[[nodiscard]] map<string, string>& get_server_settings() noexcept
	{
		return server_settings;
	}

	//   uint64_t previous_downloaded_data_in_bytes{};
	inline uint64_t get_previous_downloaded_data_in_bytes() const noexcept
	{
		return previous_downloaded_data_in_bytes.load();
	}

	void set_previous_downloaded_data_in_bytes(const size_t new_value) noexcept
	{
		previous_downloaded_data_in_bytes.store(new_value);
	}

	// uint64_t previous_uploaded_data_in_bytes{};
	inline uint64_t get_previous_uploaded_data_in_bytes() const noexcept
	{
		return previous_uploaded_data_in_bytes.load();
	}

	void set_previous_uploaded_data_in_bytes(const size_t new_value) noexcept
	{
		previous_uploaded_data_in_bytes.store(new_value);
	}


	//   uint64_t next_downloaded_data_in_bytes{};
	inline uint64_t get_next_downloaded_data_in_bytes() const noexcept
	{
		return next_downloaded_data_in_bytes.load();
	}

	void add_to_next_downloaded_data_in_bytes(const size_t value) noexcept
	{
		next_downloaded_data_in_bytes.fetch_add(value);
	}

	// uint64_t next_uploaded_data_in_bytes{};
	inline uint64_t get_next_uploaded_data_in_bytes() const noexcept
	{
		return next_uploaded_data_in_bytes.load();
	}

	void add_to_next_uploaded_data_in_bytes(const size_t value) noexcept
	{
		next_uploaded_data_in_bytes.fetch_add(value);
	}

	void update_download_and_upload_speed_statistics()
	{
		static constexpr uint64_t one_gigabyte{ 1024U * 1024U * 1024U };
		static constexpr uint64_t one_megabyte{ 1024U * 1024U };
		static constexpr uint64_t one_kilobyte{ 1024U };
		static auto last_time_stamp = std::chrono::high_resolution_clock::now();

		const auto now_time_stamp = std::chrono::high_resolution_clock::now();
		const double elapsed_time_interval_in_seconds = static_cast<double>(std::chrono::duration<double, std::milli>{ now_time_stamp - last_time_stamp }.count()) / 1000.0;

		uint64_t uploaded_gigabytes{}, uploaded_megabytes{}, uploaded_kilobytes{};
		const uint64_t uploaded_bytes = next_uploaded_data_in_bytes.load();
		std::ostringstream oss_ua;

		if (uploaded_bytes >= one_gigabyte) {
			oss_ua << std::format("^1{:.2f} GB", static_cast<double>(uploaded_bytes) / one_gigabyte);
		}
		else if (uploaded_bytes >= one_megabyte) {
			oss_ua << std::format("^1{:.2f} MB", static_cast<double>(uploaded_bytes) / one_megabyte);
		}
		else if (uploaded_bytes >= one_kilobyte) {
			oss_ua << std::format("^1{:.2f} kB", static_cast<double>(uploaded_bytes) / one_kilobyte);
		}
		else {
			oss_ua << std::format("^1{} bytes", uploaded_bytes);
		}

		uploaded_gigabytes = uploaded_megabytes = uploaded_kilobytes = 0ULL;
		const uint64_t uploaded_bytes_diff = static_cast<uint64_t>((next_uploaded_data_in_bytes.load() - previous_uploaded_data_in_bytes.load()) / elapsed_time_interval_in_seconds);
		std::ostringstream oss_us;
		oss_us << "^3Upload speed: ";

		if (uploaded_bytes_diff >= one_gigabyte) {
			oss_us << std::format("^1{:.2f} GB", static_cast<double>(uploaded_bytes_diff) / one_gigabyte);
		}
		else if (uploaded_bytes_diff >= one_megabyte) {
			oss_us << std::format("^1{:.2f} MB", static_cast<double>(uploaded_bytes_diff) / one_megabyte);
		}
		else if (uploaded_bytes_diff >= one_kilobyte) {
			oss_us << std::format("^1{:.2f} kB", static_cast<double>(uploaded_bytes_diff) / one_kilobyte);
		}
		else {
			oss_us << std::format("^1{} bytes", uploaded_bytes_diff);
		}

		oss_us << " ^3Total: " << oss_ua.str();
		const string upload_speed_message{ oss_us.str() };

		uint64_t downloaded_gigabytes{}, downloaded_megabytes{}, downloaded_kilobytes{};
		const uint64_t downloaded_bytes = next_downloaded_data_in_bytes.load();
		std::ostringstream oss_da;

		if (downloaded_bytes >= one_gigabyte) {
			oss_da << std::format("^1{:.2f} GB", static_cast<double>(downloaded_bytes) / one_gigabyte);
		}
		else if (downloaded_bytes >= one_megabyte) {
			oss_da << std::format("^1{:.2f} MB", static_cast<double>(downloaded_bytes) / one_megabyte);
		}
		else if (downloaded_bytes >= one_kilobyte) {
			oss_da << std::format("^1{:.2f} kB", static_cast<double>(downloaded_bytes) / one_kilobyte);
		}
		else {
			oss_da << std::format("^1{} bytes", downloaded_bytes);
		}

		downloaded_gigabytes = downloaded_megabytes = downloaded_kilobytes = 0ULL;
		const uint64_t downloaded_bytes_diff = static_cast<uint64_t>((next_downloaded_data_in_bytes.load() - previous_downloaded_data_in_bytes.load()) / elapsed_time_interval_in_seconds);
		std::ostringstream oss_ds;

		oss_ds << "^2Download speed: ";

		if (downloaded_bytes_diff >= one_gigabyte) {
			oss_ds << std::format("^1{:.2f} GB", static_cast<double>(downloaded_bytes_diff) / one_gigabyte);
		}
		else if (downloaded_bytes_diff >= one_megabyte) {
			oss_ds << std::format("^1{:.2f} MB", static_cast<double>(downloaded_bytes_diff) / one_megabyte);
		}
		else if (downloaded_bytes_diff >= one_kilobyte) {
			oss_ds << std::format("^1{:.2f} kB", static_cast<double>(downloaded_bytes_diff) / one_kilobyte);
		}
		else {
			oss_ds << std::format("^1{} bytes", downloaded_bytes_diff);
		}

		oss_ds << " ^2Total: " << oss_da.str();
		const string download_speed_message{ oss_ds.str() };
		const string speed_message{ format("{} ^5| {}", oss_ds.str(), oss_us.str()) };
		SetWindowTextA(app_handles->hwnd_download_and_upload_speed_info, "");
		print_colored_text(app_handles->hwnd_download_and_upload_speed_info, speed_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no, true);
		/*SendMessage(app_handles.hwnd_upload_speed_info, EM_SETSEL, 0, -1);
		SendMessage(app_handles.hwnd_upload_speed_info, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);*/

		previous_uploaded_data_in_bytes.store(next_uploaded_data_in_bytes.load());
		previous_downloaded_data_in_bytes.store(next_downloaded_data_in_bytes.load());
		last_time_stamp = now_time_stamp;
	}

	inline stats& get_stats_data() noexcept
	{
		return stats_data;
	}

};
