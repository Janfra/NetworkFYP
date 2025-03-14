// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/NetworkFYPGameMode.h"
#include "Actors/NetworkFYPCharacter.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogNetworkFYPGameMode);

ANetworkFYPGameMode::ANetworkFYPGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
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
	check(Character);
	AController* CharacterController = Character->GetController();
	// Unposses to ensure that the actor is spawned
	Character->UnPossessed();

	if (CharacterController) 
	{
		CharacterController->UnPossess();
		CharacterController->SetPawn(nullptr);
		RestartPlayer(CharacterController);
	}
	else if (FallbackController)
	{
		FallbackController->UnPossess();
		FallbackController->SetPawn(nullptr);
		RestartPlayer(FallbackController);
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
