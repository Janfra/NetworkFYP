// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DamageEvents.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FHealthUpdated)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDynamicHealthUpdated);

USTRUCT()
struct FCachedDamageTaken 
{
	GENERATED_BODY()
	FCachedDamageTaken()  {}
	FCachedDamageTaken(float DamageValue, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
		: DamageValue(DamageValue)
		, DamageEventID(DamageEvent.GetTypeID())
		, EventInstigator(EventInstigator)
		, DamageCauser(DamageCauser)
	{
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID)) 
		{
			const FPointDamageEvent* pointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
			// Should already be normalised, but just in case
			PushDirection = pointDamage->ShotDirection.GetSafeNormal();
		}
	}

	float DamageValue; 
	int32 DamageEventID;
	FVector PushDirection;
	TSoftObjectPtr<AController> EventInstigator; 
	TSoftObjectPtr<AActor> DamageCauser;

	const bool IsDamageEventOfType(int32 InId) 
	{
		return InId == DamageEventID;
	}
};

UCLASS( ClassGroup=(FYP), meta=(BlueprintSpawnableComponent) )
class NETWORKFYP_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	// Sets default values for this component's properties
	UHealthComponent();

/// <summary>
/// Properties Section
/// </summary>
public:

	/* Replicating through a component instead of an actor does has a slight overhead. Each component adds 4 bytes for NetGUID `header` and aprox 1 byte `footer` */
	/* Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/// <summary>
	/// Attempts to retrieve the Health Component from the given actor
	/// </summary>
	/// <param name="Actor">Actor to search</param>
	/// <returns>Returns either found Health Component or nullptr</returns>
	UFUNCTION(BlueprintPure, Category = "Health")
	static UHealthComponent* FindHealthComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr); }

	/// <summary>
	/// Maximum Health getter
	/// </summary>
	/// <returns>Maximum Health</returns>
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/// <summary>
	/// Current Health getter
	/// </summary>
	/// <returns>Current Health</returns>
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION()
	FORCEINLINE FCachedDamageTaken GetLastDamageTakenData() const { return LastDamageTaken; }

	/// <summary>
	/// Returns current health as a 0 to 1 range. 0: No health, 1: full health
	/// </summary>
	/// <returns>Returns current health as a 0 to 1 range</returns>
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthNormalised() const { return CurrentHealth / MaxHealth; }

	/// <summary>
	/// Current Health Setter. Should only be called on server
	/// </summary>
	/// <param name="HealthValue">Value to override Current Health to</param>
	UFUNCTION(Category = "Health")
	void SetCurrentHealth(float HealthValue);

	UFUNCTION(Category = "Health")
	const float ApplyDamage(float DamageValue, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UPROPERTY(BlueprintAssignable)
	FDynamicHealthUpdated OnDynamicDamageTaken;
	UPROPERTY(BlueprintAssignable)
	FDynamicHealthUpdated OnDynamicHealed;
	UPROPERTY(BlueprintAssignable)
	FDynamicHealthUpdated OnDynamicDeath;

	FHealthUpdated OnDamageTaken;
	FHealthUpdated OnHealed;
	FHealthUpdated OnDeath;

protected:

	/* Sets the max health, this value does not change. Not replicated. */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;

	/* Sets the current health, this value is replicated. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;

	/* Health before update */
	float OldHealth;

	/* Storing information about damage taken to inform during events */
	FCachedDamageTaken LastDamageTaken;

	/* RepNotify for changes made to current health. */
	UFUNCTION()
	void OnRep_CurrentHealth();

/// <summary>
/// Methods Section
/// </summary>
protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	/* Response to health update. Called on server immediately after modification, and on client in response to a RepNotify */
	void OnHealthUpdate();

	/// <summary>
	/// Health has been depleted
	/// </summary>
	void Died();
};
