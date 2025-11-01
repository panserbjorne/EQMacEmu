#include <map>
#include <random>
#include <utility> // For std::pair

#include "../client.h"
#include "../guild_mgr.h"
#include "../../common/discord/discord.h"
#include "../../common/events/player_event_logs.h"

// Guild ID, <Validation Code, Webhook URL>
std::map<int, std::pair<std::string,std::string>> webhook_validation_map;

std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void command_guildhook(Client *c, const Seperator *sep){
	if (c->GuildRank() != GUILD_LEADER) {
		c->Message(Chat::Red, "You need to be a Guild Leader to use this command.");
		return;
	}
	
	int guild_id = c->GuildID();
	
	// For "#guildhook validate <code>"
	if (sep->arg[1] != nullptr && std::strcmp(sep->arg[1], "validate") == 0) {
		std::string validation_code = sep->arg[2];
		if ( webhook_validation_map.find(guild_id) == webhook_validation_map.end() ) {
			c->Message(Chat::Red, "Please use '#guildhook <URL>' first.");
			return;
		}
		std::pair<std::string,std::string> validation_data = webhook_validation_map.at(guild_id);
		std::string stored_code = validation_data.first;
		std::string webhook_url = validation_data.second
		;
		if (validation_code != stored_code) {
			c->Message(Chat::Red, "Incorrect code, please try again.");
			return;
		}
		
		// Code was validated succesfully
		webhook_validation_map.erase(guild_id);
		
		// Check for existing webhook
		std::string existing_webhook_query = fmt::format(
			"SELECT discord_webhooks.id FROM discord_webhooks "
			"INNER JOIN guilds ON discord_webhooks.id = guilds.webhook_id "
			"WHERE guilds.id = {}", std::to_string(guild_id));
		auto existing_webhook_results = database.QueryDatabase(existing_webhook_query);
		if(!existing_webhook_results.Success()) {
			c->Message(Chat::Red, "Unknown error.");
			return;
		}
		
		if (existing_webhook_results.RowCount() == 0) {
			// No existing webhook, insert new
			std::string guild_name = "<" + c->GetGuildName() + ">";
			std::string new_webhook_query = fmt::format(
				"INSERT INTO discord_webhooks "
				"(webhook_name, webhook_url, created_at, deleted_at) "
				"VALUES ('{}', '{}', '{}', NULL) "
				"RETURNING id",
				guild_name,
				webhook_url,
				get_timestamp()
			);
			auto new_webhook_results = database.QueryDatabase(new_webhook_query);
			
			// Get the newly-created Webhook ID
			auto new_webhook_row = new_webhook_results.begin();
			auto new_webhook_id = new_webhook_row[0];
			
			// Update the Guild table with the new Webhook ID
			std::string update_guild_webhook_query = fmt::format(
				"UPDATE guilds "
				"SET webhook_id = {} "
				"WHERE id = {}",
				new_webhook_id,
				guild_id
			);
			database.QueryDatabase(update_guild_webhook_query);
			c->Message(Chat::White, "Webhook added.");
		} else {
			// Found an existing webhook, update it.
			auto webhook_row = existing_webhook_results.begin();
			auto webhook_id = webhook_row[0];
			std::string update_webhook_query = fmt::format(
				"UPDATE discord_webhooks "
				"SET webhook_url = '{}' "
				"WHERE id = {}",
				webhook_url,
				webhook_id
			);
			database.QueryDatabase(update_webhook_query);
			c->Message(Chat::White, "Webhook updated.");
		}
		
		// Reload Log Settings from Database to apply changes
		// Not working, should replicate "#reload logs" which does though
		zone->SendReloadMessage("Log Settings");
		LogSys.LoadLogDatabaseSettings();
		player_event_logs.ReloadSettings();
		
		return;
	}
	
	// Default, for "#guildhook <webhook url>" 
	std::string webhook_url = sep->argplus[1];
	
	if (!Strings::Contains(webhook_url, "https://")) {
		c->Message(Chat::Red, "You need to enter a valid https:// URL");
		return;
	}
	
	// Generate a random code to be used for validation
	std::random_device rd;
    std::mt19937 gen(rd()); 
	std::uniform_int_distribution<> distrib(1000, 9999);
	int fourDigitCode = distrib(gen);
	std::string fourDigitCodeString = std::to_string(fourDigitCode);
	
	// Store details for validation use
	webhook_validation_map.insert_or_assign(guild_id,std::make_pair(fourDigitCodeString, webhook_url));
	
	// Send validation code to webhook
	Discord::SendWebhookMessage("Use this command to validate: `#guildhook validate " + fourDigitCodeString + "`", webhook_url);
	c->Message(Chat::White, "Validation code sent. Check to make sure the message was received, then enter the provided code here.");
	
	return;
}
