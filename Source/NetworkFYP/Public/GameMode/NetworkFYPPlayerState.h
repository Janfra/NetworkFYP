// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetworkFYPPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerKilled, FPlayerKillScoreData)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDynamicLocallyPaused, bool, bIsPaused);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDynamicValidPlayerStateFoundDelegate, ANetworkFYPPlayerState*, PlayerState);

USTRUCT()
struct FPlayerKillScoreData 
{
	GENERATED_BODY()

	FString InstigatorName;
	FString TargetName;
};

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ANetworkFYPPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Utilities|Player")
	static void TryGetCallbackOnValidPlayerState(UPARAM(DisplayName = "Event") FDynamicValidPlayerStateFoundDelegate Delegate, UPARAM(ref) FTimerHandle& ReattemptHandle, TSoftObjectPtr<APlayerController> TargetPlayer);

	virtual void ClientInitialize(class AController* Controller) override;

	virtual void SetPlayerName(const FString& NewName) override;

	virtual void OnRep_PlayerName() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	virtual void SetIsLocallyPaused(const bool bPaused);

	UFUNCTION(BlueprintCallable)
	bool GetIsLocallyPaused() { return bIsLocallyPaused; }

	UFUNCTION(BlueprintCallable)
	void TrySetPlayerCustomName(const FString& NewName);

	UFUNCTION()
	virtual void RegisterPlayerKill(APlayerState* OtherPlayer);

	FPlayerKilled OnPlayerKilled;

	UPROPERTY(BlueprintAssignable)
	FDynamicLocallyPaused OnDynamicLocallyPaused;

protected:
	UFUNCTION(Server, Unreliable)
	void SetPlayerCustomName(const FString& NewName);

private:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerName)
	FString CustomName;

	const float RenameMinTimeBetweenAttempts = 0.25f;
	FTimerHandle RenameTimeoutTimer;

	UPROPERTY()
	uint32 bIsLocallyPaused : 1;

	static void ReattemptToFindValidPlayerStateForCallback(FTimerHandle& ReattemptHandle, TSoftObjectPtr<APlayerController> TargetPlayer, FDynamicValidPlayerStateFoundDelegate Delegate);
};
