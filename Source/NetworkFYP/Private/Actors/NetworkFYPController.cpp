// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/NetworkFYPController.h"
#include "GameMode/NetworkFYPPlayerState.h"

bool ANetworkFYPController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate)
{
	return Super::SetPause(bPause, CanUnpauseDelegate);
}

bool ANetworkFYPController::SetLocalPause(bool bPause) 
{
	bool bResult = false;
	if (IsLocalPlayerController())
	{
		if (ANetworkFYPPlayerState* playerState = GetNetworkPlayerState()) 
		{
			playerState->SetIsLocallyPaused(bPause);
			bResult = true;
		}
	}
	
	return bResult;
}

bool ANetworkFYPController::GetIsLocallyPaused()
{
	bool bResult = false;
	if (ANetworkFYPPlayerState* playerState = GetNetworkPlayerState()) 
	{
		bResult = playerState->GetIsLocallyPaused();
	}

	return bResult;
}

ANetworkFYPPlayerState* ANetworkFYPController::GetNetworkPlayerState()
{
	if (!ANetworkPlayerState)
	{
		ANetworkPlayerState = GetPlayerState<ANetworkFYPPlayerState>();
	}

	return ANetworkPlayerState;
}
