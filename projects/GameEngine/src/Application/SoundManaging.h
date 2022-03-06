#pragma once
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "common.h"

/**
 * The timing class is a very simple singleton class that will store our timing values
 * per-frame
 */

class SoundManaging final {
public:
	SoundManaging() {
		Common_Init(&extradriverdata);

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

	inline void UpdateSounds(float deltaT) {
		for each (soundData sampleSound in sounds)
		{
			sampleSound.currentDurationMS += deltaT * 1000;
		}

		system->update();
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
				}

				return;
			}
		}
	}

	inline void LoadBank(const char* filename)
	{
		ERRCHECK(studioSystem->loadBankFile(filename, FMOD_STUDIO_LOAD_BANK_NONBLOCKING, &bank));
		//bank->loadSampleData();
		//bank->getSampleLoadingState(state1);

		//bank->getEventList(&events1, 10, numEvents);
		//bank->getEventCount(numEvents);
	}

	inline bool GetStatus()
	{
		return *state1 == FMOD_STUDIO_LOADING_STATE_ERROR;
	}

	inline void LoadStringBank(const char* filename)
	{
		ERRCHECK(studioSystem->loadBankFile(filename, FMOD_STUDIO_LOAD_BANK_NONBLOCKING, &stringBank));
		//stringBank->loadSampleData();
		//stringBank->getSampleLoadingState(state2);

		//stringBank->getEventList(&events2, 10, numEvents2);
		//stringBank->getEventCount(numEvents2);
	}

	inline void SetEvent(const char* path)
	{
		FMOD::Studio::EventDescription* tempDesc = NULL;
		(studioSystem->getEvent("event:/Footsteps", &tempDesc));

		(tempDesc->createInstance(&footsteps));
	}

	inline void PlayInstance()
	{
		(footsteps->start());
	}

	inline int GetNumEvents1()
	{
		return *numEvents;
	}

	inline int GetNumEvents2()
	{
		return *numEvents2;
	}

	inline void PlayEvent(std::string name)
	{
		bank->getEventCount(numEvents);
	}

	inline void StopSounds()
	{
		channel->stop();
	}

	inline void SetVolume(float inVolume)
	{
		volume = inVolume;
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

	FMOD::Studio::System	*studioSystem;
	FMOD::Studio::Bank		*bank;
	FMOD::Studio::Bank		*stringBank;
	int						*numEvents;
	int						*numEvents2;
	FMOD::Studio::EventDescription* events1;
	FMOD::Studio::EventDescription* events2;

	FMOD::Studio::EventInstance* footsteps;

	FMOD_STUDIO_LOADING_STATE* state1;
	FMOD_STUDIO_LOADING_STATE* state2;


	FMOD::System			*system;
	FMOD::Channel			*channel = 0;
	void					*extradriverdata;

	float volume = 0.5f;
};

inline SoundManaging SoundManaging::_singleton = SoundManaging();