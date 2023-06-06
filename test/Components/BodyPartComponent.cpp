#include "Components/BodyPartComponent.h"

BodyPartComponent::BodyPartComponent(entt::entity parent, float size, int partID) :
	m_Parent(parent), m_Size(size), m_PartID(partID)
{
}

BodyPartComponent::BodyPartComponent(float size, int partID) :
	m_Size(size), m_PartID(partID)
{
}