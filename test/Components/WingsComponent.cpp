#include "Components/WingsComponent.h"
#include "ECS/Components/CollisionComponent.h"


#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/SpriteComponent.h"

#include "Meta/MetaReflectImplement.h"

#include "Renderer.h"
#include "Level.h"
#include "Input.h"

#include "Components/PlayerComponent.h"
#include "ECS/Components/MeshComponent.h"

#include "Serial/Unserializable.h"

#include <glm/gtc/quaternion.hpp>

#include "Components/TimeKillComponent.h"

glm::vec3 RotateAroundPoint2D(glm::vec3 pivot, float angle, glm::vec3 point)
{
	float s = sin(angle);
	float c = cos(angle);

	// translate point back to origin:
	point.x -= pivot.x;
	point.z -= pivot.z;

	// rotate point
	float xnew = point.x * c - point.z * s;
	float ynew = point.x * s + point.z * c;

	// translate point back:
	point.x = xnew + pivot.x;
	point.z = ynew + pivot.z;
	return point;
}

void SpawnMirroredParts(entt::registry& reg, entt::entity e, int side)
{
	float inverse = ((float)side * 2.f) - 1.f;
	auto& playerTransform = reg.get<Perry::TransformComponent>(e);
	glm::vec3 playerPos = Perry::Transform::GetPosition(playerTransform);
	glm::vec3 playerRight = Perry::Transform::GetRightVector(playerTransform);
	glm::vec3 playerForward = Perry::Transform::GetForwardVector(playerTransform);
	glm::vec3 offsetWingPoint = glm::vec3(playerPos.x, playerPos.y + 2.3f, playerPos.z) + (playerRight * inverse) + (playerForward * 0.5f);

	Perry::Entity wingCenterPoint = Perry::GetCurrentLevel().AddEntity(offsetWingPoint);
	wingCenterPoint.AddComponent<Unserializable>();
	Perry::Transform::SetParent(e, wingCenterPoint.entID);
	reg.get<WingsComponent>(e).m_WingCenters.push_back(wingCenterPoint.entID);
	
	if (side == 1)
	{
		Perry::Entity wing0 = Perry::GetCurrentLevel().AddEntity(Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wingCenterPoint.entID)), glm::vec3(0.f), glm::vec3(1.5f));
		wing0.LoadModel("Models/Player/New Wings/GLTF/L_Wing1.gltf");
		wing0.AddComponent<Unserializable>();
		Perry::Transform::SetParent(wingCenterPoint.entID, wing0.entID);

		Perry::Entity wing1 = Perry::GetCurrentLevel().AddEntity(Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wingCenterPoint.entID)), glm::vec3(0.f), glm::vec3(1.5f));
		wing1.LoadModel("Models/Player/New Wings/GLTF/L_Wing2.gltf");
		wing1.AddComponent<Unserializable>();

		Perry::Entity wing2 = Perry::GetCurrentLevel().AddEntity(Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wingCenterPoint.entID)), glm::vec3(0.f), glm::vec3(1.5f));
		wing2.LoadModel("Models/Player/New Wings/GLTF/L_Wing3.gltf");
		wing2.AddComponent<Unserializable>();

		reg.get<WingsComponent>(e).m_WingBlades.push_back(wing0.entID);
		reg.get<WingsComponent>(e).m_WingMains.push_back(wing1.entID);
		Perry::Transform::SetParent(wing0.entID, wing1.entID);
		Perry::Transform::SetParent(wing1.entID, wing2.entID);
	}
	else
	{
		Perry::Entity wing0 = Perry::GetCurrentLevel().AddEntity(Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wingCenterPoint.entID)), glm::vec3(0.f), glm::vec3(1.5f));
		wing0.LoadModel("Models/Player/New Wings/GLTF/R_Wing1.gltf");
		wing0.AddComponent<Unserializable>();
		Perry::Transform::SetParent(wingCenterPoint.entID, wing0.entID);

		Perry::Entity wing1 = Perry::GetCurrentLevel().AddEntity(Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wingCenterPoint.entID)), glm::vec3(0.f), glm::vec3(1.5f));
		wing1.LoadModel("Models/Player/New Wings/GLTF/R_Wing2.gltf");
		wing1.AddComponent<Unserializable>();

		Perry::Entity wing2 = Perry::GetCurrentLevel().AddEntity(Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wingCenterPoint.entID)), glm::vec3(0.f), glm::vec3(1.5f));
		wing2.LoadModel("Models/Player/New Wings/GLTF/R_Wing3.gltf");
		wing2.AddComponent<Unserializable>();

		reg.get<WingsComponent>(e).m_WingBlades.push_back(wing0.entID);
		reg.get<WingsComponent>(e).m_WingMains.push_back(wing1.entID);
		Perry::Transform::SetParent(wing0.entID, wing1.entID);
		Perry::Transform::SetParent(wing1.entID, wing2.entID);
	}
}

void ConstructWings(entt::registry& reg, entt::entity e)
{
	auto& wingsComp = reg.get<WingsComponent>(e);

	wingsComp.m_Parent = e;

	for (int i = 0; i < 2; i++)
	{
		SpawnMirroredParts(reg, e, i);
	}
}

void WingsMovementSystem::Init(entt::registry& reg)
{
	reg.on_construct<WingsComponent>().connect<&ConstructWings>();
}

glm::vec3 WingLerp(glm::vec3 a, glm::vec3 b, float t) {
	return a * (1.0f - t) + b * t;
}

void SetWingIdle(entt::registry& reg, WingsComponent& wings, PlayerComponent& player, Perry::TransformComponent& transform, int i)
{
	glm::vec3 offsetWingPoint = glm::vec3(((i * 2.f) - 1.f) * 1.5f, 5.2f, 0.f);

	Perry::Transform::SetLocalPosition(reg.get<Perry::TransformComponent>(wings.m_WingBlades[i]), glm::vec3(0.f, 0.f, 0.f));
	Perry::Transform::SetLocalPosition(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), offsetWingPoint + glm::vec3(0.f, (sin(player.m_AnimationOffset) * 0.5f), 0.f));
	Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), glm::vec3(0.f, 0.f, 0.f));

	//Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingMains[i]), glm::quat(glm::vec3(0.f, 0.f, 0.f)));
	Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingBlades[i]), glm::vec3(0.f, 0.f, ((sin(player.m_AnimationOffset) * 10.f) + 15.f) * ((i * 2.f) - 1.f)));
	Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingMains[i]), glm::vec3(0.f, 0.f, -((sin(player.m_AnimationOffset) * 15.f)) * ((i * 2.f) - 1.f)));

	Perry::Transform::SetLocalScale(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), glm::vec3(1.5f));
}

void WingsMovementSystem::Update(entt::registry& reg)
{
	for (auto&& [entity, wings, player, transform] : reg.view<WingsComponent, PlayerComponent, Perry::TransformComponent>().each())
	{
		if (!reg.valid(wings.m_Parent)) return;

		float inverse = ((float)wings.m_WingSide * 2.f) - 1.f;

		if (player.m_PlayerAttackState == PlayerAttackState::NORMAL_ATTACK)
		{

			switch (wings.m_WingState)
			{
			case WINGS_IDLE:
			{
				for (int i = 0; i < 2; i++)
				{
					SetWingIdle(reg, wings, player, transform, i);
				}
			}
			break;
			case WINGS_ATTACKING:
			{
				SetWingIdle(reg, wings, player, transform, 1 - (int)wings.m_WingSide);
				
				wings.m_AttackNormaltimer += GetDeltaTime() * wings.m_AttackSpeed;

				auto currentCenter = wings.m_WingCenters[(int)wings.m_WingSide];
				auto currentBlade = wings.m_WingBlades[(int)wings.m_WingSide];
				auto currentWing = wings.m_WingMains[(int)wings.m_WingSide];

				float timerStartup = std::clamp(wings.m_AttackNormaltimer / player.m_AttackStartupBase, 0.f, 1.f);
				float timer = std::clamp(wings.m_AttackNormaltimer / (player.m_AttackStartupBase + player.m_AttackHitboxLifetime), 0.f, 1.f);
				float sinTimer = sin(glm::radians(timer * 90.f));
				float sin2Timer = sin(glm::radians(timer * 180.f));
				float sin3Timer = sin(glm::radians(timer * 360.f));

				glm::vec3 offsetWingPoint = glm::vec3((((int)wings.m_WingSide * 2.f) - 1.f) * 1.5f, 4.8f + ((1.f - timer) * 2.f), 0.f);

				// make wings larger
				Perry::Transform::SetLocalScale(reg.get<Perry::TransformComponent>(currentCenter), glm::vec3(1.5f) + (glm::vec3(0.4f) * timerStartup));

				// wing opening motiond
				Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(currentCenter), glm::vec3(0.f, 90.f * timerStartup * inverse, 45.f * -inverse * timerStartup));

				Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(currentWing), glm::vec3(0.f, 0.f, timerStartup * 200.f * inverse));

				// slashing motion
				Perry::Transform::SetLocalPosition(reg.get<Perry::TransformComponent>(currentCenter), offsetWingPoint + glm::vec3((wings.m_CurveX * inverse) * sin3Timer, -wings.m_CurveY * sinTimer, wings.m_CurveZ * sin2Timer));

				Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(currentBlade), glm::vec3(0.f, 0.f, 130.f * sinTimer * inverse));

				if (wings.m_AttackNormaltimer >= (player.m_AttackStartupBase + player.m_AttackHitboxLifetime))
				{
					wings.m_AttackNormaltimer = 0.f;
					wings.m_WingState = WingState::WINGS_IDLE;
				}
			}
			break;
			case WINGS_DASHING:
			{

			}
			break;
			case WINGS_BIG_ATTACK:
			{
				wings.m_AttackNormaltimer += GetDeltaTime() * wings.m_AttackSpeed;

				float timerStartup = std::clamp(wings.m_AttackNormaltimer / player.m_AttackStartupBase, 0.f, 1.f);
				float timer = std::clamp(wings.m_AttackNormaltimer / (player.m_AttackStartupBase + player.m_AttackHitboxLifetime), 0.f, 1.f);

				for (int i = 0; i < 2; i++)
				{
					float localInverse = ((i * 2.f) - 1.f);
					glm::vec3 offsetWingPoint = glm::vec3(localInverse * 4.f, 3.8f, 0.f);

					// make wings larger
					Perry::Transform::SetLocalScale(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), glm::vec3(1.5f) + (glm::vec3(0.4f) * timerStartup));

					Perry::Transform::SetLocalPosition(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), offsetWingPoint);

					// wing opening motion
					Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), glm::vec3(90.f * timerStartup * localInverse, 120.f * timerStartup, 0.f));

					Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingMains[i]), glm::vec3(0.f, 0.f, timerStartup * (200.f * localInverse)));

					// slashing motion
					Perry::Transform::SetLocalRotation(transform, glm::vec3(0.f, wings.m_OriginalRotation.y + timer * 360.f, 0.f));
				}

				if (wings.m_AttackNormaltimer >= (player.m_AttackStartupBase + player.m_AttackHitboxLifetime))
				{
					Perry::Transform::SetLocalRotation(transform, wings.m_OriginalRotation);
					wings.m_AttackNormaltimer = 0.f;
					wings.m_WingState = WingState::WINGS_IDLE;
				}
			}
			break;
			default:
				break;
			}

		}
		else
		{
			if (Perry::GetInput().GetAction("Attack"))
			{
				// sawblade

				for (int i = 0; i < 2; i++)
				{
					// make wings larger
					Perry::Transform::SetLocalScale(reg.get<Perry::TransformComponent>(wings.m_WingCenters[i]), glm::vec3(1.9f));

					Perry::Transform::SetLocalPosition(reg.get<Perry::TransformComponent>(wings.m_WingBlades[i]), glm::vec3(-((i * 2.f) - 1.f), -.75f, 3.f));
					Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingBlades[i]), glm::vec3(90.f, player.m_SawTimer * 1000.f, 0.f));

					Perry::Transform::SetLocalRotation(reg.get<Perry::TransformComponent>(wings.m_WingMains[i]), glm::vec3(0.f, 0.f, 200.f * ((i * 2.f) - 1.f)));
				}
			}
			else
			{
				// idle

				for (int i = 0; i < 2; i++)
				{
					SetWingIdle(reg, wings, player, transform, i);
				}
			}
		}

		Perry::Transform::SetPosition(reg.get<Perry::TransformComponent>(entity), Perry::Transform::GetPosition(reg.get<Perry::TransformComponent>(wings.m_Parent)));
	}
}

void WingsMovementSystem::Attack(entt::registry& reg, entt::entity e, float attackSpeed, int attack)
{
	if (!reg.any_of<WingsComponent>(e)) return;
	if (!reg.any_of<PlayerComponent>(e)) return;

	auto& wings = reg.get<WingsComponent>(e);

	//Varialbes for the VFX
	constexpr glm::vec3 pos = glm::vec3(0.f);
	constexpr glm::vec3 offset = glm::vec3(0.f, 2.5f, 0.f);
	constexpr glm::vec3 rot = glm::vec3(-90.f, 0.f, 0.f);
	constexpr glm::vec3 scale = glm::vec3(7.f);
	constexpr glm::vec4 color = glm::vec4(1.f, 0.01f, 0.0f, 0.1f);
	constexpr float emissive_factor = 15.f;
	constexpr float time = 36.f;

	if (attack == 3)
	{
		//Overwrite the Different Variables
		constexpr float time = 16.f;
		constexpr glm::vec3 offset = glm::vec3(0.f);
		constexpr glm::vec3 scale = glm::vec3(10.f);

		//Load Spin Attack VFX
		auto SpinAttackVFX = Perry::GetCurrentLevel().AddEntity();
		auto& SpinAttackTransform = reg.get<Perry::TransformComponent>(SpinAttackVFX.entID);
		auto& SpinTransform = reg.get<Perry::TransformComponent>(e);
		Perry::Transform::SetPosition(SpinAttackTransform, Perry::Transform::GetPosition(SpinTransform) + Perry::Transform::GetForwardVector(SpinTransform) * pos + offset);
		Perry::Transform::SetRotation(SpinAttackTransform, Perry::Transform::GetRotation(SpinTransform));
		Perry::Transform::AddRotation(SpinAttackTransform, rot);
		Perry::Transform::SetScale(SpinAttackTransform, scale);
		auto& spriteCmp = SpinAttackVFX.AddComponent<Perry::SpriteComponent>("Textures/attackVFX/attack_spin.png", 8, time);
		spriteCmp.m_Color = color;
		spriteCmp.m_EmissiveFactor = emissive_factor;
		SpinAttackVFX.AddComponent<Unserializable>();
		SpinAttackVFX.AddComponent<TimeKillComponent>(8 / time);

		wings.m_WingState = WingState::WINGS_BIG_ATTACK;
		wings.m_OriginalRotation = glm::eulerAngles(Perry::Transform::GetLocalRotation(reg.get<Perry::TransformComponent>(e)));

		wings.m_AttackNormaltimer = 0.f;
		wings.m_AttackSpeed = attackSpeed;

		return;
	};

	wings.m_WingSide = (WingSide)(attack - 1); // offset because 0 is not attacking --- attack being '1' should equal LEFT but left should equal 0
	wings.m_WingState = WingState::WINGS_ATTACKING;

	//Load Regular Attack VFX
	auto RegAttackVFX = Perry::GetCurrentLevel().AddEntity();
	auto& RegAttackTrans = reg.get<Perry::TransformComponent>(RegAttackVFX.entID);
	auto& transform = reg.get<Perry::TransformComponent>(e);
	Perry::Transform::SetPosition(RegAttackTrans, Perry::Transform::GetPosition(transform) + Perry::Transform::GetForwardVector(transform) * pos + offset);
	Perry::Transform::SetRotation(RegAttackTrans, Perry::Transform::GetRotation(transform));
	Perry::Transform::AddRotation(RegAttackTrans, rot);
	Perry::Transform::SetScale(RegAttackTrans, scale);
	RegAttackVFX.AddComponent<Unserializable>();
	RegAttackVFX.AddComponent<TimeKillComponent>(12 / time);

	//Use mirrored spritesheet based on attack dir
	if (wings.m_WingSide == LEFT) {
		auto& spriteCmp = RegAttackVFX.AddComponent<Perry::SpriteComponent>("Textures/attackVFX/swing_01_spritesheet.png", 12, time);
		spriteCmp.m_Color = color;
		spriteCmp.m_EmissiveFactor = emissive_factor;

	}else if(wings.m_WingSide == RIGHT)
	{
		auto& spriteCmp = RegAttackVFX.AddComponent<Perry::SpriteComponent>("Textures/attackVFX/swing_02_spritesheet.png", 12, time);
		spriteCmp.m_Color = color;
		spriteCmp.m_EmissiveFactor = emissive_factor;
	}

	wings.m_AttackNormaltimer = 0.f;
	wings.m_AttackSpeed = attackSpeed;
};

IMPLEMENT_REFLECT_COMPONENT(WingsComponent)
{
	meta.data<&WingsComponent::m_CurveX>("m_CurveX"_hs)
		PROP_DISPLAYNAME("Attack Animation X curve")
		PROP_DRAGSPEED(0.1f);

	meta.data<&WingsComponent::m_CurveY>("m_CurveY"_hs)
		PROP_DISPLAYNAME("Attack Animation Y curve")
		PROP_DRAGSPEED(0.1f);

	meta.data<&WingsComponent::m_CurveZ>("m_CurveZ"_hs)
		PROP_DISPLAYNAME("Attack Animation Z curve")
		PROP_DRAGSPEED(0.1f);
}
FINISH_REFLECT()

IMPLEMENT_REFLECT_SYSTEM(WingsMovementSystem)
{

}
FINISH_REFLECT()