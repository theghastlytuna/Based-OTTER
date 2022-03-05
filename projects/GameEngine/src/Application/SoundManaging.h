#pragma once
#include "fmod.hpp"
#include <string>
#include <vector>
#include <iostream>

/**
 * The timing class is a very simple singleton class that will store our timing values
 * per-frame
 */
class SoundManaging final {
public:
	SoundManaging() {
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

	inline void LoadEvent(std::string name)
	{

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

	FMOD::System	*system;
	FMOD::Channel	*channel = 0;
	void			*extradriverdata = 0;

	float volume = 0.5f;
};

inline SoundManaging SoundManaging::_singleton = SoundManaging();