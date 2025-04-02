// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
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
	
/// <summary>
/// Method section
/// </summary>
public:
	UFUNCTION(BlueprintCallable, Category = "Utilities|Player")
	static void TryGetCallbackOnValidPlayerState(UPARAM(DisplayName = "Event") FDynamicValidPlayerStateFoundDelegate Delegate, UPARAM(ref) FTimerHandle& ReattemptHandle, TSoftObjectPtr<APlayerController> TargetPlayer);

	virtual void ClientInitialize(class AController* Controller) override;
	
	/// <summary>
	/// Updates a player stat - this is used to unlock achievements. It lets backend decide how to update it.
	/// </summary>
	/// <param name="StatName">Name of the stat to update</param>
	/// <param name="StatValue">Value to update it with</param>
	void UpdateStat(FString StatName, int32 StatValue, FString AssociatedLeaderboardName = "");

	/// <summary>
	/// Returns the requested global leaderboard
	/// </summary>
	/// <param name="LeaderboardName">Requested leaderboard name</param>
	void QueryLeaderboardGlobal(FName LeaderboardName);

	/// <summary>
	/// Returns friends leaderboard based on a single stat
	/// </summary>
	/// <param name="StatName"></param>
	/// <param name="LeaderboardName"></param>
	void QueryLeaderboardFriends(FString StatName, FName LeaderboardName);

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
	/// <summary>
	/// Callback function. This function will run when a global OR friend leaderboard is retrieved
	/// </summary>
	/// <param name="bWasSuccessful">Was the leaderboard found</param>
	/// <param name="GlobalLeaderboardReadRef">Reference to the leaderboard</param>
	void OnHandleQueryLeaderboarComplete(bool bWasSuccessful, FOnlineLeaderboardReadRef GlobalLeaderboardReadRef);

/// <summary>
/// Properties section
/// </summary>
private:
	/// <summary>
	/// Delegate to bind callback event for when a leaderboard is retrieved. Same delgate used for global and friend leaderboards
	/// </summary>
	FDelegateHandle QueryLeaderboardDelegateHandle;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerName)
	FString CustomName;

	const float RenameMinTimeBetweenAttempts = 0.25f;
	FTimerHandle RenameTimeoutTimer;

	UPROPERTY()
	uint32 bIsLocallyPaused : 1;

	static void ReattemptToFindValidPlayerStateForCallback(FTimerHandle& ReattemptHandle, TSoftObjectPtr<APlayerController> TargetPlayer, FDynamicValidPlayerStateFoundDelegate Delegate);
};
