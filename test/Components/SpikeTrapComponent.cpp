#include "Components/SpikeTrapComponent.h"
#include "Collision/CollisionWorld.h"
#include "Transform.h"
#include "ECS/Entity.h"
#include "Collision/CollisionWorld.h"
#include "Audio/AudioSystem.h"
#include "ECS/Components/MeshComponent.h"
#include "Components/HealthComponent.h"
#include "Components/PlayerComponent.h"

#include "Level.h"

#include "Meta/MetaReflectImplement.h"

// Static variables
float SpikeTrapSystem::m_SpikeDamage = 100.0f;
float SpikeTrapSystem::m_PopOutSpeed = 15.0f;
float SpikeTrapSystem::m_ReturnSpeed = 1.0f;
float SpikeTrapSystem::m_SpikeActiveHeight = 2.0f;

IMPLEMENT_REFLECT_SYSTEM(SpikeTrapSystem)
{
	meta.func<&entt::registry::emplace<SpikeTrapComponent>>(f_AddComponent);
	meta.data<&SpikeTrapSystem::m_SpikeDamage>("M_SpikeDamage"_hs)
		PROP_DISPLAYNAME("Damage");
	meta.data<&SpikeTrapSystem::m_PopOutSpeed>("M_PopOutSpeed"_hs)
		PROP_DISPLAYNAME("Pop out speed");
	meta.data<&SpikeTrapSystem::m_ReturnSpeed>("M_ReturnSpeed"_hs)
		PROP_DISPLAYNAME("Return speed");
	meta.data<&SpikeTrapSystem::m_SpikeActiveHeight>("M_ActiveHeight"_hs)
		PROP_DISPLAYNAME("Active height");
}
FINISH_REFLECT()

static glm::vec3 MyLerp(glm::vec3 a, glm::vec3 b, float t) {
	return a * (1.0f - t) + b * t;
}

void SpikeTrapComponent::Activate() {
	if (this->m_State == SpikeTrapState::Idle) {
		this->m_State = SpikeTrapState::Active;

		//Perry::GetEngine().GetAudio().PlaySound("SpikeSFX.ogg");
		
		// @Note: Quick hack because there is no easy way to get the player entity.
		// So we just get every player component.
		const auto &p = Perry::GetRegistry().view<PlayerComponent>();
		for (auto &&[player_entity, p] : p.each()) {
			HealthSystem::Damage(player_entity, SpikeTrapSystem::m_SpikeDamage);
		}
	}
}

SpikeTrapComponent::SpikeTrapComponent(Perry::Entity entity) {
	auto& col = entity.FindComponent<Perry::CollisionComponent>();

	col.m_OnCollide.connect<&SpikeTrapComponent::Activate>(this);
}

void SpikeTrapSystem::Init(entt::registry &reg) { }

void SpikeTrapSystem::Update(entt::registry & reg)
{
	const auto &spike = reg.view<SpikeTrapComponent, Perry::CollisionComponent, Perry::TransformComponent>();

	for (auto &&[id, spike, collider, transform] : spike.each()) {

		glm::vec3 ActivePosition = spike.m_SpikeRestPosition + glm::vec3(0.0f, SpikeTrapSystem::m_SpikeActiveHeight, 0.0f);

		switch (spike.m_State) {

		case SpikeTrapState::Idle: { 
			spike.m_SpikeRestPosition = Perry::Transform::GetPosition(transform);
		} break;

		case SpikeTrapState::Active: {

			spike.m_ActivateT += GetDeltaTime() * SpikeTrapSystem::m_PopOutSpeed;
			
			Perry::Transform::SetPosition(
				transform,
				MyLerp(spike.m_SpikeRestPosition, ActivePosition, spike.m_ActivateT));

			if (spike.m_ActivateT >= 1.0f) {
				spike.m_State = SpikeTrapState::Returning;
				spike.m_ActivateT = 0.0f;
			}

		} break;

		case SpikeTrapState::Returning: {
			spike.m_ActivateT += GetDeltaTime() * SpikeTrapSystem::m_ReturnSpeed;

			Perry::Transform::SetPosition(
				transform,
				MyLerp(ActivePosition, spike.m_SpikeRestPosition, spike.m_ActivateT));

			if (spike.m_ActivateT >= 1.0f) {
				spike.m_State = SpikeTrapState::Idle;
				spike.m_ActivateT = 0.0f;
			}

		} break;

		default: {
			assert(false && "Invalid spike trap state!");
		} break;

		}
	}
}

Perry::Entity SpikeTrapSystem::Create(glm::vec3 position) {
	auto &level = Perry::GetCurrentLevel();

	// @Note: set scale to 0.01 because the cube model is giant,
	// Should get fixed when we use the spike trap model. - Jesper 08-05-2023.
	Perry::Entity spike_trap = level.AddEntity(position + glm::vec3(-1.0f, 0.0f, -1.0f), { 0, 90.0f, 90.0f }, { .1f, .1f, .1f });
	spike_trap.SetName("SpikeTrap");
	{
		spike_trap.AddComponent<Perry::CollisionComponent>(glm::vec2(1.0f, 1.0f), LAYER_STATIC);
		spike_trap.AddComponent<SpikeTrapComponent>(spike_trap);
		spike_trap.LoadModel("Models/ConeSpike/scene.gltf");
	}

	{
		Perry::Entity spike_trap_base = level.AddEntity();
		spike_trap_base.SetName("SpikeTrapBase");
		auto& transform = spike_trap_base.FindComponent<Perry::TransformComponent>();

		Perry::Transform::SetScale(transform, { 0.03f, 0.001f, 0.03f });
		Perry::Transform::SetRotation(transform, { 0, 45.0f, 0.0f });
		Perry::Transform::SetPosition(spike_trap_base.FindComponent<Perry::TransformComponent>(), position);

		spike_trap_base.LoadModel("Models/1Meter Cube/1m cube.gltf");
	}

	return spike_trap;
}