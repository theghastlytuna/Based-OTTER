#pragma once
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "common.h"
#include "fmod_errors.h"

/**
 */

class SoundManaging final {
public:
	SoundManaging() {
		//Common_Init(&extradriverdata);

		FMOD::Studio::System::create(&studioSystem);

		FMOD::System* coreSystem = NULL;
		(studioSystem->getCoreSystem(&coreSystem));
		(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

		studioSystem->initialize(128, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extradriverdata);
		
		FMOD::System_Create(&system);
		system->init(32, FMOD_INIT_NORMAL, extradriverdata);
		
	};
	~SoundManaging() = default;

	struct soundData
	{
		FMOD::Sound* sound;
		std::string name;
		const char* path;
		unsigned int soundLenMS;
		float currentDurationMS;
	};

	struct eventData
	{
		FMOD::Studio::EventInstance* instance;
		std::string name;
		const char* path;
	};

	inline void UpdateSounds(float deltaT) {
		for each (soundData sampleSound in sounds)
		{
			sampleSound.currentDurationMS += deltaT * 1000;
		}

		system->update();
		studioSystem->update();
		Common_Update();
	}

	inline void LoadSound(const char* filePath, std::string name)
	{
		FMOD::Sound* sound;
		unsigned int lenMS;

		system->createSound(filePath, FMOD_DEFAULT, 0, &sound);
		sound->getLength(&lenMS, FMOD_TIMEUNIT_MS);

		std::string tempStr = lowercase(name);

		soundData newSound;
		newSound.sound = sound;
		newSound.name = tempStr;
		newSound.path = filePath;
		newSound.soundLenMS = lenMS;
		newSound.currentDurationMS = (float)lenMS;

		sounds.push_back(newSound);
	}

	inline void PlaySound(std::string name)
	{
		std::string tempStr = lowercase(name);

		for each (soundData sampleSound in sounds)
		{
			if (sampleSound.name == tempStr)
			{
				if (sampleSound.currentDurationMS >= sampleSound.soundLenMS)
				{
					system->playSound(sampleSound.sound, 0, false, &channel);
					sampleSound.currentDurationMS = 0.0f;
					channel->setVolume(volume);

					return;
				}
			}
		}
	}

	inline void LoadBank(const char* filename)
	{
		studioSystem->loadBankFile(filename, FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
	}

	inline void LoadStringBank(const char* filename)
	{
		studioSystem->loadBankFile(filename, FMOD_STUDIO_LOAD_BANK_NORMAL, &stringBank);
	}

	inline void SetEvent(const char* path, std::string name)
	{

		//FMOD_RESULT result;
		FMOD::Studio::EventInstance* instance;

		FMOD::Studio::EventDescription* tempDesc = NULL;
		
		studioSystem->getEvent(path, &tempDesc);

		tempDesc->createInstance(&instance);

		eventData newEvent;
		newEvent.instance = instance;
		newEvent.name = lowercase(name);
		newEvent.path = path;

		events.push_back(newEvent);
	}

	inline void PlayEvent(std::string name)
	{
		std::string tempStr = lowercase(name);

		for each (eventData sampleEvent in events)
		{
			if (sampleEvent.name == tempStr)
			{
				sampleEvent.instance->start();

				return;
			}

		}
	}

	inline void StopSounds()
	{
		channel->stop();

		for each (eventData sampleEvent in events)
		{
			sampleEvent.instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		}
	}

	inline void SetVolume(float inVolume)
	{
		volume = inVolume;

		for each (eventData sampleEvent in events)
		{
			std::cout << "FMOD ERROR for setting volume: " << FMOD_ErrorString(sampleEvent.instance->setParameterByName("Volume", inVolume));
		}
	}

	static inline SoundManaging& Current() { return _singleton; }

	std::string lowercase(std::string origStr)
	{
		//Make a temporary string
		std::string newStr = "";

		//Go through the input name, convert it to lowercase, and add each lowercase character to tempStr
		//Note: converted to lowercase to make it easier to search for names
		for (int i = 0; i < origStr.length(); i++)
		{
			newStr += std::tolower(origStr[i]);
		}

		return newStr;
	}

protected:
	friend class Application;

	static SoundManaging _singleton;

	std::vector<soundData> sounds;
	std::vector<eventData> events;

	FMOD::Studio::System	*studioSystem = NULL;
	FMOD::Studio::Bank		*bank = nullptr;
	FMOD::Studio::Bank		*stringBank = nullptr;

	FMOD::System			*system = nullptr;
	FMOD::Channel			*channel = 0;
	void					*extradriverdata = nullptr;

	float volume = 0.5f;
};

inline SoundManaging SoundManaging::_singleton = SoundManaging();