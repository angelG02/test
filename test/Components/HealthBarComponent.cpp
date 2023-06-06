#include "Components/HealthBarComponent.h"

#include "Components/HealthComponent.h"
#include "ECS/Components/SpriteComponent.h"
#include "Meta/MetaReflectImplement.h"

class HealthComponent;
IMPLEMENT_REFLECT_COMPONENT(HealthBarComponent)
{
	meta.data<&HealthBarComponent::m_TargetEntity>("TargetEnitty"_hs)
		PROP_DISPLAYNAME("Target")
		PROP_DESCRIPTION("The entity that has a healthbar This helaht is linked to the bar.")
		PROP_MINMAX(0.f, 1.f);
}
FINISH_REFLECT()

IMPLEMENT_REFLECT_SYSTEM(HealthBarSystem)
{

}
FINISH_REFLECT()

void HealthBarSystem::Update(entt::registry& reg)
{
	System::Update(reg);

	for (auto&& [healthbarEntity, healthBarComponent] : reg.view<HealthBarComponent>().each())
	{
		auto& pivotTransform = reg.get<Perry::TransformComponent>(healthBarComponent.m_PivotEntity);
		auto oldScale = Perry::Transform::GetLocalScale(pivotTransform);

		float factor = 1.f;
		if (reg.valid(healthBarComponent.m_TargetEntity))
		{
			if (HealthComponent* hp = reg.try_get<HealthComponent>(healthBarComponent.m_TargetEntity))
			{
				factor = oldScale.x;
				float newFactor = hp->m_HealthCurrent / hp->m_HealthMax;
				float t = GetDeltaTime() ;
				factor = factor * (1 - t)  +newFactor *t;
			}
		}

		if (glm::isnan(factor)) {
			factor = 0;
		}
		oldScale.x = factor;

		Perry::Transform::SetLocalScale(pivotTransform, oldScale);
	}

}
