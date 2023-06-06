#include "Components/LegsComponent.h"

#include "ECS/Components/JointComponent.h"
#include "Components/MonsterLegComponent.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "Serial/Unserializable.h"
#include "Meta/MetaReflectImplement.h"

#include "Renderer.h"
#include "Level.h"

#include "ECS/Components/MeshComponent.h"

#include <glm/gtc/quaternion.hpp>

void ConstructLegs(entt::registry& reg, entt::entity e)
{
	auto&& legs = reg.get<LegsComponent>(e);
	Perry::Entity legL = Perry::GetCurrentLevel().AddEntity(glm::vec3(0.f, 0.f, -legs.m_Offset));
	Perry::Transform::SetParent(e, legL);
	legL.AddComponent<MonsterLegComponent>(1.5f, 1.8f + float(rand()%100) / 100.f,  3.f + float(rand()%200) / 100.f, LEFT_SIDE, glm::vec3(0.f, -1.f, -1.f), glm::vec3(0.5f, 0.5f, 0.5f), 1.3f, float(rand()%200) / 100.f, 10.f + float(rand()%300) / 100.f);
	legL.SetName("LegL");
	legL.AddComponent<Unserializable>();

	Perry::Entity legR = Perry::GetCurrentLevel().AddEntity(glm::vec3(0.f, 0.f, legs.m_Offset));
	Perry::Transform::SetParent(e, legR);
	legR.AddComponent<MonsterLegComponent>(1.5f,  1.8f + float(rand()%100) / 100.f,  3.f + float(rand()%200) / 100.f, RIGHT_SIDE, glm::vec3(0.f, -1.f, -1.f), glm::vec3(0.5f, 0.5f, 0.5f), 1.3f, float(rand()%200) / 100.f, 10.f + float(rand()%300) / 100.f);
	legR.SetName("LegR");
	legR.AddComponent<Unserializable>();

}

void LegMovementSystem::Init(entt::registry& reg)
{
	reg.on_construct<LegsComponent>().connect<&ConstructLegs>();
}


IMPLEMENT_REFLECT_COMPONENT(LegsComponent)
{

}
FINISH_REFLECT()

IMPLEMENT_REFLECT_SYSTEM(LegMovementSystem)
{
}
FINISH_REFLECT()