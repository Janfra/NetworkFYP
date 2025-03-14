// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NetworkFYPGameMode.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogNetworkFYPGameMode, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDynamicPlayerRespawn, ACharacter*, Character, AController*, FallbackController);

UCLASS(minimalapi)
class ANetworkFYPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANetworkFYPGameMode();

	const FDynamicPlayerRespawn& GetOnPlayerRespawn() const { return OnPlayerRespawn; }

	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void PlayerRespawn(ACharacter* Character, AController* FallbackController);

	UPROPERTY(BlueprintAssignable)
	FDynamicPlayerRespawn OnPlayerRespawn;
};



