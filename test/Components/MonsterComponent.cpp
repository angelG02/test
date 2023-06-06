#include "Components/MonsterComponent.h"
#include "Components/BodyPartComponent.h"
#include "Components/LegsComponent.h"
#include "ECS/Components/CollisionComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Entity.h"
#include "Level.h"
#include "Meta/MetaReflectImplement.h"
#include "Components/ChaseTargetComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Components/ChainResponseComponent.h"
#include "Components/HealthComponent.h"
#include "Components/BodyPartEnemyComponent.h"
#include "ECS/Components/VelocityComponent.h"
#include "Components/MonsterLegComponent.h"
#include "Serial/Unserializable.h"

#define floorHeight 2.f
using namespace Perry;

IMPLEMENT_REFLECT_SYSTEM(MonsterSystem)
FINISH_REFLECT()


IMPLEMENT_REFLECT_COMPONENT(MonsterComponent)

meta.data<&MonsterComponent::m_Speed>("m_Speed"_hs)
		PROP_DISPLAYNAME("Current Snake Speed")
		PROP_READONLY;

	meta.data<&MonsterComponent::m_ChasingSpeed>("m_ChasingSpeed"_hs)
		PROP_DRAGSPEED(0.005f)
		PROP_MINMAX(0.f, 1.f)
		PROP_DISPLAYNAME("Chasing Snake Speed")
		PROP_DESCRIPTION("Value set to Speed multiplier when Chasing Speed");

	meta.data<&MonsterComponent::m_ClimbingSpeed>("m_ClimbingSpeed"_hs)
		PROP_DRAGSPEED(0.005f)
		PROP_MINMAX(0.f, 1.f)
		PROP_DISPLAYNAME("Climbing Snake Speed")
		PROP_DESCRIPTION("Value set to Speed multiplier when Climbing Speed");

	meta.data<&MonsterComponent::m_LungeDetectionRange>("m_LungeDetectionRange"_hs)
		PROP_MINMAX(0.f, 10000.f)
		PROP_DRAGSPEED(0.1f)
		PROP_DISPLAYNAME("Lunge Distance")
		PROP_DESCRIPTION("Range in which the monster can 'detect' the player and start preparing a lunge towards them");

	meta.data<&MonsterComponent::m_WaitBeforeLunge>("m_WaitBeforeLunge"_hs)
		PROP_DRAGSPEED(0.1f)
		PROP_MINMAX(0.f, 10000.f)
		PROP_DISPLAYNAME("Wait Before Lunge Time ")
		PROP_DESCRIPTION("Time in Seconds the monster spends in the waiting state before lunging towards the player's last recorded position when in range");

	meta.data<&MonsterComponent::m_LungeCooldown>("m_LungeCooldown"_hs)
		PROP_DRAGSPEED(0.05f)
		PROP_MINMAX(0.f, 10000.f)
		PROP_DISPLAYNAME("Lunge Cooldown")
		PROP_DESCRIPTION("Time in Seconds after a Lunge has executed before the monster can lunge again");

	meta.data<&MonsterComponent::m_TimeChasingBeforeSwitchToClimbing>("m_TimeChasingBeforeSwitchToClimbing"_hs)
		PROP_DRAGSPEED(1.0f)
		PROP_MINMAX(0.f, 10000.f)
		PROP_DISPLAYNAME("Time Chasing Before Switch")
		PROP_DESCRIPTION("Time the boss spends chasing and luning at the player before Switching To Climbing Pillar State");

	meta.data<&MonsterComponent::m_TimeMeteorBeforeSwitchToChasing>("m_TimeMeteorBeforeSwitchToChasing"_hs)
		PROP_DRAGSPEED(1.0f)
		PROP_MINMAX(0.f, 10000.f)
		PROP_DISPLAYNAME("Time Meteor Before Chasing")
		PROP_DESCRIPTION("Time the boss spends throwing meteors at the player before Switching To Chasing Player State");

	meta.data<&MonsterComponent::m_LungePowerMultiplier>("m_LungePowerMultiplier"_hs)
		PROP_DRAGSPEED(0.05f)
		PROP_MINMAX(0.f, 10000.f)
		PROP_DISPLAYNAME("Lunge Power")
		PROP_DESCRIPTION("Multiplier value of how far and quickly the monster lunges when executing the lunge");

	meta.data<&MonsterComponent::m_SeekingMovementDamp>("m_SeekingMovementDamp"_hs)
		PROP_DRAGSPEED(0.05f)
		PROP_MINMAX(0.f, 5.f)
		PROP_DISPLAYNAME("Seeking Accuracy")
		PROP_DESCRIPTION("How accurately the snake's Direction vector changes to go towards the player");

	meta.data<&MonsterComponent::m_StoppingDamp>("m_StoppingDamp"_hs)
		PROP_DRAGSPEED(0.05f)
		PROP_MINMAX(0.f, 100.f)
		PROP_DISPLAYNAME("Stopping Speed Before Lunge")
		PROP_DESCRIPTION("How quickly the Directional vector approaches 0 when the enemy has to stop before executing lunge");

	meta.data<&MonsterComponent::m_ClimbingUpDamp>("m_ClimbingDamp"_hs)
		PROP_DRAGSPEED(0.05f)
		PROP_MINMAX(0.f, 3.5f)
		PROP_DISPLAYNAME("Climbing Speed Up Pillars")
		PROP_DESCRIPTION("How quickly the Directional vector approaches the target value when the enemy is climbing");

FINISH_REFLECT()

MonsterComponent::MonsterComponent(entt::entity m_ChasingEntity, int numParts) :
	m_PlayerEntity(m_ChasingEntity), m_AmountOfBodyParts(numParts)
{}

int32_t MonsterComponent::GetTotalHealth() {

	int32_t sum = 0;
	for (size_t i = 1; i < m_MonsterPartEntities.size(); i++)
	{
		sum += static_cast<int32_t>(GetRegistry().get<HealthComponent>(m_MonsterPartEntities[i]).m_HealthCurrent);
	}

	return sum;
}

void OnMonsterCreated(entt::registry& reg, entt::entity e)
{
	auto&& [monster, transform, health] = reg.get<MonsterComponent, TransformComponent, HealthComponent>(e);
	monster.m_MonsterPartEntities.push_back(e);

	for (int i = 1; i < monster.m_AmountOfBodyParts; ++i)
	{
		MonsterSystem::AddMonsterPart(e, monster.m_BaseMonsterPartHealth, monster.m_BaseMonsterPartHealth, monster.m_BaseMonsterInvSeconds);
	}
}

void MonsterSystem::Init(entt::registry& reg)
{
	reg.on_construct<MonsterComponent>().connect<&OnMonsterCreated>();
}

void MonsterSystem::AddMonsterPart(entt::entity monster, float currentHealth, float maxHealth, float iFrameDuration)
{
	auto& monsterComponent = GetRegistry().get<MonsterComponent>(monster);
	size_t i = monsterComponent.m_MonsterPartEntities.size();

	Perry::Entity bodyPart = GetCurrentLevel().AddEntity();
	bodyPart.SetName("MonsterBody");
	bodyPart.LoadModel("Models/Monster/Vertebra_Parts/Vertebra_Main.gltf");
	bodyPart.AddComponent<BodyPartComponent>(monster, 1.2f, (int)i);
	auto &c = bodyPart.AddComponent<Perry::CollisionComponent>(glm::vec2(5.f, 5.f), LAYER_2);
	c.m_TriggerBox = true;
	bodyPart.AddComponent<ChainResponseComponent>(ChainResponse::BOSS);
    bodyPart.AddComponent<Unserializable>();
	auto& hp = bodyPart.AddComponent<HealthComponent>(maxHealth, iFrameDuration);
	hp.m_HealthCurrent = currentHealth;
	hp.m_Created = true;
	Perry::Entity bodyLegs = GetCurrentLevel().AddEntity();
	bodyLegs.AddComponent<LegsComponent>(0.75f);
    bodyLegs.AddComponent<Unserializable>();
	bodyLegs.SetName("Body Legs");
	Perry::Transform::SetParent(bodyPart, bodyLegs);
	Perry::Transform::SetLocalPosition(bodyPart.FindComponent<Perry::TransformComponent>(), glm::vec3(2.f * i, 0.f, -2.f));
	Perry::Transform::SetLocalScale(bodyPart.FindComponent<Perry::TransformComponent>(), glm::vec3(2.5f));
	Perry::Transform::SetLocalRotation(bodyLegs.FindComponent<Perry::TransformComponent>(), glm::vec3(-90.f, 0.f, -90.f));
	Perry::Transform::SetLocalPosition(bodyLegs.FindComponent<Perry::TransformComponent>(), glm::vec3(0.f, 0.f, -0.25f));


	monsterComponent.m_MonsterPartEntities.push_back(bodyPart.entID);
};

void MonsterSystem::RemoveMonsterPart(entt::entity monster)
{
	const auto& part = GetRegistry().get<MonsterComponent>(monster).m_MonsterPartEntities.back();
	GetRegistry().get<MonsterComponent>(monster).m_MonsterPartEntities.pop_back();
	GetRegistry().destroy(part);
}

entt::entity MonsterSystem::SpawnMonsterPartEnemy(entt::entity bodyPart)
{
	auto& test = GetRegistry().get<HealthComponent>(bodyPart);

	Perry::Entity loosePart = GetCurrentLevel().AddEntity();
	loosePart.SetName("LoosePart");
	loosePart.LoadModel("Models/Monster/Vertebra_Parts/Vertebra_Main.gltf");
	auto& c = loosePart.AddComponent<Perry::CollisionComponent>(glm::vec2(1.7f, 1.7f), LAYER_2);
	c.m_TriggerBox = true;
	loosePart.AddComponent<ChainResponseComponent>(ChainResponse::ENEMY);
	loosePart.AddComponent<VelocityComponent>();
	auto& hp = loosePart.AddComponent<HealthComponent>();
	loosePart.AddComponent<BodyPartEnemyComponent>(GetRegistry().get<BodyPartComponent>(bodyPart).m_Parent);

	hp.m_Created			= true;
	hp.m_HealthCurrent		= test.m_HealthCurrent;
	hp.m_HealthMax			= test.m_HealthMax	;
	hp.m_InvSecondsDuration	= test.m_InvSecondsDuration;
	
	Perry::Entity bodyLegs = GetCurrentLevel().AddEntity();
	bodyLegs.AddComponent<LegsComponent>(0.75f);
    bodyLegs.AddComponent<Unserializable>();
	bodyLegs.SetName("Body Legs");
	Perry::Transform::SetParent(loosePart, bodyLegs);
	Perry::Transform::SetLocalPosition(loosePart.FindComponent<Perry::TransformComponent>(), Transform::GetPosition(GetRegistry().get<TransformComponent>(bodyPart)));
	Perry::Transform::SetLocalScale(loosePart.FindComponent<Perry::TransformComponent>(), glm::vec3(2.5f));
	Perry::Transform::SetLocalRotation(bodyLegs.FindComponent<Perry::TransformComponent>(), glm::vec3(-90.f, 0.f, -90.f));
	Perry::Transform::SetLocalPosition(bodyLegs.FindComponent<Perry::TransformComponent>(), glm::vec3(0.f, 0.f, -0.25f));

	Perry::Transform::SetLocalRotation(GetRegistry().get<TransformComponent>(loosePart.entID), glm::quat(0.5f, 0.5f, 0.f, 0.f));

	return loosePart.entID;
};

glm::vec3 smooth_damp(glm::vec3 current, glm::vec3 target, float damp, float delta_time)
{
	glm::vec3 delta = target - current;
	glm::vec3 smooth_damp = delta * std::exp(-damp * delta_time);
	glm::vec3 result = target - smooth_damp;
	return result;
}
float distanceFromSegmentToPoint(const glm::vec3& a, const glm::vec3& b, const glm::vec3& p) {
	glm::vec3 ab = b - a;
	glm::vec3 ap = p - a;
	glm::vec3 bp = p - b;

	// Calculate the projection of ap onto ab
	const float abLength = glm::length(ab);
	const glm::vec3 abUnit = ab / abLength;
	const float projection = glm::dot(ap, abUnit);

	// Check if the projection is outside the segment
	if (projection <= 0.0f) {
		return glm::length(ap);
	}
	else if (projection >= abLength) {
		return glm::length(bp);
	}
	else {
		// Calculate the distance to the segment
		const glm::vec3 closestPoint = a + abUnit * projection;
		return glm::length(p - closestPoint);
	}
}

void MonsterSystem::UpdateTarget(entt::entity monsterEntity, entt::registry& reg) {

	auto& monsterComp = reg.get<MonsterComponent>(monsterEntity);
	if (glm::length(Transform::GetPosition(reg.get<TransformComponent>(monsterEntity)) - Transform::GetPosition(reg.get<TransformComponent>(monsterComp.m_ChaseTargets[0]))) < 5.0f) {
		monsterComp.m_DirTargets = Transform::GetPosition(reg.get<TransformComponent>(monsterComp.m_ChaseTargets[0]));

		const auto target = reg.get<ChaseTargetComponent>(monsterComp.m_ChaseTargets[0]);

		// Start Climbing Position - Set Speed to FAST
		if (target.m_StateChange == 1)
			monsterComp.SetClimbingMovementSpeed();

		//End Climbing Position
		if (target.m_StateChange == 2)
			monsterComp.SwitchToMeteorFiring();

		monsterComp.m_ChaseTargets.erase(monsterComp.m_ChaseTargets.begin());
		RefreshChaseTargets(monsterComp, reg);
		monsterComp.m_DirTargets -= Transform::GetPosition(reg.get<TransformComponent>(monsterComp.m_ChaseTargets[0])) ;
	}
}

void MonsterSystem::Update(entt::registry& reg)
{
	const auto monsterGroup = reg.group<MonsterComponent, HealthComponent>(entt::get<Perry::TransformComponent>);

	for (auto&& [monsterEntity, monster, health, monsterTransform] : monsterGroup.each())
	{

		float currentHealth = 0.f;
		float maxHealth	  = 0.f;

		for(int i = 1 ; i < monster.m_MonsterPartEntities.size(); i++)
		{
			const auto& health_cmp = reg.get<HealthComponent>(monster.m_MonsterPartEntities[i]);
			currentHealth += health_cmp.m_HealthCurrent;
			maxHealth += health_cmp.m_HealthMax;
		}

		//Update Health
		health.m_HealthCurrent = currentHealth;
		health.m_HealthMax = maxHealth;

		RefreshChaseTargets(monster, reg);
		UpdateTarget(monsterEntity, reg);

		glm::vec3 chaseObjPos = glm::vec3(0.f);
		switch(monster.m_State)
		{
		case MonsterComponent::MonsterState::PillarClimbing:
			{	//Needs to be encapsulated for PS5 C++ Compiler 
				glm::vec3 chaseTargetPos = Transform::GetPosition(reg.get<TransformComponent>(monster.m_ChaseTargets[0]));
				chaseObjPos = chaseTargetPos;
			}
			break;

		case MonsterComponent::MonsterState::MeteorShooting: 
			break;

		case MonsterComponent::MonsterState::ChasingPlayer:
			{//Needs to be encapsulated for PS5 C++ Compiler 
				glm::vec3 playerTargetPos = Transform::GetPosition(reg.get<TransformComponent>(monster.m_PlayerEntity));
				chaseObjPos = playerTargetPos;
			}
			break;
		}
		 
		glm::vec3 dir = chaseObjPos - Transform::GetPosition(monsterTransform);

		glm::quat additionalRotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::quat newRotation = glm::rotation(glm::vec3(0.0f, 0.0f, -1.0f), glm::normalize(glm::vec3(dir.x, 0.0, dir.z))) * additionalRotation;
		additionalRotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, -1.0f, 0.0f));

		//Lazy way to make sure it doesn't rotate when not climbing up the pillars
		if (monster.m_DirTargets.y < 0.0 && monster.m_Speed == monster.m_ClimbingSpeed) newRotation = newRotation * additionalRotation;

		Transform::SetRotation(monsterTransform, newRotation);

		monster.m_Waiting += GetDeltaTime();

		// Timer for Monster Shooting Meteor Phase
		if(monster.m_State == MonsterComponent::MonsterState::MeteorShooting && monster.m_Waiting > monster.m_TimeMeteorBeforeSwitchToChasing)
		{
			monster.SwitchStateToChasing();
		}

		//Lunge Logic
		if (monster.m_State == MonsterComponent::MonsterState::ChasingPlayer) {

			monster.m_ChaseTimerTracker += GetDeltaTime();

			//After X seconds set back to pillar climbing state
			if (monster.m_ChaseTimerTracker >= monster.m_TimeChasingBeforeSwitchToClimbing)
			{
				monster.SwitchStateToClimbing();
			}

			if (monster.m_Waiting >= 0.f && monster.m_LungeState != MonsterComponent::ChasingState::Waiting)
			{
				const float distance = glm::distance(chaseObjPos, Transform::GetPosition(monsterTransform));
				if (distance < monster.m_LungeDetectionRange) {
					monster.m_LungeState = MonsterComponent::ChasingState::RecordPLayerPosForLunge;
					// printf("Waiting %f until %f \n", monster.m_Waiting, monster.m_WaitBeforeLunge);
				}
				else
				{
					monster.m_LungeState = MonsterComponent::ChasingState::Chasing;
					// printf("NOT on Cooldown || Regular Seeking \n", monster.m_Waiting);
					monster.m_Waiting = 0.f;
					monster.m_Direction = smooth_damp(monster.m_Direction, dir, monster.m_SeekingMovementDamp, GetDeltaTime());
				}
			}

			if (monster.m_LungeState == MonsterComponent::ChasingState::OnCooldown)
			{
				// printf("On Cooldown %f || Regular Seeking \n", monster.m_Waiting);
				monster.m_Direction = smooth_damp(monster.m_Direction, dir, monster.m_SeekingMovementDamp, GetDeltaTime());
			}

			if (monster.m_LungeState == MonsterComponent::ChasingState::RecordPLayerPosForLunge) {
				// printf("Recorded PLayer Position \n");
				monster.m_LungePlayerPos = chaseObjPos;
				monster.m_LungeState = MonsterComponent::ChasingState::Waiting;
			}

			if (monster.m_LungeState == MonsterComponent::ChasingState::Waiting) {
				//This works better than putting the target to vec3(0.f)
				monster.m_Direction = smooth_damp(monster.m_Direction, -monster.m_Direction, monster.m_StoppingDamp, GetDeltaTime());

				//Go back
				if (monster.m_Waiting > monster.m_WaitBeforeLunge - monster.m_WaitBeforeLunge / 4.f)
				{
					Perry::Transform::AddTranslation(monsterTransform, -dir * 0.5f * GetDeltaTime());
				}
			}

			if (monster.m_Waiting > monster.m_WaitBeforeLunge)
			{
				// printf("---------- LUNDGING ---------- \n");
				const glm::vec3 lungeDir = monster.m_LungePlayerPos - Transform::GetPosition(monsterTransform);
				glm::normalize(lungeDir);
				monster.m_Direction = lungeDir * monster.m_LungePowerMultiplier;
				monster.m_Waiting = -monster.m_LungeCooldown;
				monster.m_LungeState = MonsterComponent::ChasingState::OnCooldown;
			}

		}
		else if (monster.m_State == MonsterComponent::MonsterState::PillarClimbing)
		{
			monster.m_Direction = smooth_damp(monster.m_Direction, dir, monster.m_Damp, GetDeltaTime());
		}
		else if(monster.m_State == MonsterComponent::MonsterState::MeteorShooting)
		{
			//Stops the monster when it needs to start shooing meteors from pillars
			monster.m_Direction = smooth_damp(monster.m_Direction, -monster.m_Direction, monster.m_StoppingDamp * 1.5f, GetDeltaTime());
		}

		Perry::Transform::AddTranslation(monsterTransform, monster.m_Direction * monster.m_Speed * GetDeltaTime());
		Move(monster);

		//Clamp to not go below ground
		glm::vec3 pos = Transform::GetPosition(monsterTransform);
		Transform::SetPosition(monsterTransform, glm::vec3(pos.x, glm::max(pos.y, floorHeight), pos.z));
	}
}

void MonsterSystem::RefreshChaseTargets(MonsterComponent& monster, entt::registry& reg) {
	if (monster.m_ChaseTargets.size() == 0) {
		const auto targetGroup = reg.group<ChaseTargetComponent, TransformComponent>();
		for (auto&& [targetEntity, target, targetTransform] : targetGroup.each())
		{
			monster.m_ChaseTargets.emplace(monster.m_ChaseTargets.begin(), targetEntity);
		}
	}
}

void MonsterSystem::Move(MonsterComponent& monster)
{
	auto& reg = GetCurrentLevel().GetRegistry();

	for (int i = 1; i < monster.m_MonsterPartEntities.size(); i++)
	{
		auto p0 = monster.m_MonsterPartEntities[i - 1];
		auto p1 = monster.m_MonsterPartEntities[i];
		auto dist = reg.get<BodyPartComponent>(p1).m_Size + reg.get<BodyPartComponent>(p0).m_Size;

		// Keep distance (harsh)
		auto dir = Perry::Transform::GetPosition(reg.get<TransformComponent>(p0)) - Perry::Transform::GetPosition(reg.get<TransformComponent>(p1));

		glm::quat additionalRotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::quat newRotation = glm::rotation(glm::vec3(0.0f, 0.0f, -1.0f), glm::normalize(glm::vec3(dir.x, 0.0, dir.z))) * additionalRotation;
		additionalRotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, -1.0f, 0.0f));

		//Lazy way to make sure it doesn't rotate when not climbing up the pillars
		if (dir.y >= 0.0 && monster.m_Speed == monster.m_ClimbingSpeed) newRotation = newRotation * additionalRotation;

		Transform::SetRotation(reg.get<TransformComponent>(p1), newRotation);
		
		if (abs(glm::length(dir) - dist) > 0.1f) {
			auto pos = Perry::Transform::GetPosition(reg.get<TransformComponent>(p0)) - glm::normalize(dir) * dist;
			Perry::Transform::SetPosition(reg.get<TransformComponent>(p1), smooth_damp(Perry::Transform::GetPosition(reg.get<TransformComponent>(p1)), pos, 100.f, GetDeltaTime()));
		}
	}

	// Resolve tail overlap
	for (int i = 1; i < monster.m_MonsterPartEntities.size() - 1; i++) 
	{
	auto curr = monster.m_MonsterPartEntities[i];
	auto next = monster.m_MonsterPartEntities[i + 1];
		for (int j = 0; j < i; j++)
		{
			auto prev = monster.m_MonsterPartEntities[j];

			float dist = distanceFromSegmentToPoint(Transform::GetPosition(reg.get<TransformComponent>(curr)) , Transform::GetPosition(reg.get<TransformComponent>(next)), Transform::GetPosition(reg.get<TransformComponent>(prev)));
			if (dist < reg.get<BodyPartComponent>(prev).m_Size)
			{
				float offset = reg.get<BodyPartComponent>(prev).m_Size - dist;
				glm::vec3 middle = (Transform::GetPosition(reg.get<TransformComponent>(curr)) + Transform::GetPosition(reg.get<TransformComponent>(next))) * 0.5f;
				glm::vec3 normal = middle - Transform::GetPosition(reg.get<TransformComponent>(prev));
				normal = glm::normalize(normal);

				auto newCurrPosition = Transform::GetPosition(reg.get<TransformComponent>(curr)) + normal * offset;
				auto newNextPosition = Transform::GetPosition(reg.get<TransformComponent>(next)) + normal * offset;

				Transform::SetPosition(reg.get<TransformComponent>(curr), newCurrPosition);
				Transform::SetPosition(reg.get<TransformComponent>(next), newNextPosition);
			}
		}
	}
}