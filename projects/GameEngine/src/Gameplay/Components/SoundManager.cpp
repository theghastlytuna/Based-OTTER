#include "SoundManager.h"
#include "Application/Timing.h"

SoundManager::SoundManager()
	: IComponent(),
	channel(0),
	extradriverdata(0)
{ }

SoundManager::~SoundManager() = default;

void SoundManager::Awake()
{
	FMOD::System_Create(&system);
	system->init(32, FMOD_INIT_NORMAL, extradriverdata);
	/*
	{
		LoadSound("Sounds/CD_Drive.wav", "Scene Startup");
		LoadSound("Sounds/Cartoon_Boing.wav", "Jump");
	}
	*/
}

void SoundManager::Update(float deltaTime)
{
	for each (soundData sampleSound in sounds)
	{
		sampleSound.currentDurationMS += Timing::Current().UnscaledDeltaTime() * 1000;
	}
}

void SoundManager::RenderImGui()
{
}

nlohmann::json SoundManager::ToJson() const
{
	return nlohmann::json();
}

SoundManager::Sptr SoundManager::FromJson(const nlohmann::json & blob)
{
	return SoundManager::Sptr();
}

void SoundManager::LoadSound(const char *filePath, std::string name)
{
	FMOD::Sound	*sound;
	unsigned int lenMS;

	system->createSound(filePath, FMOD_DEFAULT, 0, &sound);
	sound->getLength(&lenMS, FMOD_TIMEUNIT_MS);
	
	//Make a temporary string
	std::string tempStr = "";

	//Go through the input name, convert it to lowercase, and add each lowercase character to tempStr
	//Note: converted to lowercase to make it easier to search for names
	for (int i = 0; i < name.length(); i++)
	{
		tempStr += std::tolower(name[i]);
	}

	soundData newSound;
	newSound.sound = sound;
	newSound.name = tempStr;
	newSound.path = filePath;
	newSound.soundLenMS = lenMS;
	newSound.currentDurationMS = (float)lenMS;

	sounds.push_back(newSound);
}

void SoundManager::PlaySound(std::string name)
{
	//Make a temporary string
	std::string tempStr = "";

	//Go through the input name, convert it to lowercase, and add each lowercase character to tempStr
	//Note: converted to lowercase to make it easier to search for names
	for (int i = 0; i < name.length(); i++)
	{
		tempStr += std::tolower(name[i]);
	}

	for each (soundData sampleSound in sounds)
	{
		if (sampleSound.name == tempStr)
		{
			if (sampleSound.currentDurationMS >= sampleSound.soundLenMS)
			{
				system->playSound(sampleSound.sound, 0, false, &channel);
				sampleSound.currentDurationMS = 0.0f;
			}
			
			return;
		}
	}
}
