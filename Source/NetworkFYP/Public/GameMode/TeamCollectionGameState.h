// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Actors/CollectableActor.h"
#include "TeamCollectionGameState.generated.h"

class ATeamPlayerState;

UENUM(BlueprintType)
enum class ETeam : uint8
{
	TeamA,
	TeamB,

	/* Purely to count total teams count, always last */
	TEAM_COUNT UMETA(Hidden)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDynamicTeamScoreUpdated, ETeam, ScoringTeam, float, CurrentScore);

UCLASS()
class NETWORKFYP_API UTeam : public UObject
{
	GENERATED_BODY()

public:
	UTeam();

	/// <summary>
	/// Sets which team this object is
	/// </summary>
	ETeam Team;

	/// <summary>
	/// Score setter
	/// </summary>
	/// <param name="ScoreValue">Value to override score to</param>
	void SetScore(const float ScoreValue);

	/// <summary>
	/// Adds given value to score
	/// </summary>
	/// <param name="ScoreValue">Value to add to score</param>
	void AddScore(const float ScoreValue);

	/// <summary>
	/// Returns if player is part of this team. NOTE: Only checks ETeam
	/// </summary>
	/// <param name="PlayerState">Player being checked</param>
	/// <returns>Is player assigned to the same team</returns>
	FORCEINLINE const bool IsPlayerOnTeam(ATeamPlayerState* PlayerState);

	/// <summary>
	/// Notifies of the team new score
	/// </summary>
	UPROPERTY(BlueprintAssignable)
	FDynamicTeamScoreUpdated OnScoreUpdated;

protected:
	/// <summary>
	/// Replication setup
	/// </summary>
	/// <param name="OutLifetimeProps"></param>
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/// <summary>
	/// Notifies that this object has implemented replication
	/// </summary>
	/// <returns>Returns whether replication is implemented</returns>
	virtual bool IsSupportedForNetworking() const override { return true; }

	/* Method to ensure parallel calls between server and client */
	/// <summary>
	/// On score changed
	/// </summary>
	void OnTeamScoreUpdate();

	/// <summary>
	/// On Score replication notify
	/// </summary>
	UFUNCTION()
	void OnRep_Score();

	/// <summary>
	/// Total score of the team
	/// </summary>
	UPROPERTY(ReplicatedUsing = OnRep_Score)
	float Score;

	/* Replicate with FastArray? */
	/// <summary>
	/// Players in this team
	/// </summary>
	TArray<TObjectPtr<ATeamPlayerState>> TeamMembers;
};

USTRUCT(BlueprintType)
struct FCollectionScoreData 
{
	GENERATED_BODY()

	// May replace with Collection object or interface
	TObjectPtr<ACollectableActor> CollectedActor;
};

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ATeamCollectionGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	ATeamCollectionGameState();

public:
	/* Team count set to 2 in this case since the gamemode is aimed to be 2 teams only, but could be changed to (uint8)ETeam::TEAM_COUNT and expand as needed */
	/// <summary>
	/// Amount of teams in gamemode
	/// </summary>
	const uint8 TeamCount = 2;

	virtual void PostInitializeComponents() override;

	/// <summary>
	/// Adds player state and assigns a team
	/// </summary>
	/// <param name="PlayerState">Player being added</param>
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	/// <summary>
	/// Adds player state and removes from team
	/// </summary>
	/// <param name="PlayerState">Player being removed</param>
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	/// <summary>
	/// Tries to register the collected item score
	/// </summary>
	/// <param name="PlayerState">Player that scored</param>
	/// <param name="CollectionData">Data to provide the score of the collection</param>
	void RegisterCollectionScore(ATeamPlayerState* PlayerState, FCollectionScoreData CollectionData);

	UFUNCTION(BlueprintCallable)
	const float GetScoreNormalised(const float ScoreValue) const { return ScoreValue / CachedRequiredScoreToWin; }

	/// <summary>
	/// Sets the required score to win. Only valid on server. Intended to be given by gamemode.
	/// </summary>
	/// <param name="ScoreToWin">Score required to win</param>
	void SetCachedRequiredScoreToWin(const float ScoreToWin);

	/// <summary>
	/// Event to notify of teams scores as they update
	/// </summary>
	UPROPERTY(BlueprintAssignable)
	FDynamicTeamScoreUpdated OnTeamScored;

protected:
	/// <summary>
	/// Replication setup
	/// </summary>
	/// <param name="OutLifetimeProps"></param>
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/// <summary>
	/// Clear the replicated subobjects
	/// </summary>
	virtual void Destroyed() override;

	/// <summary>
	/// Cached score required to win from Gamemode
	/// </summary>
	UPROPERTY(Replicated)
	float CachedRequiredScoreToWin;

	/// <summary>
	/// Team one data
	/// </summary>
	UPROPERTY(ReplicatedUsing = OnRep_TeamA)
	TObjectPtr<UTeam> TeamA;

	/// <summary>
	/// Team two data
	/// </summary>
	UPROPERTY(ReplicatedUsing = OnRep_TeamB)
	TObjectPtr<UTeam> TeamB;

	/// <summary>
	/// Create and initialise the team data objects
	/// </summary>
	void InitialiseTeams();

	/// <summary>
	/// On Team A replication notify
	/// </summary>
	/// <param name="LastValue">Value before replication</param>
	UFUNCTION()
	void OnRep_TeamA(UTeam* LastValue);

	/// <summary>
	/// On Team B replication notify
	/// </summary>
	/// <param name="LastValue">Value before replication</param>
	UFUNCTION()
	void OnRep_TeamB(UTeam* LastValue);

	/// <summary>
	/// Sets initial values for a team
	/// </summary>
	/// <param name="Team">Team object to initialise</param>
	/// <param name="AssignTeam">Which team to assign</param>
	void SetupTeam(UTeam* Team, ETeam AssignTeam);

	/// <summary>
	/// Returns the team the given player is a part of
	/// </summary>
	/// <param name="PlayerState">Player to find team</param>
	/// <returns>Given player team</returns>
	UTeam* GetPlayerTeam(ATeamPlayerState* PlayerState);

	/// <summary>
	/// Adds the given score to the player team
	/// </summary>
	/// <param name="PlayerState">Player that scored</param>
	/// <param name="ScoreValue">Value of the score</param>
	void AddScoreToTeam(ATeamPlayerState* PlayerState, const float ScoreValue);

	/// <summary>
	/// All teams notify of their new score via this method
	/// </summary>
	/// <param name="ScoringTeam">Team that scored</param>
	/// <param name="CurrentScore">Team current score</param>
	UFUNCTION()
	void OnTeamScoreUpdate(ETeam ScoringTeam, float CurrentScore);
};
