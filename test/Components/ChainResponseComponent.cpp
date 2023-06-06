#include "Components/ChainResponseComponent.h"
#include "Meta/MetaReflectImplement.h"

IMPLEMENT_REFLECT_SYSTEM(ChainResponseSystem)
FINISH_REFLECT()

void ChainResponseSystem::Update(entt::registry& reg)
{
	for (auto&& [entity, chain] : reg.view<ChainResponseComponent>().each())
	{
		chain.m_Stunned = (chain.m_Hooked) || (chain.m_StunTimer < chain.m_StunDuration);

		chain.m_StunTimer += GetDeltaTime();
	}
}