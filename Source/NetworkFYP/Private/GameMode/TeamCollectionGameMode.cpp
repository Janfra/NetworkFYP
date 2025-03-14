// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/TeamCollectionGameMode.h"
#include "Actors/NetworkFYPCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "GameMode/TeamCollectionGameState.h"
#include "GameMode/TeamPlayerState.h"

DEFINE_LOG_CATEGORY(LogTeamCollectionGameMode);

ATeamCollectionGameMode::ATeamCollectionGameMode()
{
	/* Default team game mode to team specific types */
	GameStateClass = ATeamCollectionGameState::StaticClass();
	PlayerStateClass = ATeamPlayerState::StaticClass();

	ScoreToWin = 1000.0f;
}

void ATeamCollectionGameMode::StartPlay()
{
	Super::StartPlay();

	check(GameState);
	if (ATeamCollectionGameState* gameState = Cast<ATeamCollectionGameState>(GameState))
	{
		gameState->SetCachedRequiredScoreToWin(ScoreToWin);
	}
}
