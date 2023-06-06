#include "Meta/MetaReflectImplement.h"

#include "Components/BodyPartEnemyComponent.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/VelocityComponent.h"
#include "ECS/Components/CollisionComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ChainResponseComponent.h"
#include "Components/MonsterComponent.h"

BodyPartEnemyComponent::BodyPartEnemyComponent(entt::entity monster) : m_Monster(monster)
{

};

void BodyPartEnemySystem::Init(entt::registry& reg)
{
	
};

void BodyPartEnemySystem::Update(entt::registry& reg)
{
	for (auto&& [entity, enemy, velocity, transform] : reg.view<BodyPartEnemyComponent, Perry::VelocityComponent, Perry::TransformComponent>().each())
	{
		if (reg.any_of<ChainResponseComponent>(entity))
		{
			if (reg.get<ChainResponseComponent>(entity).m_Stunned) 
			{
				return;
			}
		}

		glm::vec3 destination = Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(reg.get<MonsterComponent>(enemy.m_Monster).m_MonsterPartEntities.back()));
		glm::vec3 pos = Perry::Transform::GetPosition(transform);

		auto& current_body_part_health = reg.get<HealthComponent>(entity);

		velocity.m_Velocity = glm::normalize(destination - pos) * m_Speed;
		Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(entity), glm::quatLookAt(glm::normalize(destination - pos), glm::vec3(0.f, 1.f, 0.f)));
		Perry::Transform::AddRotation(reg.get<Perry::TransformComponent>(entity), glm::quat(glm::vec3(90.f, 0.f, 0.f)));

		if (glm::distance(pos, destination) < 3.f)
		{
			MonsterSystem::AddMonsterPart(enemy.m_Monster, current_body_part_health.m_HealthCurrent, current_body_part_health.m_HealthMax, current_body_part_health.m_InvSecondsDuration);

			reg.destroy(entity);
		}
	}
};

IMPLEMENT_REFLECT_SYSTEM(BodyPartEnemySystem)
{
	meta.data<&BodyPartEnemySystem::m_Speed>("BodyPartEnemySpeed"_hs)
		PROP_DISPLAYNAME("Body Part Enemy")
		PROP_DRAGSPEED(1.f);
}
FINISH_REFLECT()