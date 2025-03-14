// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameMode/NetworkFYPGameMode.h"
#include "TeamCollectionGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTeamCollectionGameMode, Log, All);

/**
 * Gamemode for collection game with two teams
 */
UCLASS()
class NETWORKFYP_API ATeamCollectionGameMode : public ANetworkFYPGameMode
{
	GENERATED_BODY()
public:
	ATeamCollectionGameMode();

	/// <summary>
	/// Returns the required score to win
	/// </summary>
	/// <returns>Score needed by a team to win</returns>
	const float GetScoreRequiredToWin() const { return ScoreToWin; }

	virtual void StartPlay() override;

protected:

	/// <summary>
	/// Sets the required score to win
	/// </summary>
	UPROPERTY(EditAnywhere)
	float ScoreToWin;
};
