// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/NetworkFYPPlayerState.h"
#include "Net/UnrealNetwork.h"

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
