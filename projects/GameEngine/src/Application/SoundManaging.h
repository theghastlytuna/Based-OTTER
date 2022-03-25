#pragma once
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "common.h"
#include "fmod_errors.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

/**/
inline FMOD_VECTOR GlmToFmod3D(glm::vec3 inVec)
{
	FMOD_VECTOR tempVec;
	tempVec.x = inVec.x;
	tempVec.y = inVec.y;
	tempVec.z = inVec.z;

	return tempVec;
}

inline FMOD_VECTOR Normalize(FMOD_VECTOR inVec)
{
	FMOD_VECTOR tempVec;

	tempVec.x = 1 / glm::sqrt(inVec.x * inVec.x + inVec.y * inVec.y + inVec.z * inVec.z) * inVec.x;
	tempVec.y = 1 / glm::sqrt(inVec.x * inVec.x + inVec.y * inVec.y + inVec.z * inVec.z) * inVec.y;
	tempVec.z = 1 / glm::sqrt(inVec.x * inVec.x + inVec.y * inVec.y + inVec.z * inVec.z) * inVec.z;

	return tempVec;
}

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

	struct eventData
	{
		FMOD::Studio::EventInstance* instance;
		std::string name;
		const char* path;
		Gameplay::GameObject* object = nullptr;
	};

	inline void UpdateSounds(float deltaT) {
		for each (soundData sampleSound in sounds)
		{
			sampleSound.currentDurationMS += deltaT * 1000;
		}

		if (listener1Object != nullptr)
		{

			Gameplay::Camera::Sptr p1Cam = listener1Object->GetScene()->PlayerCamera;

			glm::vec3 cameraLocalForward = glm::vec3(p1Cam->GetView()[0][2], p1Cam->GetView()[1][2], p1Cam->GetView()[2][2]);
			glm::vec3 cameraLocalUp = glm::vec3(p1Cam->GetView()[0][1], p1Cam->GetView()[1][1], p1Cam->GetView()[2][1]);

			listener1Attribs.forward = Normalize(GlmToFmod3D(cameraLocalForward));
			listener1Attribs.up = Normalize(GlmToFmod3D(cameraLocalUp));

			listener1Attribs.position = GlmToFmod3D(listener1Object->GetPosition());
			studioSystem->setListenerAttributes(0, &listener1Attribs);

			for each (eventData sampleEvent in events)
			{
				if (sampleEvent.object != nullptr)
				{
					listener1Attribs.position = GlmToFmod3D(sampleEvent.object->GetPosition());
					sampleEvent.instance->set3DAttributes(&listener1Attribs);
				}
			}
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

		if (name == "Jump") tempDesc->is3D(&is3D);

		std::cout << path << ' ' << is3D << std::endl;

		eventData newEvent;
		newEvent.instance = instance;
		newEvent.name = lowercase(name);
		newEvent.path = path;

		events.push_back(newEvent);
	}


	/*
	* Play an event that has been loaded through SetEvent
	* name: the name of the event to be played
	* object: the object that corresponds to the event being played. This is for 3D sound, so if the event being played should not be 3D, leave this blank
	*/
	inline void PlayEvent(std::string name, Gameplay::GameObject *object = nullptr)
	{
		std::string tempStr = lowercase(name);

		for (int i = 0; i < events.size(); i++)
		{
			if (events[i].name == tempStr)
			{
				events[i].instance->start();
				
				if (object != nullptr)
				{
					listener1Attribs.position = GlmToFmod3D(object->GetPosition());
					std::cout << "FMOD ERROR for setting event attribs: " << FMOD_ErrorString(events[i].instance->set3DAttributes(&listener1Attribs)) << std::endl;
					events[i].object = object;
				}

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

	inline void SetListenerObjects(Gameplay::GameObject::Sptr inObject1, Gameplay::GameObject::Sptr inObject2)
	{
		listener1Object = inObject1;
		listener2Object = inObject2;
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

	//Vectors for holding sound/event data
	std::vector<soundData> sounds;
	std::vector<eventData> events;

	//Systems
	FMOD::Studio::System	*studioSystem = NULL;
	FMOD::System* system = nullptr;

	//Banks
	FMOD::Studio::Bank		*bank = nullptr;
	FMOD::Studio::Bank		*stringBank = nullptr;

	//Extra shit
	FMOD::Channel			*channel = 0;
	void					*extradriverdata = nullptr;
	float volume = 0.5f;

	//Listener stuff
	Gameplay::GameObject::Sptr listener1Object = nullptr;
	FMOD_3D_ATTRIBUTES listener1Attribs = { { 0 } };

	Gameplay::GameObject::Sptr listener2Object = nullptr;
	FMOD_3D_ATTRIBUTES listener2Attribs = { { 0 } };

	FMOD_3D_ATTRIBUTES eventAttribs = { { 0 } };

	bool is3D;

};

inline SoundManaging SoundManaging::_singleton = SoundManaging();