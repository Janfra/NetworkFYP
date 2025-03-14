// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/NetworkFYPPlayerState.h"
#include "GameMode/TeamCollectionGameState.h"
#include "TeamPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ATeamPlayerState : public ANetworkFYPPlayerState
{
	GENERATED_BODY()
	
public:
	/* Specifies which team this player is. Used to easily access team related data in game state. WIP */
	ETeam PlayerTeam;
};
