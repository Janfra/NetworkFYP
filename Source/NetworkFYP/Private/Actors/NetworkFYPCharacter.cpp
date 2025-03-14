// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/NetworkFYPCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Actors/NetworkProjectile.h"
#include "GameMode/NetworkFYPGameMode.h"
#include "Engine/DamageEvents.h"
#include "GameMode/NetworkFYPPlayerState.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANetworkFYPCharacter

ANetworkFYPCharacter::ANetworkFYPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	/* Add health component class */
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeath.AddUObject(this, &ANetworkFYPCharacter::OnHandleDeath);

	/* FIRINGCOMPONENT TODO */
	ProjectileClass = ANetworkProjectile::StaticClass();
	FireRate = 0.35f;
	bIsFiringWeapon = false;
	
}

void ANetworkFYPCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void ANetworkFYPCharacter::Destroyed()
{
	// Only server can trigger respawn
	if (GetLocalRole() == ENetRole::ROLE_Authority) 
	{
		// Initiate player respawn
		AController* controller = GetController();

		Super::Destroyed();

		if (UWorld* world = GetWorld())
		{
			// GameMode only available on server
			if (ANetworkFYPGameMode* gameMode = Cast<ANetworkFYPGameMode>(world->GetAuthGameMode()))
			{
				gameMode->GetOnPlayerRespawn().Broadcast(this, controller);
			}
		}
	}
	else 
	{
		Super::Destroyed();
	}
}

void ANetworkFYPCharacter::OnHandleDeath()
{
	if (IsLocallyControlled()) 
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) 
		{
			DisableInput(PlayerController);
		}
	}

	FCachedDamageTaken damageData = HealthComponent->GetLastDamageTakenData();
	
	/* Ragdoll character */
	if (USkeletalMeshComponent* mesh = GetMesh()) 
	{
		mesh->SetSimulatePhysics(true);
		mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	}

	// Let server verify since clients won't have access to controller
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		if (damageData.EventInstigator.IsValid())
		{
			AController* instigatingPlayer = damageData.EventInstigator.Get();
			if (ANetworkFYPPlayerState* playerState = instigatingPlayer->GetPlayerState<ANetworkFYPPlayerState>())
			{
				playerState->RegisterPlayerKill(GetPlayerState());
			}
		}

		/* Start respawn timer, need to replace with gamemode respawn */
		const float RespawnDelay = 1.2f;
		FTimerHandle timer;
		GetWorld()->GetTimerManager().SetTimer(timer, this, &ANetworkFYPCharacter::OnDeathFinish, RespawnDelay, false);
	}
}

void ANetworkFYPCharacter::OnDeathFinish()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties

void ANetworkFYPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Super to ensure that inherited properties are replicated
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

//////////////////////////////////////////////////////////////////////////
// Input

void ANetworkFYPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANetworkFYPCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANetworkFYPCharacter::Look);

		/* Firing */
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ANetworkFYPCharacter::StartFire);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

}

void ANetworkFYPCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ANetworkFYPCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// START FIRINGCOMPONENT TODO
void ANetworkFYPCharacter::StartFire()
{
	if (!bIsFiringWeapon) 
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &ANetworkFYPCharacter::StopFire, FireRate, false);
		HandleFire();
	}
}

void ANetworkFYPCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

/* Only called on the server due to having the Server specifier, also requiring a special name */
void ANetworkFYPCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetActorRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	ANetworkProjectile* spawnedProjectile = GetWorld()->SpawnActor<ANetworkProjectile>(ProjectileClass, spawnLocation, spawnRotation, spawnParameters);
}

// END FIRINGCOMPONENT TODO

//////////////////////////////////////////////////////////////////////////
// Methods

float ANetworkFYPCharacter::TakeDamage(float DamageTaken, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	/* In case of point damage, push player */ 
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		// Can't use Cast template in this case
		const FPointDamageEvent* pointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		// Should already be normalised, but to ensure
		FVector shotDirection = pointDamage->ShotDirection.GetSafeNormal();
		if (UCharacterMovementComponent* movementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent()))
		{
			movementComponent->AddImpulse(shotDirection * 1000.0f, true);
			movementComponent->HandlePendingLaunch();
		}
	}

	return HealthComponent->ApplyDamage(DamageTaken, DamageEvent, EventInstigator, DamageCauser);
}