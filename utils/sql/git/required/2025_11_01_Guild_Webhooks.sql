ALTER TABLE `guilds` 
ADD COLUMN `webhook_id` int(11) DEFAULT NULL AFTER `raid_enabled`;