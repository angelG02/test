#include "Components/MonsterLegComponent.h"

#include "ECS/Components/JointComponent.h"
#include "ECS/Components/IKArmComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "Meta/MetaReflectImplement.h"
#include "Serial/Unserializable.h"
#include "Renderer.h"
#include "Level.h"

#include "ECS/Components/MeshComponent.h"

#include <glm/gtc/quaternion.hpp>

void ConstructLegComponent(entt::registry& reg, entt::entity e)
{
	auto&& leg = reg.get<MonsterLegComponent>(e);
	Perry::Entity arm = Perry::GetCurrentLevel().AddEntity(glm::vec3(0), glm::vec3(0));
	Perry::Transform::SetParent(e, arm);
	// Spawn 3 joints with models
	std::vector<float> lengths = {leg.m_JointLength, leg.m_JointLength, leg.m_JointLength};
	arm.AddComponent<Perry::IKArmComponent>(lengths, 1, 1);
	for (int i = 0; i < lengths.size(); i++) {
		Perry::Entity joint = arm.FindComponent<Perry::IKArmComponent>().m_Joints[i];
		
		joint.SetName("Joint Model");
		if (i == lengths.size() - 1) {
			auto jointC = Perry::GetRegistry().try_get<Perry::JointComponent>(joint);
			jointC->m_EndPos = jointC->m_StartPos - glm::vec3(0.f, jointC->m_Length, 0.f);
			joint.LoadModel("Models/Monster/Vertebra_Parts/Vertebra_Tibiad.gltf");
		}
		else {
			joint.LoadModel("Models/Monster/Vertebra_Parts/Vertebra_Femurd.gltf");
		}
	}
	arm.AddComponent<Unserializable>();
	leg.m_IKArm = arm;
	// Wen need to rotate the leg 180 degrees if itis a rght one, so it faces proper direction
	if(leg.m_Direction == RIGHT_SIDE) Perry::Transform::SetRotation(*Perry::GetRegistry().try_get<Perry::TransformComponent>(e), glm::vec3(0.f, 180.f, 0.f));
	Perry::Transform::SetScale(*Perry::GetRegistry().try_get<Perry::TransformComponent>(e), leg.m_ModelScale);
}
void MonsterLegSystem::Init(entt::registry& reg)
{
	reg.on_construct<MonsterLegComponent>().connect<&ConstructLegComponent>();
}


void MonsterLegSystem::Update(entt::registry& reg)
{
	const auto legGroup = reg.group<MonsterLegComponent>(entt::get<Perry::TransformComponent>);

	for (auto&& [legEntity, legC, legTransform] : legGroup.each())
	{
		// We get the portion of the walking cycle, we're currently in
		// The speed multiplyer and the distance change between the frames can make a cycle faster
		legC.m_TimePassed += GetDeltaTime() * glm::distance(legC.m_LastTransform, Perry::Transform::GetPosition(legTransform)) * legC.m_Speed;
		if (legC.m_TimePassed > legC.m_WalkCycleTime) legC.m_TimePassed = 0.f;
		legC.m_WalkCycleProgress = legC.m_TimePassed / legC.m_WalkCycleTime;
		// We will move only the target position ofthe IK arm
		auto target = reg.get<Perry::IKArmComponent>(legC.m_IKArm).m_Target;
		auto transform =  Perry::GetRegistry().try_get<Perry::TransformComponent>(target);
		glm::vec3 pos = Perry::Transform::GetLocalPosition(*transform);
		// Half of the walk Cycle leg goes in sin wave up and down and forward
		if (legC.m_WalkCycleProgress < 0.5f) {
			pos.x = legC.m_HorAmplitude * legC.m_WalkCycleProgress;
			pos.y = sin(legC.m_WalkCycleProgress * 3.14f * 2.f) * legC.m_VertAmplitude;
		}
		// Another half of the cycle it goes backwards
		else {
			pos.y = 0.f;
			pos.x = legC.m_HorAmplitude * (1.f - legC.m_WalkCycleProgress);
		}
		if(legC.m_Direction == LEFT_SIDE) Perry::Transform::SetLocalPosition(*transform, glm::vec3(legC.m_LegStepOffset.x + pos.x,legC.m_LegStepOffset.y + pos.y, legC.m_LegStepOffset.z));
		else Perry::Transform::SetLocalPosition(*transform, glm::vec3(legC.m_LegStepOffset.x -pos.x,legC.m_LegStepOffset.y + pos.y, legC.m_LegStepOffset.z));
		legC.m_LastTransform = Perry::Transform::GetPosition(legTransform);
	}
}

IMPLEMENT_REFLECT_COMPONENT(MonsterLegComponent)
{
	meta.data<&MonsterLegComponent::m_WalkCycleTime>("m_WalkCycleTime"_hs)
		PROP_DRAGSPEED(0.1f)
		PROP_MINMAX(0.1f, 20.f)
		PROP_DISPLAYNAME("Walk Cycle Time");

	meta.data<&MonsterLegComponent::m_VertAmplitude>("m_VertAmplitude"_hs)
		PROP_DRAGSPEED(0.1f)
		PROP_MINMAX(0.0f, 30.f)
		PROP_DISPLAYNAME("Vertical Amplitude");

	meta.data<&MonsterLegComponent::m_HorAmplitude>("m_HorAmplitude"_hs)
		PROP_DRAGSPEED(0.1f)
		PROP_MINMAX(0.0f, 30.f)
		PROP_DISPLAYNAME("Horizontal Amplitude");

	meta.data<&MonsterLegComponent::m_LegStepOffset>("m_LegStepOffset"_hs)
		PROP_DRAGSPEED(0.1f)
		PROP_DISPLAYNAME("Leg Step Offset");
	
}
FINISH_REFLECT()

IMPLEMENT_REFLECT_SYSTEM(MonsterLegSystem)
{
}
FINISH_REFLECT()