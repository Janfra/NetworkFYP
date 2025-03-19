// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/NetworkFYPPlayerState.h"
#include "GameMode/NetworkFYPGameMode.h"
#include "Net/UnrealNetwork.h"

void ANetworkFYPPlayerState::TryGetCallbackOnValidPlayerState(FDynamicValidPlayerStateFoundDelegate Delegate, FTimerHandle& ReattemptHandle, TSoftObjectPtr<APlayerController> TargetPlayer)
{
	if (!TargetPlayer.IsValid()) 
	{
		return;
	}

	if (ReattemptHandle.IsValid()) 
	{
		UE_LOG(LogNetworkFYPGameMode, Warning, TEXT("Try Get Callback On Valid Player State has been cancelled due to given reattempt handle already being valid. Ignore in case that this function was called again before completing."))
		return;
	}

	APlayerController* player = TargetPlayer.Get();
	if (!player) 
	{
		return;
	}

	if (ANetworkFYPPlayerState* playerState = player->GetPlayerState<ANetworkFYPPlayerState>()) 
	{
		Delegate.ExecuteIfBound(playerState);
		return;
	}

	FTimerManager& timerManager = player->GetWorldTimerManager();
	const float ReattemptFor = 1.0f;
	FTimerManagerTimerParameters params;
	params.bLoop = false;
	params.bMaxOncePerFrame = true;

	timerManager.SetTimer(ReattemptHandle, ReattemptFor, params);
	timerManager.SetTimerForNextTick([&ReattemptHandle, TargetPlayer, Delegate] {ReattemptToFindValidPlayerStateForCallback(ReattemptHandle, TargetPlayer, Delegate); });
}

void ANetworkFYPPlayerState::ReattemptToFindValidPlayerStateForCallback(FTimerHandle& ReattemptHandle, TSoftObjectPtr<APlayerController> TargetPlayer, FDynamicValidPlayerStateFoundDelegate Delegate)
{
	if (!TargetPlayer.IsValid() || !ReattemptHandle.IsValid())
	{
		return;
	}

	APlayerController* player = TargetPlayer.Get();
	if (!player)
	{
		return;
	}

	FTimerManager& timerManager = player->GetWorldTimerManager();
	if (ANetworkFYPPlayerState* playerState = player->GetPlayerState<ANetworkFYPPlayerState>())
	{
		Delegate.ExecuteIfBound(playerState);
		timerManager.ClearTimer(ReattemptHandle);
		return;
	}

	if (timerManager.TimerExists(ReattemptHandle)) 
	{
		timerManager.SetTimerForNextTick([&ReattemptHandle, TargetPlayer, Delegate] {ReattemptToFindValidPlayerStateForCallback(ReattemptHandle, TargetPlayer, Delegate); });
	}
}

void ANetworkFYPPlayerState::ClientInitialize(AController* Controller)
{
	Super::ClientInitialize(Controller);
	bUseCustomPlayerNames = true;
}

void ANetworkFYPPlayerState::SetPlayerName(const FString& NewName)
{
	if (bUseCustomPlayerNames)
	{
		CustomName = NewName;

		// RepNotify callback won't get called by net code if we are the server
		ENetMode NetMode = GetNetMode();
		if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
		{
			OnRep_PlayerName();
		}

		ForceNetUpdate();
	}
	else 
	{
		Super::SetPlayerName(NewName);
	}
}

void ANetworkFYPPlayerState::OnRep_PlayerName()
{
	if (bUseCustomPlayerNames) 
	{
		HandleWelcomeMessage();
	}
	else 
	{
		Super::OnRep_PlayerName();
	}
}

void ANetworkFYPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkFYPPlayerState, CustomName);
}

void ANetworkFYPPlayerState::SetIsLocallyPaused(const bool bPaused)
{
	bIsLocallyPaused = bPaused;
	OnDynamicLocallyPaused.Broadcast(bIsLocallyPaused);
}

void ANetworkFYPPlayerState::TrySetPlayerCustomName(const FString& NewName)
{
	if (RenameTimeoutTimer.IsValid()) 
	{
		return;
	}

	SetPlayerCustomName(NewName);
	FTimerManagerTimerParameters params;
	params.bLoop = false;
	params.bMaxOncePerFrame = true;
	GetWorldTimerManager().SetTimer(RenameTimeoutTimer, RenameMinTimeBetweenAttempts, params);
}

void ANetworkFYPPlayerState::SetPlayerCustomName_Implementation(const FString& NewName)
{
	if (bUseCustomPlayerNames)
	{
		SetPlayerName(NewName);
	}
}

/* Added for testing, needs to be updated later */
void ANetworkFYPPlayerState::RegisterPlayerKill(APlayerState* OtherPlayer)
{
	float currentScore = GetScore();
	// May create a data asset that determines this, but for now, hard coded.
	const float killValue = 100.0f;

	SetScore(currentScore + killValue);
	FPlayerKillScoreData killData;
	killData.InstigatorName = GetPlayerName();
	killData.TargetName = OtherPlayer->GetPlayerName();
}
