#include "World/LevelManager.h"

#include "Resources/ResourceLoader.h"

#include "Log/Log.h"

#include "Utilities/SHA256.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

LambdaEngine::TArray<LambdaEngine::String> LevelManager::s_LevelNames;
LambdaEngine::TArray<LevelManager::LevelDesc> LevelManager::s_LevelDescriptions;

bool LevelManager::Init()
{
	using namespace LambdaEngine;
	using namespace rapidjson;

	FILE* pLevelsFile = fopen("../Assets/World/levels.json", "r");

	if (pLevelsFile == nullptr)
	{
		LOG_ERROR("[LevelManager]: Failed to open levels file");
		return false;
	}
	else
	{
		constexpr uint32 READ_BUFFER_SIZE = 65536;
		char* pReadBuffer = DBG_NEW char[READ_BUFFER_SIZE]; //We allocate this on the heap, otherwise Visual Studio gives warning because its large
		FileReadStream inputStream(pLevelsFile, pReadBuffer, READ_BUFFER_SIZE);

		Document d;
		d.ParseStream(inputStream);

		String byteRepresentation;
		THashTable<String, String> moduleByteRepresentations;

		if (d.HasMember("levels"))
		{
			GenericArray levels = d["levels"].GetArray();

			s_LevelNames.Resize(levels.Size());
			s_LevelDescriptions.Resize(levels.Size());
			
			for (uint32 l = 0; l < levels.Size(); l++)
			{
				GenericObject level = levels[l].GetObject();

				LevelDesc levelDesc = {};
				levelDesc.Name = level.HasMember("name") ? level["name"].GetString() : "No Name " + l;

				byteRepresentation.resize(levelDesc.Name.length());
				memcpy(byteRepresentation.data(), levelDesc.Name.data(), levelDesc.Name.length());

				if (level.HasMember("modules"))
				{
					GenericArray levelModules = level["modules"].GetArray();

					levelDesc.LevelModules.Resize(levelModules.Size());

					for (uint32 m = 0; m < levelModules.Size(); m++)
					{
						GenericObject levelModule = levelModules[m].GetObject();

						ModuleDesc moduleDesc = {};

						if (levelModule.HasMember("filename"))
						{
							moduleDesc.Filename = levelModule["filename"].GetString();

							//Load module and store byte representation
							auto moduleByteRepresentationIt = moduleByteRepresentations.find(moduleDesc.Filename);

							uint32 currentByteRepresentationSize = byteRepresentation.size();
							if (moduleByteRepresentationIt == moduleByteRepresentations.end())
							{
								byte* pData;
								uint32 dataSize;
								if (!ResourceLoader::ReadDataFromFile("../Assets/World/Levels/" + moduleDesc.Filename, "rb", &pData, &dataSize))
								{
									LOG_ERROR("[LevelManager]: Failed to load level %s, module %s does not exist", levelDesc.Name.c_str(), moduleDesc.Filename.c_str());
									return false;
								}

								moduleByteRepresentationIt = moduleByteRepresentations.insert({ moduleDesc.Filename, String(reinterpret_cast<char*>(pData), dataSize) }).first;
								SAFEDELETE(pData);
							}

							byteRepresentation.resize(currentByteRepresentationSize + moduleByteRepresentationIt->second.size());
							memcpy(byteRepresentation.data() + currentByteRepresentationSize, moduleByteRepresentationIt->second.data(), moduleByteRepresentationIt->second.size());
						}
						else
						{
							LOG_ERROR("[LevelManager]: Failed to load level %s, module %d has no filename", levelDesc.Name.c_str(), m);
							return false;
						}

						if (levelModule.HasMember("translation"))
						{
							GenericObject translation = levelModule["translation"].GetObject();

							moduleDesc.Translation.x = translation.HasMember("x") ? translation["x"].GetFloat() : 0.0f;
							moduleDesc.Translation.y = translation.HasMember("y") ? translation["y"].GetFloat() : 0.0f;
							moduleDesc.Translation.z = translation.HasMember("z") ? translation["z"].GetFloat() : 0.0f;

							uint32 currentByteRepresentationSize = byteRepresentation.size();
							byteRepresentation.resize(currentByteRepresentationSize + sizeof(moduleDesc.Translation));
							memcpy(byteRepresentation.data() + currentByteRepresentationSize, glm::value_ptr(moduleDesc.Translation), sizeof(moduleDesc.Translation));
						}
						else
						{
							LOG_WARNING("[LevelManager]: Module %d in level %s, has no translation", m, levelDesc.Name.c_str());
						}

						levelDesc.LevelModules[m] = moduleDesc;
					}
				}

				String hash = SHA256::Hash(byteRepresentation);
				memcpy(levelDesc.SHA256, hash.data(), hash.size());

				s_LevelNames[l]			= levelDesc.Name;
				s_LevelDescriptions[l]	= levelDesc;

				byteRepresentation.clear();

				D_LOG_INFO("\n[LevelManager]: Level Loaded:\nName: %s\nNum Modules: %d\nSHA256: %d%d%d%d\n",
					levelDesc.Name.c_str(),
					levelDesc.LevelModules.GetSize(),
					levelDesc.SHA256Chunk0,
					levelDesc.SHA256Chunk1,
					levelDesc.SHA256Chunk2,
					levelDesc.SHA256Chunk3);
			}
		}

		SAFEDELETE_ARRAY(pReadBuffer);
		fclose(pLevelsFile);
	}
	
	return true;
}

const LambdaEngine::TArray<LambdaEngine::String>& LevelManager::GetLevelNames()
{
	return s_LevelNames;
}

void LevelManager::LoadLevel(const LambdaEngine::String& levelName)
{
}

void LevelManager::LoadLevel(uint32 index)
{
}
