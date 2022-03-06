#include "MenuElement.h"
#include "Gameplay/Components/GUI/RectTransform.h"

MenuElement::MenuElement()
	: IComponent()
{ }

MenuElement::~MenuElement() = default;

void MenuElement::Awake()
{
	thisElement = GetGameObject()->SelfRef();
}

void MenuElement::Update(float deltaTime)
{
}

void MenuElement::GrowElement()
{
	glm::vec2 tempMin = thisElement->Get<RectTransform>()->GetMin();
	glm::vec2 tempMax = thisElement->Get<RectTransform>()->GetMax();

	thisElement->Get<RectTransform>()->SetMin(tempMin - glm::vec2(100, 25));
	thisElement->Get<RectTransform>()->SetMax(tempMax + glm::vec2(100, 25));

	_selected = true;
}

void MenuElement::ShrinkElement()
{
	glm::vec2 tempMin = thisElement->Get<RectTransform>()->GetMin();
	glm::vec2 tempMax = thisElement->Get<RectTransform>()->GetMax();

	thisElement->Get<RectTransform>()->SetMin(tempMin + glm::vec2(100, 25));
	thisElement->Get<RectTransform>()->SetMax(tempMax - glm::vec2(100, 25));

	_selected = false;
}

void MenuElement::RenderImGui()
{
}

nlohmann::json MenuElement::ToJson() const
{
	return nlohmann::json();
}

MenuElement::Sptr MenuElement::FromJson(const nlohmann::json& blob)
{
	return MenuElement::Sptr();
}

bool MenuElement::IsSelected()
{
	return _selected;
}
