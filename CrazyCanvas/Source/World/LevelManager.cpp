#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"
#include "World/LevelModule.h"
#include "World/Level.h"

#include "Resources/ResourceLoader.h"

#include "Log/Log.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

bool LevelManager::Init()
{
	using namespace LambdaEngine;
	using namespace rapidjson;

	if (!LevelObjectCreator::Init())
	{
		LOG_ERROR("[LevelManager]: Failed to initialize LevelObjectCreator");
		return false;
	}

	FILE* pLevelsFile = fopen((String(LEVELS_JSON_DIRECTORY) + "levels.json").c_str(), "r");

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

					levelDesc.LevelModuleDescriptions.Resize(levelModules.Size());

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
								if (!ResourceLoader::ReadDataFromFile(LEVEL_MODULES_DIRECTORY + moduleDesc.Filename, "rb", &pData, &dataSize))
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

						levelDesc.LevelModuleDescriptions[m] = moduleDesc;
					}
				}

				levelDesc.Hash			= SHA256::Hash(byteRepresentation);

				s_LevelNames[l]			= levelDesc.Name;
				s_LevelDescriptions[l]	= levelDesc;

				byteRepresentation.clear();

				D_LOG_INFO("\n[LevelManager]: Level Registered:\nName: %s\nNum Modules: %d\nSHA256: %x%x\n",
					levelDesc.Name.c_str(),
					levelDesc.LevelModuleDescriptions.GetSize(),
					levelDesc.Hash.SHA256Chunk0,
					levelDesc.Hash.SHA256Chunk1);
			}
		}

		SAFEDELETE_ARRAY(pReadBuffer);
		fclose(pLevelsFile);
	}
	
	return true;
}

bool LevelManager::Release()
{
	for (auto modulesToDeleteIt = s_LoadedModules.begin(); modulesToDeleteIt != s_LoadedModules.end(); modulesToDeleteIt++)
	{
		SAFEDELETE(modulesToDeleteIt->second);
	}
	s_LoadedModules.clear();

	return true;
}

Level* LevelManager::LoadLevel(const LambdaEngine::SHA256Hash& levelHash)
{
	for (uint32 l = 0; l < s_LevelHashes.GetSize(); l++)
	{
		if (levelHash == s_LevelHashes[l])
		{
			return LoadLevel(l);
		}
	}

	LOG_ERROR("[LevelManager]: Can't find level with Hash: %x%x", levelHash.SHA256Chunk0, levelHash.SHA256Chunk1);
	return nullptr;
}

Level* LevelManager::LoadLevel(const LambdaEngine::String& levelName)
{
	for (uint32 l = 0; l < s_LevelNames.GetSize(); l++)
	{
		if (levelName == s_LevelNames[l])
		{
			return LoadLevel(l);
		}
	}

	LOG_ERROR("[LevelManager]: Can't find level with Name: %s", levelName.c_str());
	return nullptr;
}

Level* LevelManager::LoadLevel(uint32 index)
{
	using namespace LambdaEngine;

	if (index < s_LevelDescriptions.GetSize())
	{
		const LevelDesc& levelDesc = s_LevelDescriptions[index];
		LevelCreateDesc levelCreateDesc = {};
		levelCreateDesc.Name = levelDesc.Name;

		//Create copy of loaded modules so that we can reuse similar ones
		THashTable<String, LevelModule*> loadedModules = s_LoadedModules;
		s_LoadedModules.clear();

		for (const ModuleDesc& moduleDesc : levelDesc.LevelModuleDescriptions)
		{
			auto moduleIt = loadedModules.find(moduleDesc.Filename);

			if (moduleIt != loadedModules.end())
			{
				//Module is already loaded, just update translation
				moduleIt->second->SetTranslation(moduleDesc.Translation);
				s_LoadedModules[moduleIt->first] = moduleIt->second;
				levelCreateDesc.LevelModules.PushBack(moduleIt->second);
				loadedModules.erase(moduleIt);
			}
			else
			{
				//Module is not loaded
				LevelModule* pModule = DBG_NEW LevelModule();
				if (!pModule->Init(moduleDesc.Filename, moduleDesc.Translation))
				{
					LOG_ERROR("[LevelManager]: Failed to initialize Level Module");
					return false;
				}

				s_LoadedModules[moduleDesc.Filename] = pModule;
				levelCreateDesc.LevelModules.PushBack(pModule);
			}
		}

		for (auto modulesToDeleteIt = loadedModules.begin(); modulesToDeleteIt != loadedModules.end(); modulesToDeleteIt++)
		{
			SAFEDELETE(modulesToDeleteIt->second);
		}

		Level* pLevel = DBG_NEW Level();

		if (!pLevel->Init(&levelCreateDesc))
		{
			LOG_ERROR("[LevelManager]: Failed to create level %s", levelDesc.Name.c_str());
			SAFEDELETE(pLevel);
			return nullptr;
		}
		else
		{
			D_LOG_INFO("[LevelManager]: Level %s created", levelDesc.Name.c_str());
			return pLevel;
		}
	}

	LOG_ERROR("[LevelManager]: Level with index %d is out of bounds", index);
	return nullptr;
}
