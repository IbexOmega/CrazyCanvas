#include "GUI/ServerInfo.h"

#include "Log/Log.h"

#pragma warning( push, 0 )
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#pragma warning( pop )
#include <fstream>


namespace LambdaEngine
{
	rapidjson::Document SavedServerSystem::s_SavedServerDocument;

	bool SavedServerSystem::LoadIpsFromFile(TArray<String>& ipAddressess)
	{
		const char* pSavedServersPath = "Saved_Servers.json";

		FILE* pFile = fopen(pSavedServersPath, "r");
		if (!pFile)
		{
			// TODO: We should probably create a default so that the user does not need to have a config file to even run the application
			LOG_ERROR("Server file could not be opened: %s", pSavedServersPath);
			DEBUGBREAK();
			return false;
		}

		char readBuffer[2048];
		rapidjson::FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		s_SavedServerDocument.ParseStream(inputStream);

		const rapidjson::Value& a = s_SavedServerDocument["SERVER_IP"];

		for (rapidjson::SizeType i = 0; i < a.Size(); i++) // Uses SizeType instead of size_t
		{
			LOG_ERROR("Server%d = %s\n", i, a[i].GetString());
			ipAddressess.PushBack(a[i].GetString());
		}

		fclose(pFile);

		return true;
	}

	bool SavedServerSystem::WriteIpsToFile(const char* ip)
	{
		const char* pSavedServersPath = "Saved_Servers.json";

		FILE* pFile = fopen(pSavedServersPath, "r");
		if (!pFile)
		{
			LOG_WARNING("Config file could not be opened: %s", pSavedServersPath);
			return false;
		}

		char readBuffer[2048];
		rapidjson::FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		s_SavedServerDocument.ParseStream(inputStream);

		fclose(pFile);


		rapidjson::Value& ips = s_SavedServerDocument["SERVER_IP"];
		assert(ips.IsArray());
		rapidjson::Document::AllocatorType& allocator = s_SavedServerDocument.GetAllocator();

		bool isNew = true;

		for (rapidjson::SizeType i = 0; i < ips.Size(); i++) // Uses SizeType instead of size_t
		{
			if (ips[i] == ip)
			{
				isNew = false;
			}
		}

		if (isNew)
		{
			rapidjson::Value newIP(ip, allocator);
			ips.PushBack(newIP, allocator);
		}
		else
		{
			return false;
		}

		pFile = fopen(pSavedServersPath, "w");
		if (!pFile)
		{
			LOG_WARNING("Config file could not be opened: %s", pSavedServersPath);
			return false;
		}



		rapidjson::FileWriteStream outputStream(pFile, readBuffer, sizeof(readBuffer));

		rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(outputStream);

		s_SavedServerDocument.Accept(writer);

		fclose(pFile);

		return true;
	}

	void SavedServerSystem::CreateFile()
	{
		const char* pSavedServersPath = "Saved_Servers.json";

		FILE* pFile = fopen(pSavedServersPath, "w");
		if (!pFile)
		{
			LOG_WARNING("Config file could not be opened: %s", pSavedServersPath);
			DEBUGBREAK();
		}


	}
}