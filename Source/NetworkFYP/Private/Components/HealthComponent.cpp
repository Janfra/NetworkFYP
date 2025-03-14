// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	/* Initialise health */
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	OldHealth = CurrentHealth;

	/* Set to replicate by default */
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}

void UHealthComponent::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

//////////////////////////////////////////////////////////////////////////
// Methods

/* Server only */
void UHealthComponent::SetCurrentHealth(float HealthValue)
{
	/* Only allow servers to update health */ 
	if (GetOwnerRole() != ENetRole::ROLE_Authority) 
	{
		return;
	}

	CurrentHealth = FMath::Clamp(HealthValue, 0.0f, MaxHealth);

	/* Ensures that both server and client have parallel calls to this function. Necessary since server will not receive the RepNotify */
	OnHealthUpdate();
}

const float UHealthComponent::ApplyDamage(float DamageValue, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float damageApplied = CurrentHealth - DamageValue;
	SetCurrentHealth(damageApplied);
	LastDamageTaken = FCachedDamageTaken(DamageValue, DamageEvent, EventInstigator, DamageCauser);
	return damageApplied;
}

/* Shared */
void UHealthComponent::OnHealthUpdate()
{
	if (CurrentHealth < OldHealth) 
	{
		if (CurrentHealth <= 0.0f)
		{
			Died();
		}
		else 
		{
			OnDynamicDamageTaken.Broadcast();
			OnDamageTaken.Broadcast();
		}
	}
	else if (CurrentHealth > OldHealth) 
	{
		OnDynamicHealed.Broadcast();
		OnHealed.Broadcast();
	}

	OldHealth = CurrentHealth;
	/* General logic, must happen regardless */
}

void UHealthComponent::Died()
{
	OnDeath.Broadcast();
	OnDynamicDeath.Broadcast();
}
	