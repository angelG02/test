#include "Meta/MetaReflectImplement.h"
#include "Meta/MetaReflect.h"
#include "Components/HealthComponent.h"
#include "ECS/Components/CollisionComponent.h"

#include "Input.h"

IMPLEMENT_REFLECT_COMPONENT(HealthComponent)
{
	meta.func<&entt::registry::emplace<HealthComponent>>(f_AddComponent);
	meta.data<&HealthComponent::m_HealthMax>("M_HealthMax"_hs)
		PROP_DISPLAYNAME("Max Health");

	meta.data<&HealthComponent::m_HealthCurrent>("m_Health"_hs)
		PROP_DISPLAYNAME("Current Health");

	meta.data<&HealthComponent::m_InvSecondsDuration>("M_IFrameDuration"_hs)
		PROP_DISPLAYNAME("iFrame Duration");
}
FINISH_REFLECT()

HealthComponent::HealthComponent(float maxHealth, float iFrameDuration)
{
	m_HealthMax = maxHealth;
	m_HealthCurrent = m_HealthMax;
	m_InvSecondsDuration = iFrameDuration;
	m_IFrameTimer = 0.f;
};



IMPLEMENT_REFLECT_SYSTEM(HealthSystem)
{

}
FINISH_REFLECT()

void OnHealthCreated(entt::registry& reg, entt::entity e)
{
	auto& health = reg.get<HealthComponent>(e);
	health.m_HealthCurrent = health.m_HealthMax;
}

void HealthSystem::Init(entt::registry& reg)
{
	reg.on_construct<HealthComponent>().connect<&OnHealthCreated>();
};

void HealthSystem::Update(entt::registry& reg)
{
	auto t = reg.view<HealthComponent>();

	for (auto&& [entity, health] : t.each())
	{
		// Serial purposes xd
		if (!health.m_Created)
		{
			health.m_HealthCurrent = health.m_HealthMax;
			health.m_Created = true;
		}

		// iframe logic
		if (health.m_IFrameTimer > 0.f)
		{
			health.m_IFrameTimer -= GetDeltaTime();
		}

		// check for death state
		if (health.m_HealthCurrent <= 0.f)
		{
			health.m_HealthCurrent = 0.f;
			// this is where death state should be triggered
		}
	}
};

void HealthSystem::Damage(entt::entity e, float damage)
{
	if (!Perry::GetRegistry().try_get<HealthComponent>(e)) return;

	auto& health = Perry::GetRegistry().get<HealthComponent>(e);

	// check if player can be damaged
	if (!(health.m_IFrameTimer > 0.f) && health.m_HealthCurrent > 0.f)
	{
		// damage application
		health.m_HealthCurrent -= damage;
		health.m_IFrameTimer = health.m_InvSecondsDuration;
	}
}