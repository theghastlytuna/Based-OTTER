#pragma once
#include "IComponent.h"
#include "Gameplay/Scene.h"

struct GLFWwindow;

class MenuElement : public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<MenuElement> Sptr;

	MenuElement();
	virtual ~MenuElement();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	void GrowElement();
	void ShrinkElement();

	bool IsSelected();

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(MenuElement);
	virtual nlohmann::json ToJson() const override;
	static MenuElement::Sptr FromJson(const nlohmann::json& blob);


private:

	Gameplay::GameObject::Sptr thisElement;

	bool _selected = false;
	
	GLFWwindow* _window;
};


