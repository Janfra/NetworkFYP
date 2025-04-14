// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/NetworkFYPGameMode.h"
#include "Actors/NetworkFYPCharacter.h"
#include "Actors/NetworkFYPController.h"
#include "GameMode/NetworkFYPGameSession.h"
#include "GameMode/NetworkFYPPlayerState.h"
#include "UObject/ConstructorHelpers.h"

#include "NetworkUtils.h"

DEFINE_LOG_CATEGORY(LogNetworkFYPGameMode);

ANetworkFYPGameMode::ANetworkFYPGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// Use my player controller that handles login into EOS
	PlayerControllerClass = ANetworkFYPController::StaticClass();
	PlayerStateClass = ANetworkFYPPlayerState::StaticClass();

	// In a real game you may want to have a waiting room before sending players to the level. You can use seamless travel to do this and persist the EOS Session across levels. 
	// This is omitted in this tutorial to keep things simple.
}

void ANetworkFYPGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!OnPlayerRespawn.IsBound()) 
	{
		OnPlayerRespawn.AddDynamic(this, &ANetworkFYPGameMode::PlayerRespawn);
	}
}

void ANetworkFYPGameMode::PlayerRespawn(ACharacter* Character, AController* FallbackController)
{
	// Should not provide a null character, but use fallback if all fails if possible
	check(Character);
	if (Character) 
	{
		// Unposses and clear controller pawn to ensure that the actor is spawned
		AController* CharacterController = Character->GetController();
		Character->UnPossessed();
		if (CharacterController) 
		{
			ClearControllerPawn(CharacterController);
			RestartPlayer(CharacterController);
		}
		else
		{
			ClearControllerPawn(FallbackController);
			RestartPlayer(FallbackController);
		}
	}
	else
	{
		UE_LOG(LogNetworkFYPGameMode, Warning, TEXT("Character should be provided when respawning. Attempting to use fallback controller."));
		ClearControllerPawn(FallbackController);
		RestartPlayer(FallbackController);
	}
}

void ANetworkFYPGameMode::ClearControllerPawn(AController* Controller) const
{
	if (Controller) 
	{
		Controller->UnPossess();
		Controller->SetPawn(nullptr);
	}
}

void ANetworkFYPGameMode::RestartPlayer(AController* NewPlayer)
{
	// Make sure that controller has no pawn or no spawning will happen
	Super::RestartPlayer(NewPlayer);
}

APawn* ANetworkFYPGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// Never save the default player pawns into a map.
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, FTransform(), SpawnInfo))
		{
			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
	}

	return nullptr;
}

void ANetworkFYPGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->GetWorld()->GetTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
}
