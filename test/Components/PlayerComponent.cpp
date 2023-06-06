#include "Components/PlayerComponent.h"

#include "Components/WingsComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/VelocityComponent.h"
#include "ECS/Components/CollisionComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "components/ChainResponseComponent.h"
#include "Components/BodyPartEnemyComponent.h"
#include "Components/LegsComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "Components/MonsterComponent.h"
#include "Components/BodyPartComponent.h"
#include "ECS/Components/CollisionComponent.h"
#include "Components/HealthComponent.h"
#include "ECS/Components/SpriteComponent.h"
#include "Components/TimeKillComponent.h"

#include "Input.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Audio/AudioSystem.h"

#include "Meta/MetaReflectImplement.h"

#include "Serial/Unserializable.h"

#include <fmod_studio.hpp>

using namespace Perry;

void ConstructPlayerBody(entt::registry& reg, entt::entity e)
{
	auto& player = reg.get<PlayerComponent>(e);
	auto& transform = reg.get<TransformComponent>(e);

	Perry::AssetManager<Model>::Get("Models/Chain/Chain.gltf");
	//Perry::AssetManager<Model>::Get("Models/Candles/candles.gltf");
	////Perry::AssetManager<Model>::Get("Models/Chain/Chain.gltf");

	//for (int i = 0; i < 16; i++)
	//{
	//	auto ee = GetCurrentLevel().AddEntity(Transform::GetPosition(transform), glm::vec3(0.f), glm::vec3(1.65f));
	//	ee.LoadModel("Models/Candles/candles.gltf");

	//	player.m_Candles[i] = ee;
	//}

	auto body = GetCurrentLevel().AddEntity(Transform::GetPosition(transform) + glm::vec3(0.f, 2.f, 0.f), glm::vec3(0.f), glm::vec3(1.65f));
	body.LoadModel("Models/Player/New Wings/GLTF/body.gltf");
	body.AddComponent<Unserializable>();
	player.m_Body = body.entID;

	Transform::SetParent(e, body.entID);

	auto head = GetCurrentLevel().AddEntity(Transform::GetPosition(transform) + glm::vec3(0.f, 2.6f, 0.5f), glm::vec3(0.f), glm::vec3(1.5f));
	head.LoadModel("Models/Player/New Wings/GLTF/head.gltf");
	head.AddComponent<Unserializable>();
	player.m_Head = head.entID;

	auto headWingL = GetCurrentLevel().AddEntity(Transform::GetPosition(transform) + glm::vec3(0.35f, 2.6f, -0.4f), glm::vec3(0.f), glm::vec3(1.5f));
	headWingL.LoadModel("Models/Player/New Wings/GLTF/L_upperWing.gltf");
	headWingL.AddComponent<Unserializable>();
	Transform::SetParent(head, headWingL);

	auto headWingR = GetCurrentLevel().AddEntity(Transform::GetPosition(transform) + glm::vec3(-0.35f, 2.6f, -0.4f), glm::vec3(0.f), glm::vec3(1.5f));
	headWingR.LoadModel("Models/Player/New Wings/GLTF/R_upperWing.gltf");
	headWingR.AddComponent<Unserializable>();
	Transform::SetParent(head, headWingR);

	Transform::SetParent(e, head.entID);
}

void PlayerSystem::Init(entt::registry& reg)
{
	reg.on_construct<PlayerComponent>().connect<&ConstructPlayerBody>();
}

void PlayerSystem::Move(entt::registry& reg)
{
	auto&& [c, t] = GetSingletonComponent<Perry::CameraComponent, Perry::TransformComponent>();
	glm::vec3 fwd = Transform::GetForwardVector(t);

	for (auto&& [player_entt, player, velocity, transform] : reg.view<PlayerComponent, Perry::VelocityComponent, TransformComponent>().each())
	{
		fwd.y = Transform::GetPosition(transform).y;
		Renderer::DrawDebugLine(
			Transform::GetPosition(transform),
			Transform::GetPosition(transform) + fwd * 10.0f, glm::vec3(1.f, 0.647f, 0.f));

		glm::vec3 right = glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::vec3 input = glm::vec3(0.f);

		if (!player.m_IsAttacking) {
			glm::vec3 forwardMovement = GetInput().GetAxis("MoveForwardBack") * fwd;

			glm::vec3 rightMovement = GetInput().GetAxis("MoveLeftRight") * right;

			input = glm::normalize(forwardMovement + rightMovement);
		}
		static glm::vec3 LastInputDir;

		bool dashing = GetInput().GetAction("Dash");

		if (glm::length(input) > 0)
		{
			input = glm::normalize(input);
			LastInputDir = input;
			player.m_CombatDirection = input;
		}


		if (player.m_CurrentDashCooldown >= 0.f)
		{
			player.m_CurrentDashCooldown -= GetDeltaTime();
		}

		constexpr glm::vec3 pos = glm::vec3(-5.f);
		constexpr glm::vec3 offset = glm::vec3(0.f);
		constexpr glm::vec3 rot = glm::vec3(90.f, 180.f, 0.f);
		constexpr glm::vec3 scale = glm::vec3(2.f, 6.f, 5.f);
		constexpr float time = 18.f;

		if (dashing && player.m_CurrentDashCooldown <= 0.f)
		{
			Entity dashVFX = GetEngine().GetLevel().AddEntity();
			auto& vfx_trans = reg.get<TransformComponent>(dashVFX.entID);
			Transform::SetPosition(vfx_trans, Transform::GetPosition(transform) + Transform::GetForwardVector(transform) * pos + offset);
			Transform::SetRotation(vfx_trans, Transform::GetRotation(transform));
			Transform::AddRotation(vfx_trans, rot);
			Transform::SetScale(vfx_trans, scale);
			auto& test = dashVFX.AddComponent<SpriteComponent>("Textures/dash/rework_dash.png", 8, time, true);
			test.m_Color = glm::vec4(1.f, 0.1f, 0.f, 1.f);
			test.m_EmissiveFactor = 10.f;
			dashVFX.AddComponent<TimeKillComponent>(8.f / time);
			dashVFX.AddComponent<Unserializable>();
			dashVFX.SetName("Player Dash");
			Perry::GetAudio().CreateEvent("event:/Character/Dash")->start();
			player.m_CurrentDashCooldown = player.m_DashCooldown;

			if (glm::length(LastInputDir) > 0)
				velocity.m_Velocity += LastInputDir * player.m_DashImpulse;
		}

		if (glm::length(input) > 0.f)
			velocity.m_Velocity += input * player.m_Acceleration * GetDeltaTime();

		if (glm::length(velocity.m_Velocity) > 0 && glm::length(velocity.m_Velocity) <= player.m_MaxSpeed)
			velocity.m_Velocity = glm::normalize(velocity.m_Velocity) * glm::min(glm::length(velocity.m_Velocity), player.m_MaxSpeed);

		if (glm::length(input) > 0.f &&
			!(player.m_AttackCounter == 3 && player.m_AttackTimer >= player.m_AttackDurationBase - player.m_AttackStartupBase - player.m_AttackHitboxLifetime)

			)
		{
			glm::vec3 dir = glm::normalize(input);

			auto roll = atan2f(-dir.x, dir.z);

			auto newRot = glm::angleAxis(-roll, glm::vec3(0.f, 1.f, 0.f));

			auto rot = glm::angleAxis(0.f, glm::vec3(0.f, 0.f, 1.f)) * newRot * glm::angleAxis(glm::radians(0.f), glm::vec3(1.f, 0.f, 0.f));

			Transform::SetRotation(transform, rot);
		}

		if (m_DrawInput)
		{
			auto p = Transform::GetPosition(transform);
			Renderer::DrawDebugLine(p, p + input, glm::vec3(0.f, 0.647f, 1.f));
		}


		//// candle trail

		//player.m_CandleTimer += GetDeltaTime();
		//
		//if (player.m_CandleTimer >= player.m_CandleCooldown)
		//{
		//	player.m_ActiveCandles.push_back(Candle{ player.m_NextCandle, 0.f });

		//	float rX = (float)((rand() % 200) - 100) * 0.01f;
		//	float rZ = (float)((rand() % 200) - 100) * 0.01f;

		//	Perry::Transform::SetPosition(reg.get<Perry::TransformComponent>(player.m_Candles[player.m_NextCandle]), Transform::GetPosition(transform) + glm::vec3(rX, 0.f, rZ));

		//	player.m_NextCandle++;
		//	player.m_NextCandle %= 16;

		//	player.m_CandleTimer -= player.m_CandleCooldown;
		//}

		//for (size_t i = 0; i < player.m_ActiveCandles.size(); i++)
		//{
		//	player.m_ActiveCandles[i].lifetime += GetDeltaTime();

		//	float n = player.m_ActiveCandles[i].lifetime / 0.8f;
		//	glm::vec3 pOffset = glm::vec3(0.f, sin(glm::radians(n * 360.f)) * 0.1f, 0.f);

		//	Perry::Transform::SetPosition(reg.get<Perry::TransformComponent>(player.m_Candles[player.m_ActiveCandles[i].index]), Transform::GetPosition(reg.get<Perry::TransformComponent>(player.m_Candles[player.m_ActiveCandles[i].index])) + pOffset);

		//	if (player.m_ActiveCandles[i].lifetime >= 0.8f)
		//	{
		//		Perry::Transform::SetPosition(reg.get<Perry::TransformComponent>(player.m_Candles[player.m_ActiveCandles[i].index]), glm::vec3(0.f, -10000.f, 0.f));
		//		player.m_ActiveCandles.erase(player.m_ActiveCandles.begin() + i);
		//	}
		//}
	}
}

void PlayerSystem::Attack(entt::registry& reg)
{
	for (auto&& [entity, player, velocity, playerTransform] : reg.view<PlayerComponent, Perry::VelocityComponent, TransformComponent>().each())
	{
		switch (player.m_PlayerAttackState)
		{
		case NORMAL_ATTACK:
		{
			// set attack speed and damage bonus based on combo

			float aSpdMult = player.m_AttackSpeedBase;
			float aDmgBuff = player.m_AttackDamageBonus;

			auto* sountrackEvent = GetAudio().GetEvent("event:/Soundtrack/Soundtrack");
			if (player.m_ComboMeter == 0)
				sountrackEvent->setParameterByName("FightIntensity", 1.0f);

			if (player.m_ComboMeter >= player.m_ComboLevel1PointsRequired)
			{
				sountrackEvent->setParameterByName("FightIntensity", 2.0f);

				aSpdMult = player.m_ComboLevel1AttackSpeed;
				aDmgBuff = player.m_ComboLevel1DamageBonus;
			}

			if (player.m_ComboMeter >= player.m_ComboLevel2PointsRequired)
			{
				aSpdMult = player.m_ComboLevel2AttackSpeed;
				aDmgBuff = player.m_ComboLevel2DamageBonus;
			}

			if (player.m_ComboMeter >= player.m_ComboLevel3PointsRequired)
			{
				sountrackEvent->setParameterByName("FightIntensity", 3.0f);

				aSpdMult = player.m_ComboLevel3AttackSpeed;
				aDmgBuff = player.m_ComboLevel3DamageBonus;

				// Start Sawblade Mode
				if (GetInput().GetActionDown("SawBladeAttack"))
				{
					player.m_PlayerAttackState = PlayerAttackState::SAWBLADE_ATTACK;
					player.m_SawTimer = 0.f;
					player.m_IndividualSawTimer = 0.f;
				}
			}

			player.m_ComboRefreshTimer += GetDeltaTime();
			player.m_AttackTimer -= GetDeltaTime() * aSpdMult;

			// allow attack input after hitbox has spawned
			if (GetInput().GetActionDown("Attack") && player.m_AttackTimer <= player.m_AttackDurationBase - player.m_AttackStartupBase - player.m_AttackHitboxLifetime)
			{
				player.m_AttackTimer = player.m_AttackDurationBase;
				player.m_AttackHitboxSpawned = false;

				// increase attack counter for 3 hit combo
				player.m_AttackCounter++;
				if (player.m_AttackCounter > 3)
					player.m_AttackCounter = 1;

				// set player rotation in attack direction 
				Transform::SetLocalRotation(reg.get<TransformComponent>(player.m_Body), glm::quat(glm::vec3(0.f, (player.m_AttackCounter == 3) ? 0.f : glm::radians((((float)player.m_AttackCounter * 2.f) - 3.f) * -15.f), 0.f)));

				// play animation
				if (reg.any_of<WingsComponent>(entity))
					WingsMovementSystem::Attack(reg, entity, aSpdMult, player.m_AttackCounter);
			}

			// actually execute attack
			if (player.m_AttackTimer <= player.m_AttackDurationBase - player.m_AttackStartupBase &&
				player.m_AttackTimer >= 0.f &&
				player.m_AttackCounter != 0 &&
				!player.m_AttackHitboxSpawned)
			{
				glm::vec3 n{};
				if (glm::length(player.m_CombatDirection) > 0.f)
					n = glm::normalize(player.m_CombatDirection);

				// spawn attack hitbox
				switch (player.m_AttackCounter)
				{
				case 1:
				{
					auto hitBox = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform) + (n * player.m_AttackRange));

					auto& collision = hitBox.AddComponent<Perry::CollisionComponent>(glm::vec2(player.m_Attack1Radius, player.m_Attack1Radius));
					collision.m_TriggerBox = true;
					hitBox.AddComponent<PlayerAttackComponent>(player.m_AttackHitboxLifetime, player.m_Attack1DamageBase + aDmgBuff, entity);
					hitBox.AddComponent<Unserializable>();

					velocity.m_Velocity += -n * 10.f;
				}
				break;
				case 2:
				{
					auto hitBox = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform) + (n * player.m_AttackRange));
					hitBox.AddComponent<Unserializable>();
					auto& collision = hitBox.AddComponent<Perry::CollisionComponent>(glm::vec2(player.m_Attack2Radius, player.m_Attack2Radius));
					collision.m_TriggerBox = true;
					hitBox.AddComponent<PlayerAttackComponent>(player.m_AttackHitboxLifetime, player.m_Attack2DamageBase + aDmgBuff, entity);

					velocity.m_Velocity += -n * 10.f;
				}
				break;
				case 3:
				{
					auto hitBox = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform));
					hitBox.AddComponent<Unserializable>();
					auto& collision = hitBox.AddComponent<Perry::CollisionComponent>(glm::vec2(player.m_Attack3Radius, player.m_Attack3Radius));
					collision.m_TriggerBox = true;
					hitBox.AddComponent<PlayerAttackComponent>(player.m_AttackHitboxLifetime, player.m_Attack3DamageBase + aDmgBuff, entity);

					velocity.m_Velocity += n * 10.f;
				}
				break;
				default:
					break;
				}

				player.m_AttackHitboxSpawned = true;

				if (player.m_ComboRefreshTimer >= player.m_ComboRefreshDuration)
				{
					sountrackEvent->setParameterByName("FightIntensity", 1.0f);
					player.m_ComboMeter = 0;
				}
			}

			// reset basic combo
			if (player.m_AttackTimer <= -0.5f * aSpdMult)
			{
				player.m_AttackCounter = 0;
			}
		}
		break;
		case SAWBLADE_ATTACK:
		{
			player.m_SawTimer += GetDeltaTime();

			if (GetInput().GetAction("Attack"))
			{
				player.m_IndividualSawTimer += GetDeltaTime();

				if (player.m_IndividualSawTimer >= 0.25f)
				{
					player.m_IndividualSawTimer -= 0.25f;

					glm::vec3 n = glm::normalize(player.m_CombatDirection);

					auto hitBox = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform) + (n * player.m_AttackRange));
					hitBox.AddComponent<Unserializable>();
					auto& collision = hitBox.AddComponent<Perry::CollisionComponent>(glm::vec2(player.m_Attack2Radius, player.m_Attack2Radius));
					collision.m_TriggerBox = true;
					hitBox.AddComponent<PlayerAttackComponent>(0.f, player.m_SawBladeDPS / 4.f, entity);
				}
			}

			if (player.m_SawTimer >= player.m_SawDuration)
			{
				player.m_PlayerAttackState = PlayerAttackState::NORMAL_ATTACK;
				player.m_ComboMeter = 0;
				player.m_AttackCounter = 0;
			}
		}
		break;
		default:
			break;
		}
	}
};

void PlayerSystem::ChainAttack(entt::registry& reg)
{
	for (auto&& [entity, player, velocity, playerTransform] : reg.view<PlayerComponent, Perry::VelocityComponent, TransformComponent>().each())
	{
		// general chain behaviour
		switch (player.m_ChainAttackState)
		{
		case ChainState::HIT_BOSS:
		case ChainState::HIT_ENEMY:
		{
			if (!reg.valid(player.m_ChainEnemyHit))
			{
				player.m_ChainAttackState = RETURNING;
				player.m_IsAttacking = false;
				player.m_ChainLinksShot = player.m_ChainLinkAmount;
				player.m_ChainAttackTimer = 0.f;
				return;
			}

			glm::vec3 pos = Transform::GetPosition(reg.get<TransformComponent>(player.m_ChainEnemyHit));

			if (reg.any_of<VelocityComponent>(player.m_ChainEnemyHit))
			{
				reg.get<ChainResponseComponent>(player.m_ChainEnemyHit).m_Hooked = true;
				reg.get<VelocityComponent>(player.m_ChainEnemyHit).m_Velocity = glm::normalize(Transform::GetPosition(playerTransform) - pos) * player.m_ChainPullSpeed;

				if (glm::distance(Transform::GetPosition(playerTransform), pos) < player.m_ChainReleaseRange)
				{
					// stun enemy
					reg.get<VelocityComponent>(player.m_ChainEnemyHit).m_Velocity = glm::vec3(0.f);
					reg.get<ChainResponseComponent>(player.m_ChainEnemyHit).m_Hooked = false;
					reg.get<ChainResponseComponent>(player.m_ChainEnemyHit).m_Stunned = true;
					reg.get<ChainResponseComponent>(player.m_ChainEnemyHit).m_StunTimer = 0.f;

					player.m_ChainAttackState = INACTIVE;
					player.m_IsAttacking = false;
					player.m_ChainLinksShot = player.m_ChainLinkAmount;
					player.m_ChainAttackTimer = 0.f;

				}
			}
		}
		break;
		case ChainState::INACTIVE:
		{
			player.m_ChainAttackCooldownTimer -= GetDeltaTime();

			if (GetInput().GetAction("ChainAttack") && !player.m_IsAttacking && player.m_ChainAttackCooldownTimer <= 0.f)
			{
				// initiate chain attack
				// play sound here

				player.m_IsAttacking = true;
				player.m_ChainAttackState = ChainState::FIRED;
				player.m_ChainAttackTimer = player.m_ChainAttackDuration;
				player.m_ChainLinkTimer = 0.f;
				player.m_ChainLinksShot = 0;
				player.m_ChainAttackCooldownTimer = player.m_ChainAttackCooldown;
			}
		}
		break;
		case ChainState::RETURNING:
		{
			player.m_ChainAttackTimer -= GetDeltaTime();

			if (player.m_ChainAttackTimer < -player.m_ChainAttackReturnDuration)
			{
				player.m_IsAttacking = false;
				player.m_ChainAttackState = ChainState::INACTIVE;
				player.m_ChainEnemyHitPoint = glm::vec3(0.f);

				player.m_ChainAttackTimer = 0.f;
				player.m_ChainLinksShot = 0;
				player.m_ChainLinkTimer = 0.f;
			}
		}
		break;
		case ChainState::FIRED:
		default:
		{
			float cd = player.m_ChainAttackDuration / (float)player.m_ChainLinkAmount;

			if (player.m_ChainAttackTimer > 0.f)
			{
				// while chain attack is happening, add to chain link timer to spawn individual chain links
				player.m_ChainAttackTimer -= GetDeltaTime();
				player.m_ChainLinkTimer += GetDeltaTime();

				if (player.m_ChainLinkTimer >= cd)
				{
					// spawn chain links

					// chain direction
					glm::vec3 n = glm::normalize(player.m_CombatDirection);
					glm::vec3 r = glm::normalize(Perry::Transform::GetRightVector(playerTransform)) * 1.6f;

					// chain mesh rotation
					glm::vec3 rot;

					if (player.m_ChainLinksShot % 2 == 0)
					{
						float t = atan2(-n.z, n.x);
						rot = glm::vec3(0.f, glm::degrees(t) + 90.f, 0.f);
					}
					else
					{
						float t = atan2(-n.z, n.x);
						rot = glm::vec3(glm::degrees(t) + 90.f, 0.f, 90.f);
					}

					auto chain = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform), rot);
					chain.AddComponent<Unserializable>();

					chain.AddComponent<Perry::VelocityComponent>();
					auto& collision = chain.AddComponent<Perry::CollisionComponent>(glm::vec2(1.f, 1.f), LAYER_STATIC);
					collision.m_TriggerBox = true;

					chain.AddComponent<ChainComponent>(entity, n, player.m_ChainAttackRange / player.m_ChainAttackDuration, player.m_ChainAttackRange / player.m_ChainAttackReturnDuration);

					auto chainLeft = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform) + glm::vec3(0.f, 4.f, 0.f) + -r, rot, glm::vec3(1.f));
					chainLeft.AddComponent<Unserializable>();
					chainLeft.LoadModel("Models/Chain/Chain.gltf");
					Perry::Transform::SetParent(chain, chainLeft);

					auto chainRight = Perry::GetCurrentLevel().AddEntity(Transform::GetPosition(playerTransform) + glm::vec3(0.f, 4.f, 0.f) + r, rot, glm::vec3(1.f));
					chainRight.AddComponent<Unserializable>();
					chainRight.LoadModel("Models/Chain/Chain.gltf");

					auto children = Perry::Transform::GetChildren(chainRight);

					Perry::Transform::SetParent(chain, chainRight);
					//chain.LoadModel("Models/Chain/Chain.gltf");

					// reset chain link spawn timer
					player.m_ChainLinkTimer -= cd;
					player.m_ChainLinksShot++;
				}

				// stop chain attack once the timer runs out
				if (player.m_ChainAttackTimer <= 0.f)
				{
					player.m_ChainAttackState = ChainState::RETURNING;
				}
			}
		}
		break;
		}
	}
}

void PlayerSystem::Update(entt::registry& reg)
{
	for (auto&& [entity, player, transform] : reg.view<PlayerComponent, TransformComponent>().each())
	{
		player.m_AnimationOffset += GetDeltaTime();

		Transform::SetLocalPosition(reg.get<TransformComponent>(player.m_Body), glm::vec3(0.f, (cos(player.m_AnimationOffset) * 0.5f), 0.f) + glm::vec3(0.f, 5.f, -0.5f));
		Transform::SetLocalPosition(reg.get<TransformComponent>(player.m_Head), glm::vec3(0.f, (cos(player.m_AnimationOffset + glm::radians(15.f)) * 0.5f), 0.f) + glm::vec3(0.f, 5.6f, 0.5f));

		if (player.m_AttackTimer < 0.f)
			Transform::SetLocalRotation(reg.get<TransformComponent>(player.m_Body), glm::quat(glm::vec3((sin(player.m_AnimationOffset) * glm::radians(10.f)) + glm::radians(10.f), 0.f, 0.f)));
	}


	Move(reg);
	Attack(reg);
	ChainAttack(reg);
}

IMPLEMENT_REFLECT_COMPONENT(PlayerComponent)
{
	meta.data<&PlayerComponent::m_Acceleration>("Acceleration"_hs)
		PROP_DISPLAYNAME("Acceleration")
		PROP_MINMAX(0.05f, 0.2f)
		PROP_DRAGSPEED(0.01f);

	meta.data<&PlayerComponent::m_DashImpulse>("DashImpulse"_hs)
		PROP_DISPLAYNAME("DashImpulse")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_MaxSpeed>("m_MaxSpeed"_hs)
		PROP_DISPLAYNAME("MaxSpeed");

	meta.data<&PlayerComponent::m_DashCooldown>("m_DashCooldown"_hs)
		PROP_DISPLAYNAME("Dash Cooldown")
		PROP_DRAGSPEED(0.01f);

	meta.data<&PlayerComponent::m_ChainAttackDuration>("m_ChainAttackDuration"_hs)
		PROP_DISPLAYNAME("Chain Attack Duration")
		PROP_DRAGSPEED(0.01f);

	meta.data<&PlayerComponent::m_ChainAttackReturnDuration>("m_ChainAttackReturnDuration"_hs)
		PROP_DISPLAYNAME("Chain Attack Return Duration")
		PROP_DRAGSPEED(0.01f);

	meta.data<&PlayerComponent::m_ChainAttackRange>("m_ChainAttackRange"_hs)
		PROP_DISPLAYNAME("Chain Attack Range")
		PROP_DRAGSPEED(0.01f);

	meta.data<&PlayerComponent::m_ChainPullSpeed>("m_ChainPullSpeed"_hs)
		PROP_DISPLAYNAME("Chain Pull Speed")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_ChainReleaseRange>("m_ChainReleaseRange"_hs)
		PROP_DISPLAYNAME("Chain Enemy Release Range")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_ChainAttackCooldown>("m_ChainAttackCooldown"_hs)
		PROP_DISPLAYNAME("Chain Attack Cooldown")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_AttackDurationBase>("m_AttackDurationBase"_hs)
		PROP_DISPLAYNAME("Base Attack Duration")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_AttackStartupBase>("m_AttackStartupBase"_hs)
		PROP_DISPLAYNAME("Base Attack Startup Duration")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_AttackSpeedBase>("m_AttackSpeedBase"_hs)
		PROP_DISPLAYNAME("Base Attack Speed")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_AttackHitboxLifetime>("m_AttackHitboxLifetime"_hs)
		PROP_DISPLAYNAME("hitbox Lifetime")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack1DamageBase>("m_Attack1DamageBase"_hs)
		PROP_DISPLAYNAME("First attack base damage")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack1Radius>("m_Attack1Radius"_hs)
		PROP_DISPLAYNAME("First attack base radius")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack2DamageBase>("m_Attack2DamageBase"_hs)
		PROP_DISPLAYNAME("Second attack base damage")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack2Radius>("m_Attack2Radius"_hs)
		PROP_DISPLAYNAME("Second attack base radius")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack2DamageBase>("m_Attack2DamageBase"_hs)
		PROP_DISPLAYNAME("Second attack base damage")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack3DamageBase>("m_Attack3DamageBase"_hs)
		PROP_DISPLAYNAME("Third attack base damage")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_Attack3Radius>("m_Attack3Radius"_hs)
		PROP_DISPLAYNAME("Third attack base radius")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_AttackRange>("m_AttackRange"_hs)
		PROP_DISPLAYNAME("Attack range")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboRefreshDuration>("m_ComboRefreshDuration"_hs)
		PROP_DISPLAYNAME("Combo Refresh Duration")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboLevel1PointsRequired>("m_ComboLevel1PointsRequired"_hs)
		PROP_DISPLAYNAME("Combo level 1 requirement")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_ComboLevel2PointsRequired>("m_ComboLevel2PointsRequired"_hs)
		PROP_DISPLAYNAME("Combo level 2 requirement")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_ComboLevel3PointsRequired>("m_ComboLevel3PointsRequired"_hs)
		PROP_DISPLAYNAME("Combo level 3 requirement")
		PROP_DRAGSPEED(1.f);

	meta.data<&PlayerComponent::m_ComboLevel1AttackSpeed>("m_ComboLevel1AttackSpeed"_hs)
		PROP_DISPLAYNAME("Combo level 1 attack speed")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboLevel2AttackSpeed>("m_ComboLevel2AttackSpeed"_hs)
		PROP_DISPLAYNAME("Combo level 2 attack speed")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboLevel3AttackSpeed>("m_ComboLevel3AttackSpeed"_hs)
		PROP_DISPLAYNAME("Combo level 3 attack speed")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboLevel1DamageBonus>("m_ComboLevel1DamageBonus"_hs)
		PROP_DISPLAYNAME("Combo level 1 damage bonus")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboLevel2DamageBonus>("m_ComboLevel2DamageBonus"_hs)
		PROP_DISPLAYNAME("Combo level 2 damage bonus")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_ComboLevel3DamageBonus>("m_ComboLevel3DamageBonus"_hs)
		PROP_DISPLAYNAME("Combo level 3 damage bonus")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_SawBladeDPS>("m_SawBladeDPS"_hs)
		PROP_DISPLAYNAME("Sawblade DPS")
		PROP_DRAGSPEED(0.1f);

	meta.data<&PlayerComponent::m_SawDuration>("m_SawDuration"_hs)
		PROP_DISPLAYNAME("Sawblade mode duration")
		PROP_DRAGSPEED(0.1f);
}
FINISH_REFLECT()


IMPLEMENT_REFLECT_SYSTEM(PlayerSystem)
{
	meta.data<&PlayerSystem::m_DrawInput>("m_DrawInput"_hs)
		PROP_DISPLAYNAME("DrawInput");
}
FINISH_REFLECT()



void ChainCollided(Perry::CollisionComponent& a, Perry::CollisionComponent& b)
{
	auto& reg = Perry::GetRegistry();

	auto entityA = entt::to_entity(reg, a);
	auto entityB = entt::to_entity(reg, b);

	if (!reg.any_of<TransformComponent>(entityA)) return;
	if (!reg.any_of<TransformComponent>(entityB)) return;
	if (!reg.any_of<ChainResponseComponent>(entityB)) return;
	if (!reg.any_of<ChainComponent>(entityA)) return;

	if (reg.get<PlayerComponent>(reg.get<ChainComponent>(entityA).m_Player).m_ChainAttackState != ChainState::FIRED) return;

	auto ctTransform = reg.get<TransformComponent>(entityB);
	auto chainTarget = reg.get<ChainResponseComponent>(entityB);
	auto& player = reg.get<PlayerComponent>(reg.get<ChainComponent>(entityA).m_Player);

	switch (chainTarget.m_ChainResponse)
	{
	case ChainResponse::ENEMY:
	{
		player.m_ChainAttackState = ChainState::HIT_ENEMY;
		player.m_ChainEnemyHitPoint = Perry::Transform::GetPosition(ctTransform);
		player.m_ChainEnemyHit = entityB;

		chainTarget.m_Hooked = true;
	}
	break;
	case ChainResponse::BOSS:
	{
		if (reg.any_of<MonsterComponent>(entityB))
		{
			player.m_ChainAttackState = ChainState::RETURNING;
			return;
		}

		if (reg.try_get<BodyPartComponent>(entityB))
		{
			auto monster = reg.get<BodyPartComponent>(entityB).m_Parent;

			if (reg.get<MonsterComponent>(monster).m_MonsterPartEntities.size() > 1)
			{
				entt::entity loosePart = MonsterSystem::SpawnMonsterPartEnemy(entityB);
				MonsterSystem::RemoveMonsterPart(monster);

				player.m_ChainEnemyHitPoint = Perry::Transform::GetPosition(reg.get<TransformComponent>(loosePart));
				player.m_ChainEnemyHit = loosePart;
				player.m_ChainAttackState = ChainState::HIT_ENEMY;

				reg.get<ChainResponseComponent>(loosePart).m_Hooked = true;
			}
		}
	}
	break;
	case ChainResponse::DEFLECT:
	default:
	{
		player.m_ChainAttackState = ChainState::RETURNING;
	}

	break;
	}
}

void ChainConnected(entt::registry& reg, entt::entity e)
{
	reg.get<CollisionComponent>(e).m_OnCollideStart.connect<&ChainCollided>();
}

// ChainComponent
void ChainSystem::Init(entt::registry& reg)
{
	reg.on_construct<ChainComponent>().connect<&ChainConnected>();
}

void ChainSystem::Update(entt::registry& reg)
{
	entt::entity playerCharacter;
	glm::vec3 playerPos;
	for (auto&& [entity, player, transform] : reg.view<PlayerComponent, TransformComponent>().each())
	{
		playerPos = Perry::Transform::GetPosition(transform);
		playerCharacter = entity;
	};

	for (auto&& [entity, chain, velocity, transform] : reg.view<ChainComponent, Perry::VelocityComponent, TransformComponent>().each())
	{
		auto player = reg.try_get<PlayerComponent>(playerCharacter);

		// handle chain movement based on chain's current state
		switch (player->m_ChainAttackState)
		{
		case ChainState::FIRED:
		{
			velocity.m_Velocity = chain.m_Direction * chain.m_Speed;
		}
		break;
		case ChainState::RETURNING:
		case ChainState::HIT_ENEMY:
		case ChainState::HIT_BOSS:
		{
			chain.m_ReturnTimer += GetDeltaTime();

			glm::vec3 d = glm::normalize(playerPos - Transform::GetPosition(transform));
			float dist = std::clamp(glm::distance(playerPos, Transform::GetPosition(transform)), 0.f, 1.f);

			velocity.m_Velocity = (d * chain.m_ReturnSpeed) * dist;

			if (glm::distance(playerPos, Perry::Transform::GetPosition(transform)) < 0.2f || chain.m_ReturnTimer >= player->m_ChainAttackReturnDuration)
			{
				reg.destroy(entity);
			}
		}
		break;
		case ChainState::INACTIVE:
		{
			reg.destroy(entity);
		}
		break;
		default:
			break;
		}
	}
}

IMPLEMENT_REFLECT_COMPONENT(ChainComponent)
{
	meta.data<&ChainComponent::m_Direction>("Direction"_hs)
		PROP_DISPLAYNAME("Direction");
}
FINISH_REFLECT()

IMPLEMENT_REFLECT_SYSTEM(ChainSystem)
{

}
FINISH_REFLECT()

void OnEnemyHit(CollisionComponent& a, CollisionComponent& b)
{
	auto player = entt::to_entity(Perry::GetRegistry(), a);
	auto e = entt::to_entity(Perry::GetRegistry(), b);

	// avoid intersections with player
	if (GetRegistry().any_of<PlayerComponent>(e)) return;

	HealthSystem::Damage(e, GetRegistry().get<PlayerAttackComponent>(player).m_Damage);
	// add damage here

	// add to combo meter once per attack at max
	if (!GetRegistry().get<PlayerAttackComponent>(player).m_HasHit)
	{
		GetRegistry().get<PlayerComponent>(GetRegistry().get<PlayerAttackComponent>(player).m_Player).m_ComboMeter++;
		GetRegistry().get<PlayerComponent>(GetRegistry().get<PlayerAttackComponent>(player).m_Player).m_ComboRefreshTimer = 0.f;
		GetRegistry().get<PlayerAttackComponent>(player).m_HasHit = true;
	}
}

void OnPlayerAttackCreated(entt::registry& reg, entt::entity e)
{
	if (!reg.any_of<CollisionComponent>(e)) return;

	reg.get<CollisionComponent>(e).m_OnCollideStart.connect<&OnEnemyHit>();
}

void PlayerAttackSystem::Init(entt::registry& reg)
{
	reg.on_construct<PlayerAttackComponent>().connect<&OnPlayerAttackCreated>();
};
void PlayerAttackSystem::Update(entt::registry& reg)
{
	for (auto&& [entity, attack] : reg.view<PlayerAttackComponent>().each())
	{
		attack.m_LifeTime -= GetDeltaTime();

		if (attack.m_LifeTime <= 0.f)
		{
			reg.destroy(entity);
		}
	}
};

IMPLEMENT_REFLECT_SYSTEM(PlayerAttackSystem)
FINISH_REFLECT()