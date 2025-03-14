// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/HealthComponent.h"
#include "NetworkFYPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ANetworkFYPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANetworkFYPCharacter();

/// <summary>
/// Properties Section
/// </summary>
/// 	
public:

	/* Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	
#pragma region ProjectileFiringComponent TODO

	/// <summary>
	/// Projectile to spawn
	/// </summary>
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Projectile")
	TSubclassOf<class ANetworkProjectile> ProjectileClass;

	/// <summary>
	/// How often are projectiles fired
	/// </summary>
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float FireRate;

	/// <summary>
	/// Is it currently firing
	/// </summary>
	bool bIsFiringWeapon;

	/// <summary>
	/// Handle to cooldown between firing
	/// </summary>
	FTimerHandle FiringTimer;

	/// <summary>
	/// Initiates firing
	/// </summary>
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StartFire();

	/// <summary>
	/// Stop firing
	/// </summary>
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopFire();

	/* Implemented with _Implementation version due to Server specifier. Reliable specifier ensures that the server will receive this, but it goes into a queue that can overflow, restrict usage. */
	/// <summary>
	/// Spawn projectile and fire projectile
	/// </summary>
	UFUNCTION(Server, Reliable)
	void HandleFire();

#pragma endregion


private:

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/* Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;

	/* Player health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthComponent;

/// <summary>
/// Method Section
/// </summary>
protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	/// <summary>
	/// On destroyed notified, start respawning on server
	/// </summary>
	virtual void Destroyed() override;

	/// <summary>
	/// On death confirmed, start death logic
	/// </summary>
	void OnHandleDeath();

	/// <summary>
	/// On death completed
	/// </summary>
	void OnDeathFinish();

public:

	/* Event for taking damage. Overriden from APawn */
	UFUNCTION(Category = "Health")
	float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

