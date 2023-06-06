#include "Components/ChaseTargetComponent.h"
#include "Meta/MetaReflectImplement.h"
using namespace Perry;
IMPLEMENT_REFLECT_SYSTEM(ChaseTargetSystem)
FINISH_REFLECT()
void OnChaseTargetCreated(entt::registry& reg, entt::entity e)
{
	
	auto&& [chaseTarget, transform] = reg.get<ChaseTargetComponent, TransformComponent>(e);
	chaseTarget.m_OrderID = static_cast<int>(GetCurrentLevel().GetRegistry().group<ChaseTargetComponent, TransformComponent>().size()) - 1;
	
}


ChaseTargetComponent::ChaseTargetComponent(int stateChange)
	:m_StateChange(stateChange)
{ }


void ChaseTargetSystem::Init(entt::registry& reg)
{
	// Needed for PS5 to avoid warning error 
	// Needed to make a system count correctly (initializes it?) ???

	reg.on_construct<ChaseTargetComponent>().connect<&OnChaseTargetCreated>();
}


