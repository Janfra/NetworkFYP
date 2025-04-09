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
	/// <summary>
	/// Specifies which team this player is in
	/// </summary>
	ETeam PlayerTeam;

	/// <summary>
	/// Score accumulated by this player
	/// </summary>
	float CollectedScore;
};
