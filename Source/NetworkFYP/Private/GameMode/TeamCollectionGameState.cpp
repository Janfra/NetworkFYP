// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TeamCollectionGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameMode/TeamCollectionGameMode.h"
#include "GameMode/TeamPlayerState.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/ActorChannel.h"

UTeam::UTeam()
{
}

void UTeam::SetScore(const float ScoreValue)
{
	Score = ScoreValue;

	/* Removed due to not having source engine */
	//MARK_PROPERTY_DIRTY_FROM_NAME(UTeam, Score, this);

	OnTeamScoreUpdate();
}

void UTeam::AddScore(const float ScoreValue)
{
	const float ResultingScore = ScoreValue + Score;
	SetScore(ResultingScore);
}

const bool UTeam::IsPlayerOnTeam(ATeamPlayerState* PlayerState)
{
	return PlayerState->PlayerTeam == Team; 
}

void UTeam::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/* Removed due to not having source engine */
	///* Only when marked as dirty replicate */
	//FDoRepLifetimeParams params;
	//params.bIsPushBased = true;
	//DOREPLIFETIME_WITH_PARAMS_FAST(UTeam, Score, params);

	DOREPLIFETIME(UTeam, Score);
}

void UTeam::OnRep_Score()
{
	OnTeamScoreUpdate();
}

void UTeam::OnTeamScoreUpdate()
{
	OnScoreUpdated.Broadcast(Team, Score);
}

ATeamCollectionGameState::ATeamCollectionGameState()
{
	bReplicateUsingRegisteredSubObjectList = true;
	InitialiseTeams();
}

void ATeamCollectionGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ATeamCollectionGameState::AddPlayerState(APlayerState* PlayerState)
{
	if (GetLocalRole() == ENetRole::ROLE_Authority) 
	{
		if (ATeamPlayerState* TeamPlayerState = Cast<ATeamPlayerState>(PlayerState)) 
		{
			uint8 count = (PlayerArray.Num() % TeamCount);
			TeamPlayerState->PlayerTeam = count < TeamCount ? (ETeam)count : ETeam::TeamA;
		}
	}

	Super::AddPlayerState(PlayerState);
}

void ATeamCollectionGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

}

void ATeamCollectionGameState::RegisterCollectionScore(ATeamPlayerState* PlayerState, FCollectionScoreData CollectionData)
{
	if (!PlayerState || !CollectionData.CollectedActor) 
	{
		return;
	}

	// For now just add a hardcoded value for testing
	AddScoreToTeam(PlayerState, 100.0f);
}

void ATeamCollectionGameState::SetCachedRequiredScoreToWin(const float ScoreToWin)
{
	if (GetLocalRole() != ENetRole::ROLE_Authority) 
	{
		return;
	}

	CachedRequiredScoreToWin = ScoreToWin;

	/* Removed due to not having source engine */
	//MARK_PROPERTY_DIRTY_FROM_NAME(ATeamCollectionGameState, CachedRequiredScoreToWin, this);
}

void ATeamCollectionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/* Removed due to not having source engine */
	///* Only when marked as dirty replicate */
	//FDoRepLifetimeParams params;
	//params.bIsPushBased = true;
	//DOREPLIFETIME_WITH_PARAMS_FAST(ATeamCollectionGameState, CachedRequiredScoreToWin, params);
	//DOREPLIFETIME_WITH_PARAMS_FAST(ATeamCollectionGameState, TeamA, params);
	//DOREPLIFETIME_WITH_PARAMS_FAST(ATeamCollectionGameState, TeamB, params);

	DOREPLIFETIME(ATeamCollectionGameState, CachedRequiredScoreToWin);
	DOREPLIFETIME(ATeamCollectionGameState, TeamA);
	DOREPLIFETIME(ATeamCollectionGameState, TeamB);
}

void ATeamCollectionGameState::Destroyed()
{
	if (IsValid(TeamA)) 
	{
		RemoveReplicatedSubObject(TeamA);
	}

	if (IsValid(TeamB))
	{
		RemoveReplicatedSubObject(TeamB);
	}
}

void ATeamCollectionGameState::InitialiseTeams()
{
	ETeam currentTeam = ETeam::TeamA;
	FString teamName = UEnum::GetValueAsString(currentTeam);
	TeamA = NewObject<UTeam>();
	SetupTeam(TeamA, currentTeam);
	UE_LOG(LogTeamCollectionGameMode, Log, TEXT("Created team data for %s"), *teamName);

	/* Removed due to not having source engine */
	//MARK_PROPERTY_DIRTY_FROM_NAME(ATeamCollectionGameState, TeamA, this);

	currentTeam = ETeam::TeamB;
	teamName = UEnum::GetValueAsString(currentTeam);
	TeamB = NewObject<UTeam>();
	SetupTeam(TeamB, currentTeam);
	UE_LOG(LogTeamCollectionGameMode, Log, TEXT("Created team data for %s"), *teamName);
	
	/* Removed due to not having source engine */
	//MARK_PROPERTY_DIRTY_FROM_NAME(ATeamCollectionGameState, TeamB, this);

	AddReplicatedSubObject(TeamA);
	AddReplicatedSubObject(TeamB);
}

void ATeamCollectionGameState::OnRep_TeamA(UTeam* LastValue)
{
	if (LastValue != nullptr) 
	{
		LastValue->OnScoreUpdated.Clear();
	}

	if (TeamA) 
	{
		SetupTeam(TeamA, ETeam::TeamA);
	}
}

void ATeamCollectionGameState::OnRep_TeamB(UTeam* LastValue)
{
	if (LastValue != nullptr)
	{
		LastValue->OnScoreUpdated.Clear();
	}

	if (TeamB) 
	{
		SetupTeam(TeamB, ETeam::TeamB);
	}
}

void ATeamCollectionGameState::SetupTeam(UTeam* Team, ETeam AssignTeam)
{
	Team->Team = AssignTeam;
	Team->OnScoreUpdated.AddDynamic(this, &ATeamCollectionGameState::OnTeamScoreUpdate);
}

void ATeamCollectionGameState::AddScoreToTeam(ATeamPlayerState* PlayerState, const float ScoreValue)
{
	if (!PlayerState || GetLocalRole() != ENetRole::ROLE_Authority)
	{
		return;
	}

	UTeam* PlayerTeam = GetPlayerTeam(PlayerState);
	checkf(PlayerTeam, TEXT("Player is not assigned to a team. Players should always be assigned to a team."));
	if (!PlayerTeam) 
	{
		return;
	}

	PlayerTeam->AddScore(ScoreValue);
}

void ATeamCollectionGameState::OnTeamScoreUpdate(ETeam ScoringTeam, float CurrentScore)
{
	OnTeamScored.Broadcast(ScoringTeam, CurrentScore);
}

UTeam* ATeamCollectionGameState::GetPlayerTeam(ATeamPlayerState* PlayerState)
{
	if (TeamA->IsPlayerOnTeam(PlayerState)) 
	{
		return TeamA;
	}
	else if (TeamB->IsPlayerOnTeam(PlayerState)) 
	{
		return TeamB;
	}

	return nullptr;
}
