#include "Components/TimeKillComponent.h"
#include "Meta/MetaReflectImplement.h"
using namespace Perry;

IMPLEMENT_REFLECT_SYSTEM(TimeKillSystem)
FINISH_REFLECT()


void TimeKillSystem::Update(entt::registry& reg)
{
	for (auto&& [e, decal] : reg.view<TimeKillComponent>().each())
	{
		decal.m_Time -= GetDeltaTime();
		if (decal.m_Time < 0)
			reg.destroy(e);
	}
}

TimeKillComponent::TimeKillComponent(float time)
	:m_Time(time)
{
}