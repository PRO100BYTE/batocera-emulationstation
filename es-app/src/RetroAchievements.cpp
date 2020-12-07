#include "RetroAchievements.h"
#include "HttpReq.h"
#include "utils/StringUtil.h"
#include "ApiSystem.h"
#include "SystemConf.h"
#include "PlatformId.h"
#include "SystemData.h"

#include "utils/ZipFile.h"
#include "scrapers/md5.h"
#include "ApiSystem.h"
#include "LocaleES.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/pointer.h>
#include <algorithm>

#include "rcheevos/include/rhash.h"
#include "rcheevos/include/rconsoles.h"

using namespace PlatformIds;

const std::map<PlatformId, unsigned short> cheevosConsoleID 
{
	{ ARCADE, RC_CONSOLE_ARCADE },
	{ NEOGEO, RC_CONSOLE_ARCADE },
	
	{ SEGA_MEGA_DRIVE, 1 },
	{ NINTENDO_64, 2 },
	{ SUPER_NINTENDO, 3 },
	{ GAME_BOY, 4 },
	{ GAME_BOY_ADVANCE, 5 },
	{ GAME_BOY_COLOR, 6 },
	{ NINTENDO_ENTERTAINMENT_SYSTEM, 7 },
	{ TURBOGRAFX_16, 8 },
	{ SEGA_CD, 9 },
	{ SEGA_32X, 10 },
	{ SEGA_MASTER_SYSTEM, 11 },
	{ PLAYSTATION, 12 },
	{ ATARI_LYNX, 13 },
	{ NEOGEO_POCKET, 14 },
	{ SEGA_GAME_GEAR, 15 },
	{ NINTENDO_GAMECUBE, 16 },
	{ ATARI_JAGUAR, 17 },
	{ NINTENDO_DS, 18 },
	{ NINTENDO_WII, 19 },
	{ NINTENDO_WII_U, 20 },
	{ PLAYSTATION_2, 21 },
	{ XBOX, 22 },
	{ VIDEOPAC_ODYSSEY2, 23 },
	{ POKEMINI, 24 },
	{ ATARI_2600, 25 },
	{ PC, 26 },
	{ NINTENDO_VIRTUAL_BOY, 28 },
	{ MSX, 29 },
	{ COMMODORE_64, 30 },
	{ ZX81, 31 },
	{ ORICATMOS, 32 },
	{ SEGA_SG1000, 33 },
	{ AMIGA, 35 },
	{ ATARI_ST, 36 },
	{ AMSTRAD_CPC, 37 },
	{ APPLE_II, 38 },
	{ SEGA_SATURN, 39 },
	{ SEGA_DREAMCAST, 40 },
	{ PLAYSTATION_PORTABLE, 41 },
	{ THREEDO, 43 },
	{ COLECOVISION, 44 },
	{ INTELLIVISION, 45 },
	{ VECTREX, 46 },
	{ PC_88, 47 },
	{ PC_98, 48 },
	{ PCFX, 49 },
	{ ATARI_5200, 50 },
	{ ATARI_7800, 51 },
	{ SHARP_X6800, 52 },
	{ WONDERSWAN, 53 },
	{ NEOGEO_CD, 56 },
	{ CHANNELF, 57 },
	{ ZX_SPECTRUM, 59 },
	{ NINTENDO_GAME_AND_WATCH, 60 },
	{ NINTENDO_3DS, 62 }

	// { VIC20, 34 },
	//	{ CDI, 42 },
	//{ CASSETTEVISION, 54 },
	//{ SUPER_CASSETTEVISION, 55 },
	//	{ FM_TOWNS, 58 },
	// { NOKIA_NGAGE, 61 },

};

const std::set<unsigned short> consolesWithmd5hashes 
{
	RC_CONSOLE_APPLE_II,
	RC_CONSOLE_ATARI_2600,
	RC_CONSOLE_ATARI_7800,
	RC_CONSOLE_ATARI_JAGUAR,
	RC_CONSOLE_COLECOVISION,
	RC_CONSOLE_GAMEBOY,
	RC_CONSOLE_GAMEBOY_ADVANCE,
	RC_CONSOLE_GAMEBOY_COLOR,
	RC_CONSOLE_GAME_GEAR,
	RC_CONSOLE_INTELLIVISION,
	RC_CONSOLE_MAGNAVOX_ODYSSEY2,
	RC_CONSOLE_MASTER_SYSTEM,
	RC_CONSOLE_MEGA_DRIVE,
	RC_CONSOLE_MSX,
	RC_CONSOLE_NEOGEO_POCKET,
	RC_CONSOLE_NINTENDO_64,
	RC_CONSOLE_ORIC,
	RC_CONSOLE_PC8800,
	RC_CONSOLE_POKEMON_MINI,
	RC_CONSOLE_SEGA_32X,
	RC_CONSOLE_SG1000,
	RC_CONSOLE_VECTREX,
	RC_CONSOLE_VIRTUAL_BOY,
	RC_CONSOLE_WONDERSWAN
};

const std::string API_DEV_L = { 42, 88, 35, 2, 36, 10, 2, 6, 23, 65, 45, 7, 10, 85, 26, 67, 89, 74, 28, 90, 41, 113, 41, 47, 16, 76, 82, 86, 22, 71, 12, 22, 54, 61, 45, 51, 16, 99, 3, 55, 54, 122, 4, 46, 69, 33, 2, 59, 5, 115 };
const std::string API_DEV_KEY = { 80, 101, 97, 99, 80, 101, 97, 99, 101, 32, 97, 110, 100, 32, 98, 101, 32, 119, 105, 108, 101, 32, 97, 110, 100, 32, 98, 101, 32, 119, 105, 108, 80, 101, 97, 99, 101, 32, 97, 110, 100, 32, 98, 101, 32, 119, 105, 108, 100 };

std::string RetroAchievements::getApiUrl(const std::string method, const std::string parameters)
{
	auto options = Utils::String::scramble(API_DEV_L, API_DEV_KEY);
	return "https://retroachievements.org/API/"+ method +".php?"+ options +"&" + parameters;
}

std::string GameInfoAndUserProgress::getImageUrl(const std::string image)
{
	if (image.empty())
		return "http://i.retroachievements.org" + ImageIcon;
	
	return "http://i.retroachievements.org" + image;
}

std::string Achievement::getBadgeUrl()
{
	if (!DateEarned.empty())
		return "http://i.retroachievements.org/Badge/" + BadgeName + ".png";

	return "http://i.retroachievements.org/Badge/" + BadgeName + "_lock.png";
}


int jsonInt(const rapidjson::Value& val, const std::string name)
{
	if (!val.HasMember(name.c_str()))
		return 0;

	const rapidjson::Value& value = val[name.c_str()];
	
	if (value.IsInt())
		return value.GetInt();
	
	if (value.IsString())
		return Utils::String::toInteger(value.GetString());

	return 0;
}

std::string jsonString(const rapidjson::Value& val, const std::string name)
{
	if (!val.HasMember(name.c_str()))
		return "";

	const rapidjson::Value& value = val[name.c_str()];

	if (value.IsInt())
		return std::to_string(value.GetInt());

	if (value.IsString())
		return value.GetString();

	return "";
}

bool sortAchievements(const Achievement& sys1, const Achievement& sys2)
{
	if (sys1.DateEarned.empty() != sys2.DateEarned.empty())
		return !sys1.DateEarned.empty() && sys2.DateEarned.empty();

	return sys1.DisplayOrder < sys2.DisplayOrder;
}

GameInfoAndUserProgress RetroAchievements::getGameInfoAndUserProgress(int gameId, const std::string userName)
{
	auto usrName = userName;
	if (usrName.empty())
		usrName = SystemConf::getInstance()->get("global.retroachievements.username");

	GameInfoAndUserProgress ret;
	ret.ID = 0;

	HttpReq httpreq(getApiUrl("API_GetGameInfoAndUserProgress", "u=" + usrName + "&g=" + std::to_string(gameId)));
	httpreq.wait();

	rapidjson::Document doc;
	doc.Parse(httpreq.getContent().c_str());
	if (doc.HasParseError())
		return ret;

	ret.ID = jsonInt(doc, "ID");
	ret.Title = jsonString(doc, "Title");
	ret.ConsoleID = jsonInt(doc, "ConsoleID");
	ret.ForumTopicID = jsonInt(doc, "ForumTopicID");
	ret.Flags = jsonInt(doc, "Flags");
	ret.ImageIcon = jsonString(doc, "ImageIcon");
	ret.ImageTitle = jsonString(doc, "ImageTitle");
	ret.ImageIngame = jsonString(doc, "ImageIngame");
	ret.ImageBoxArt = jsonString(doc, "ImageBoxArt");
	ret.Publisher = jsonString(doc, "Publisher");
	ret.Developer = jsonString(doc, "Developer");
	ret.Genre = jsonString(doc, "Genre");
	ret.Released = jsonString(doc, "Released");
	ret.ConsoleName = jsonString(doc, "ConsoleName");
	ret.NumDistinctPlayersCasual = jsonString(doc, "NumDistinctPlayersCasual");
	ret.NumDistinctPlayersHardcore = jsonString(doc, "NumDistinctPlayersHardcore");
	ret.NumAchievements = jsonInt(doc, "NumAchievements");
	ret.NumAwardedToUser = jsonInt(doc, "NumAwardedToUser");
	ret.NumAwardedToUserHardcore = jsonInt(doc, "NumAwardedToUserHardcore");
	ret.UserCompletion = jsonString(doc, "UserCompletion");
	ret.UserCompletionHardcore = jsonString(doc, "UserCompletionHardcore");

	if (doc.HasMember("Achievements"))
	{
		const rapidjson::Value& ra = doc["Achievements"];
		for (auto achivId = ra.MemberBegin(); achivId != ra.MemberEnd(); ++achivId)
		{
			auto& recent = achivId->value;
						
			Achievement item;
			item.ID = jsonString(recent, "ID");
			item.NumAwarded = jsonString(recent, "NumAwarded");
			item.NumAwardedHardcore = jsonString(recent, "NumAwardedHardcore");
			item.Title = jsonString(recent, "Title");
			item.Description = jsonString(recent, "Description");
			item.Points = jsonString(recent, "Points");
			item.TrueRatio = jsonString(recent, "TrueRatio");
			item.Author = jsonString(recent, "Author");
			item.DateModified = jsonString(recent, "DateModified");
			item.DateCreated = jsonString(recent, "DateCreated");
			item.BadgeName = jsonString(recent, "BadgeName");
			item.DisplayOrder = jsonInt(recent, "DisplayOrder");
			item.DateEarned = jsonString(recent, "DateEarned");
			ret.Achievements.push_back(item);
		}
	}

	std::sort(ret.Achievements.begin(), ret.Achievements.end(), sortAchievements);

	return ret;
}

UserSummary RetroAchievements::getUserSummary(const std::string userName, int gameCount)
{
	auto usrName = userName;
	if (usrName.empty())
		usrName = SystemConf::getInstance()->get("global.retroachievements.username");

	UserSummary ret;

	std::string count = std::to_string(gameCount);

	HttpReq httpreq(getApiUrl("API_GetUserSummary", "u="+ usrName +"&g="+ count +"&a="+ count));
	httpreq.wait();

	rapidjson::Document doc;
	doc.Parse(httpreq.getContent().c_str());
	if (doc.HasParseError())
		return ret;

	ret.Username = usrName;
	ret.RecentlyPlayedCount = jsonInt(doc, "RecentlyPlayedCount");
	ret.MemberSince = jsonString(doc, "MemberSince");
	ret.RichPresenceMsg = jsonString(doc, "RichPresenceMsg");
	ret.LastGameID = jsonString(doc, "LastGameID");
	ret.ContribCount = jsonString(doc, "ContribCount");
	ret.ContribYield = jsonString(doc, "ContribYield");
	ret.TotalTruePoints = jsonString(doc, "TotalTruePoints");
	ret.TotalPoints = jsonString(doc, "TotalPoints");
	ret.Permissions = jsonString(doc, "Permissions");
	ret.Untracked = jsonString(doc, "Untracked");
	ret.ID = jsonString(doc, "ID");
	ret.UserWallActive = jsonString(doc, "UserWallActive");
	ret.Motto = jsonString(doc, "Motto");
	ret.Rank = jsonString(doc, "Rank");
	ret.TotalRanked = jsonString(doc, "TotalRanked");
	ret.Points = jsonString(doc, "Points");
	ret.UserPic = jsonString(doc, "UserPic");
	ret.Status = jsonString(doc, "Status");

	if (doc.HasMember("RecentlyPlayed"))
	{
		for (auto& recent : doc["RecentlyPlayed"].GetArray())
		{
			RecentGame item;
			item.GameID = jsonString(recent, "GameID");
			item.ConsoleID = jsonString(recent, "ConsoleID");
			item.ConsoleName = jsonString(recent, "ConsoleName");
			item.Title = jsonString(recent, "Title");
			item.ImageIcon = jsonString(recent, "ImageIcon");
			item.LastPlayed = jsonString(recent, "LastPlayed");
			item.MyVote = jsonString(recent, "MyVote");

			ret.RecentlyPlayed.push_back(item);
		}
	}

	if (doc.HasMember("Awarded"))
	{
		const rapidjson::Value& ra = doc["Awarded"];
		for (auto achivId = ra.MemberBegin(); achivId != ra.MemberEnd(); ++achivId)
		{
			std::string gameID = achivId->name.GetString();
			auto& recent = achivId->value;

			Award item;
			item.NumPossibleAchievements = jsonInt(recent, "NumPossibleAchievements");
			item.PossibleScore = jsonInt(recent, "PossibleScore");
			item.NumAchieved = jsonInt(recent, "NumAchieved");
			item.ScoreAchieved = jsonInt(recent, "ScoreAchieved");
			item.NumAchievedHardcore = jsonInt(recent, "NumAchievedHardcore");
			item.ScoreAchievedHardcore = jsonInt(recent, "ScoreAchievedHardcore");

			ret.Awarded[gameID] = item;
		}
	}

	if (doc.HasMember("RecentAchievements"))
	{
		const rapidjson::Value& ra = doc["RecentAchievements"];		
		for (auto achivId = ra.MemberBegin(); achivId != ra.MemberEnd(); ++achivId)
		{			
			std::string gameID = achivId->name.GetString();

			for (auto itrc = achivId->value.MemberBegin(); itrc != achivId->value.MemberEnd(); ++itrc)
			{
				auto& recent = itrc->value;
				RecentAchievement item;

				item.ID = jsonString(recent, "ID");
				item.GameID = jsonString(recent, "GameID");
				item.GameTitle = jsonString(recent, "GameTitle");
				item.Description = jsonString(recent, "Description");
				item.Points = jsonString(recent, "Points");
				item.BadgeName = jsonString(recent, "BadgeName");
				item.IsAwarded = jsonString(recent, "IsAwarded");
				item.DateAwarded = jsonString(recent, "DateAwarded");
				item.HardcoreAchieved = jsonString(recent, "HardcoreAchieved");
				ret.RecentAchievements[gameID].push_back(item);
			}			
		}
	}

	return ret;
}


RetroAchievementInfo RetroAchievements::toRetroAchivementInfo(UserSummary& ret)
{
	RetroAchievementInfo info;

	info.userpic = "https://retroachievements.org" + ret.UserPic;
	info.rank = ret.Rank;
	info.totalpoints = ret.TotalTruePoints;
	info.username = ret.Username;
	info.registered = ret.MemberSince;

	for (auto played : ret.RecentlyPlayed)
	{
		RetroAchievementGame rg;
		rg.id = played.GameID;		

		if (!played.ImageIcon.empty())
			rg.badge = "http://i.retroachievements.org" + played.ImageIcon;

		rg.name = played.Title + " [" + played.ConsoleName + "]";
		rg.lastplayed = played.LastPlayed;

		auto aw = ret.Awarded.find(played.GameID);
		if (aw != ret.Awarded.cend())
		{
			if (aw->second.NumAchieved == 0 && aw->second.ScoreAchieved == 0)
				continue;

			rg.wonAchievements = aw->second.NumAchieved;
			rg.totalAchievements = aw->second.NumPossibleAchievements;

			rg.achievements = std::to_string(aw->second.NumAchieved) + " of " + std::to_string(aw->second.NumPossibleAchievements);
			rg.points = std::to_string(aw->second.ScoreAchieved) + "/" + std::to_string(aw->second.PossibleScore);			
		}

		info.games.push_back(rg);
	}

	return info;
}

std::map<std::string, std::string> RetroAchievements::getCheevosHashes()
{
	std::map<std::string, std::string> ret;

	try
	{
		HttpReq hashLibrary("https://retroachievements.org/dorequest.php?r=hashlibrary");
		HttpReq officialGamesList("https://retroachievements.org/dorequest.php?r=officialgameslist");

		// Official games
		officialGamesList.wait();

		rapidjson::Document ogdoc;
		ogdoc.Parse(officialGamesList.getContent().c_str());
		if (ogdoc.HasParseError())
			return ret;

		if (!ogdoc.HasMember("Response"))
			return ret;

		std::map<int, std::string> officialGames;

		const rapidjson::Value& response = ogdoc["Response"];
		for (auto it = response.MemberBegin(); it != response.MemberEnd(); ++it)
		{
			int gameId = Utils::String::toInteger(it->name.GetString());

			if (it->value.GetType() == rapidjson::Type::kStringType)
				officialGames[gameId] = it->value.GetString();
			else if (it->value.GetType() == rapidjson::Type::kNumberType)
				officialGames[gameId] = std::to_string(it->value.GetInt());
		}

		// Hash library
		hashLibrary.wait();

		rapidjson::Document doc;
		doc.Parse(hashLibrary.getContent().c_str());
		if (doc.HasParseError())
			return ret;

		if (!doc.HasMember("MD5List"))
			return ret;

		const rapidjson::Value& mdlist = doc["MD5List"];
		for (auto it = mdlist.MemberBegin(); it != mdlist.MemberEnd(); ++it)
		{
			std::string name = Utils::String::toUpper(it->name.GetString());

			if (!it->value.IsInt())
				continue;
			
			int gameId = it->value.GetInt();

			if (officialGames.find(gameId) == officialGames.cend())
				continue;

			ret[name] = std::to_string(gameId);			
		}
	}
	catch (...)
	{

	}

	return ret;
}

#include "formats/cdfs.h"

#define CHEEVOS_FREE(p) do { void* q = (void*)p; if (q) free(q); } while (0)

static void* rc_hash_handle_cd_open_track(const char* path, uint32_t track)
{
	cdfs_track_t* cdfs_track;

	if (track == 0)
		cdfs_track = cdfs_open_data_track(path);
	else
		cdfs_track = cdfs_open_track(path, track);

	if (cdfs_track)
	{
		cdfs_file_t* file = (cdfs_file_t*)malloc(sizeof(cdfs_file_t));
		if (cdfs_open_file(file, cdfs_track, NULL))
			return file;

		CHEEVOS_FREE(file);
	}

	cdfs_close_track(cdfs_track); /* ASSERT: this free()s cdfs_track */
	return NULL;
}

static size_t rc_hash_handle_cd_read_sector(void* track_handle, uint32_t sector, void* buffer, size_t requested_bytes)
{
	cdfs_file_t* file = (cdfs_file_t*)track_handle;

	cdfs_seek_sector(file, sector);
	return cdfs_read_file(file, buffer, requested_bytes);
}

static void rc_hash_handle_cd_close_track(void* track_handle)
{
	cdfs_file_t* file = (cdfs_file_t*)track_handle;
	if (file)
	{
		cdfs_close_track(file->track);
		cdfs_close_file(file); /* ASSERT: this does not free() file */
		CHEEVOS_FREE(file);
	}	
}

static rc_hash_cdreader cdreader;
static bool cdreaderinit = false;

std::string RetroAchievements::getCheevosHashFromFile(int consoleId, const std::string fileName)
{
	try
	{
		if (!cdreaderinit)
		{
			cdreader.open_track = rc_hash_handle_cd_open_track;
			cdreader.read_sector = rc_hash_handle_cd_read_sector;
			cdreader.close_track = rc_hash_handle_cd_close_track;
			cdreader.absolute_sector_to_track_sector = nullptr;
			rc_hash_init_custom_cdreader(&cdreader);
			cdreaderinit = true;
		}

		char hash[33];
		if (rc_hash_generate_from_file(hash, consoleId, fileName.c_str()) != 0)
			return hash;
	}
	catch (...)
	{
	}

	return "";
}

std::string RetroAchievements::getCheevosHash( SystemData* system, const std::string fileName)
{
	bool fromZipContents = system->shouldExtractHashesFromArchives();

	int consoleId = 0;

	for (auto pid : system->getPlatformIds())
	{
		auto it = cheevosConsoleID.find(pid);
		if (it != cheevosConsoleID.cend())
		{
			consoleId = it->second;
			break;
		}
	}

	if (consoleId == RC_CONSOLE_ARCADE)
		return getCheevosHashFromFile(consoleId, fileName);

	if (consoleId == 0 || consolesWithmd5hashes.find(consoleId) != consolesWithmd5hashes.cend())
		return ApiSystem::getInstance()->getMD5(fileName, fromZipContents);

	std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fileName));
	if (ext != ".zip" && ext != ".7z")
		return getCheevosHashFromFile(consoleId, fileName);

	std::string contentFile = fileName;
	std::string ret;
	std::string tmpZipDirectory;

	if (fromZipContents)
	{
		tmpZipDirectory = Utils::FileSystem::getTempPath() + "/" + Utils::FileSystem::getStem(fileName);
		Utils::FileSystem::deleteDirectoryFiles(tmpZipDirectory);

		if (ApiSystem::getInstance()->unzipFile(fileName, tmpZipDirectory))
		{
			auto fileList = Utils::FileSystem::getDirContent(tmpZipDirectory, true);

			std::vector<std::string> res;
			std::copy_if(fileList.cbegin(), fileList.cend(), std::back_inserter(res), [](const std::string file) { return Utils::FileSystem::getExtension(file) != ".txt";  });

			if (res.size() == 1)
				contentFile = *res.cbegin();
		}
	}

	if (consoleId != 0)
		ret = getCheevosHashFromFile(consoleId, contentFile);
	else
		ret = ApiSystem::getInstance()->getMD5(contentFile, false);

	if (!tmpZipDirectory.empty())
	{
		Utils::FileSystem::deleteDirectoryFiles(tmpZipDirectory);
		Utils::FileSystem::removeFile(tmpZipDirectory);
	}

	return ret;
}

bool RetroAchievements::testAccount(const std::string& username, const std::string& password, std::string& error)
{
	if (username.empty() || password.empty())
	{
		error = _("A valid account is required. Please register an account on https://retroachievements.org");
		return false;
	}

	std::map<std::string, std::string> ret;

	try
	{
		HttpReq request("https://retroachievements.org/dorequest.php?r=login&u=" + username + "&p=" + password);
		if (!request.wait())
		{						
			error = request.getErrorMsg();
			return false;
		}

		rapidjson::Document ogdoc;
		ogdoc.Parse(request.getContent().c_str());
		if (ogdoc.HasParseError() || !ogdoc.HasMember("Success"))
		{
			error = "Unable to parse response";
			return false;
		}

		if (ogdoc["Success"].IsTrue())
			return true;		

		if (ogdoc.HasMember("Error"))
			error = ogdoc["Error"].GetString();
	}
	catch (...)
	{
		error = "Unknown error";
	}

	return false;
}