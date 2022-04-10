#pragma once
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include <string>
#include <vector>
#include <iostream>
//#include "common.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

//Function for converting a glm vector to an fmod vector
inline FMOD_VECTOR GlmToFmod3D(glm::vec3 inVec)
{
	FMOD_VECTOR tempVec;
	tempVec.x = inVec.x;
	tempVec.y = inVec.y;
	tempVec.z = inVec.z;

	return tempVec;
}

//Function for normalizing an fmod vector
inline FMOD_VECTOR Normalize(FMOD_VECTOR inVec)
{
	FMOD_VECTOR tempVec;

	tempVec.x = 1 / glm::sqrt(inVec.x * inVec.x + inVec.y * inVec.y + inVec.z * inVec.z) * inVec.x;
	tempVec.y = 1 / glm::sqrt(inVec.x * inVec.x + inVec.y * inVec.y + inVec.z * inVec.z) * inVec.y;
	tempVec.z = 1 / glm::sqrt(inVec.x * inVec.x + inVec.y * inVec.y + inVec.z * inVec.z) * inVec.z;

	return tempVec;
}

//Function for converting an std::string to lowercase
inline std::string lowercase(std::string origStr)
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

class SoundManaging final {
public:
	SoundManaging() {
		//Common_Init(&extradriverdata);

		//Create an fmod studio system (use for events)
		FMOD::Studio::System::create(&studioSystem);

		//Set up the studio system
		FMOD::System* coreSystem = NULL;
		(studioSystem->getCoreSystem(&coreSystem));
		(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

		//Initialize the studio system
		studioSystem->initialize(128, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extradriverdata);
		
		//Create an fmod system (use for playing mp3s, wavs, etc.)
		FMOD::System_Create(&system);

		//Initialize the system
		system->init(32, FMOD_INIT_NORMAL, extradriverdata);

		//Specify the number of listeners (same as # of players)
		studioSystem->setNumListeners(2);
		
	};
	~SoundManaging() = default;

	//Struct for holding sound data of mp3s, wavs, etc.
	struct soundData
	{
		FMOD::Sound* sound;
		std::string name;
		const char* path;
		unsigned int soundLenMS;
		float currentDurationMS;
	};

	//Struct for holding sata of events
	struct eventData
	{
		FMOD::Studio::EventInstance* instance;
		std::string name;
		const char* path;
		Gameplay::GameObject* object = nullptr;
	};

	//Call this once per frame
	inline void UpdateSounds(float deltaT) {
		
		//Update timing info for sounds (not relevant to events)
		for each (soundData sampleSound in sounds)
		{
			sampleSound.currentDurationMS += deltaT * 1000;
		}

		//If we have a listener, meaning we want 3D sound
		if (listener1Object != nullptr)
		{
			//Grab references to the two players' cameras to use for up.forward calculations
			Gameplay::Camera::Sptr p1Cam = listener1Object->GetScene()->PlayerCamera;
			Gameplay::Camera::Sptr p2Cam = listener1Object->GetScene()->PlayerCamera2;

			///////Update the first listener's attributes
			glm::vec3 cameraLocalForward = glm::vec3(p1Cam->GetView()[0][2], p1Cam->GetView()[1][2], p1Cam->GetView()[2][2]);
			glm::vec3 cameraLocalUp = glm::vec3(p1Cam->GetView()[0][1], p1Cam->GetView()[1][1], p1Cam->GetView()[2][1]);

			listener1Attribs.forward = Normalize(GlmToFmod3D(cameraLocalForward));
			listener1Attribs.up = Normalize(GlmToFmod3D(cameraLocalUp));

			listener1Attribs.position = GlmToFmod3D(listener1Object->GetPosition());
			//std::cout << "FMOD ERROR for setting 1st attribs: " << FMOD_ErrorString(studioSystem->setListenerAttributes(0, &listener1Attribs));
			studioSystem->setListenerAttributes(0, &listener1Attribs);
			////////////////
			
			//////////Update the second listener's attributes
			cameraLocalForward = glm::vec3(p2Cam->GetView()[0][2], p2Cam->GetView()[1][2], p2Cam->GetView()[2][2]);
			cameraLocalUp = glm::vec3(p2Cam->GetView()[0][1], p2Cam->GetView()[1][1], p2Cam->GetView()[2][1]);

			listener2Attribs.forward = Normalize(GlmToFmod3D(cameraLocalForward));
			listener2Attribs.up = Normalize(GlmToFmod3D(cameraLocalUp));

			listener2Attribs.position = GlmToFmod3D(listener2Object->GetPosition());
			studioSystem->setListenerAttributes(1, &listener2Attribs);
			///////////////

			//Look through every event
			for each (eventData sampleEvent in events)
			{
				if (sampleEvent.object != nullptr)
				{
					//Update any instances that have 3D sounds.
					//This is mainly to ensure that for actions such as jumping or walking, 
					//the player who performed the action doesn't move away from the sound as it's playing
					listener1Attribs.position = GlmToFmod3D(sampleEvent.object->GetPosition());
					sampleEvent.instance->set3DAttributes(&listener1Attribs);
				}
			}
		}

		//Update the systems
		system->update();
		studioSystem->update();
		//Common_Update();
	}

	//For loading mp3s, wavs, etc.
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

	//For playing mp3s, wavs, etc.
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

	//For loading in banks (e.g. master bank)
	inline void LoadBank(const char* filename)
	{
		studioSystem->loadBankFile(filename, FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
	}

	//For loading in string banks (e.g. master string bank)
	inline void LoadStringBank(const char* filename)
	{
		studioSystem->loadBankFile(filename, FMOD_STUDIO_LOAD_BANK_NORMAL, &stringBank);
	}

	//For initializing events
	inline void SetEvent(const char* path, std::string name)
	{
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

	/*
	* Play an event that has been loaded through SetEvent
	* name: the name of the event to be played
	* object: the object that corresponds to the event being played. This is for 3D sound, so if the event being played should not be 3D, leave this blank
	*/
	inline void PlayEvent(std::string name, Gameplay::GameObject *object = nullptr)
	{
		std::string tempStr = lowercase(name);

		//Look through every event
		for (int i = 0; i < events.size(); i++)
		{
			//IF we find the event specified by the input
			if (events[i].name == tempStr)
			{
				//Start the event
				events[i].instance->start();
				
				//If the sound is 3D, assign the corresponding object and update the attributes' position
				if (object != nullptr)
				{
					listener1Attribs.position = GlmToFmod3D(object->GetPosition());
					events[i].instance->set3DAttributes(&listener1Attribs);
					events[i].object = object;
				}

				return;
			}

		}
	}

	//Call this whenever we want all sounds in the scene to stop playing (both mp3s/wavs and events)
	inline void StopSounds()
	{
		channel->stop();

		for each (eventData sampleEvent in events)
		{
			sampleEvent.instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		}
	}

	//Sets the volume for all sounds (mp3s/wavs and events)
	inline void SetVolume(float inVolume)
	{
		volume = inVolume;

		for each (eventData sampleEvent in events)
		{
			sampleEvent.instance->setParameterByName("Volume", inVolume);
		}
	}

	//Function for setting the corresponding objects to each listener, e.g. object1 = player1 = location of listener 1.
	//Call this after a new scene is loaded that needs 3D sounds
	inline void SetListenerObjects(Gameplay::GameObject::Sptr inObject1, Gameplay::GameObject::Sptr inObject2)
	{
		listener1Object = inObject1;
		listener2Object = inObject2;
	}

	//For grabbing the reference to the sound managing singleton
	static inline SoundManaging& Current() { return _singleton; }

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

	//Listener 1 stuff
	Gameplay::GameObject::Sptr listener1Object = nullptr;
	FMOD_3D_ATTRIBUTES listener1Attribs = { { 0 } };

	//Listener 2 stuff
	Gameplay::GameObject::Sptr listener2Object = nullptr;
	FMOD_3D_ATTRIBUTES listener2Attribs = { { 0 } };
};

inline SoundManaging SoundManaging::_singleton = SoundManaging();