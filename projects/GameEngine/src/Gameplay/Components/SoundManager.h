#pragma once
#include "IComponent.h"
#include "Gameplay/Scene.h"
#include "fmod.hpp"

struct GLFWwindow;

class SoundManager : public Gameplay::IComponent
{
public:

	typedef std::shared_ptr<SoundManager> Sptr;

	SoundManager();
	virtual ~SoundManager();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	virtual void RenderImGui() override;
	MAKE_TYPENAME(SoundManager);
	virtual nlohmann::json ToJson() const override;
	static SoundManager::Sptr FromJson(const nlohmann::json& blob);

	void LoadSound(const char *filePath, std::string name);

	void PlaySound(std::string name);

	struct soundData 
	{
		FMOD::Sound *sound;
		std::string name;
		const char	*path;
		unsigned int soundLenMS;
		float currentDurationMS;
	};

private:

	std::vector<soundData> sounds;

	FMOD::System	*system;
	FMOD::Channel	*channel;
	void			*extradriverdata;
};

