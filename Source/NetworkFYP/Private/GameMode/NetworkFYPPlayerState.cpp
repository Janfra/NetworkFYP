// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/NetworkFYPPlayerState.h"
#include "GameMode/NetworkFYPGameMode.h"
#include "Net/UnrealNetwork.h"

/* EOS Headers */
//#include "OnlineSubsystem.h"
//#include "OnlineSubsystemUtils.h"
//#include "OnlineSubsystemTypes.h"
//#include "Interfaces/OnlineIdentityInterface.h"
//#include "Interfaces/OnlineSessionInterface.h"
//#include "Interfaces/OnlineStatsInterface.h"
//#include "OnlineStats.h"
/**/

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

//void ANetworkFYPPlayerState::UpdateStat(FString StatName, int32 StatValue, FString AssociatedLeaderboardName)
//{
//	// This function will add a StatValue to the StatName on the EOS backend.
//	// If the achievement stat threshold is meant, the achievement will unlock
//	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
//	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
//	IOnlineStatsPtr Stats = Subsystem->GetStatsInterface();
//
//	// Check if player is online before trying to update stat 
//	const int LocalPlayerNum = 0;
//	FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(LocalPlayerNum);
//
//	if (!NetId || Identity->GetLoginStatus(*NetId) != ELoginStatus::LoggedIn)
//	{
//		return;
//	}
//
//	// All values in leaderboard and stats are in uppercase, so convert it just in case.
//	StatName = StatName.ToUpper();
//
//	// Prepare stat update by setting input arguments to update stat. 
//	FOnlineStatsUserUpdatedStats StatToUpdate = FOnlineStatsUserUpdatedStats(NetId.ToSharedRef());
//
//	// Unknown type to let backend decide
//	FOnlineStatUpdate IngestAmount = FOnlineStatUpdate(StatValue, FOnlineStatUpdate::EOnlineStatModificationType::Unknown);
//	StatToUpdate.Stats.Add(StatName, IngestAmount);
//
//	TArray<FOnlineStatsUserUpdatedStats> StatsToUpdate;
//	StatsToUpdate.Add(StatToUpdate);
//
//	// Unlike other OSS functions we've seen in previous modules, there is no delegate handle for Stat Updates. 
//	// Instead we will use an inline lambda 
//	Stats->UpdateStats(NetId.ToSharedRef(), StatsToUpdate,
//		FOnlineStatsUpdateStatsComplete::CreateLambda([](
//			const FOnlineError& UpdateResult)
//			{
//				// Just log if the update failed. 
//				if (!UpdateResult.bSucceeded)
//				{
//					UE_LOG(LogTemp, Warning, TEXT("Error updating player statistics: %s"), *UpdateResult.ErrorCode);
//					return;
//				}
//			}));
//
//	// Fetch our 2 leaderboards when updating stats. 
//	if (!AssociatedLeaderboardName.IsEmpty()) 
//	{
//		const FName LeaderboardName = FName(AssociatedLeaderboardName.ToUpper());
//		QueryLeaderboardGlobal(LeaderboardName);
//		QueryLeaderboardFriends(StatName, LeaderboardName);
//	}
//}
//
//void ANetworkFYPPlayerState::QueryLeaderboardGlobal(FName LeaderboardName)
//{
//	// This function will retrieve a global leaderboard for a certain rank range
//	// The rank range is hardcoded to 0,10. In a real game you may want to pass this as a parameter. 
//
//	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
//	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
//	IOnlineLeaderboardsPtr Leaderboards = Subsystem->GetLeaderboardsInterface();
//
//	// Again check if the player is logged in 
//	FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(0);
//
//	if (!NetId || Identity->GetLoginStatus(*NetId) != ELoginStatus::LoggedIn)
//	{
//		return;
//	}
//
//	FOnlineLeaderboardReadRef GlobalLeaderboardReadRef = MakeShared<FOnlineLeaderboardRead, ESPMode::ThreadSafe>();
//	GlobalLeaderboardReadRef->LeaderboardName = LeaderboardName;
//
//	// Create a delegate handle and pass in the function to execute once the leaderboard is fetch. 
//	// The function here is the same as with the friends leaderboard. 
//	QueryLeaderboardDelegateHandle =
//		Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateUObject(
//			this,
//			&ThisClass::OnHandleQueryLeaderboarComplete,
//			GlobalLeaderboardReadRef));
//
//	// Try to read the leaderboard. If it fails, log the error, clear and reset the delegate. 
//	if (!Leaderboards->ReadLeaderboardsAroundRank(0, 10, GlobalLeaderboardReadRef))
//	{
//		UE_LOG(LogTemp, Error, TEXT("Failed to read global leaderboard."));
//		Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(QueryLeaderboardDelegateHandle);
//		QueryLeaderboardDelegateHandle.Reset();
//	}
//}
//
//void ANetworkFYPPlayerState::OnHandleQueryLeaderboarComplete(bool bWasSuccessful, FOnlineLeaderboardReadRef GlobalLeaderboardReadRef)
//{
//	// Function triggered when either global or friend leaderboard query completes. 
//	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
//	IOnlineLeaderboardsPtr Leaderboards = Subsystem->GetLeaderboardsInterface();
//
//	if (bWasSuccessful)
//	{
//		// In a real game you would store the leaderboard data and show it in a UI. 
//		// To keep things simple in this course, we are writing the data to the UE logs. 
//		for (auto Row : GlobalLeaderboardReadRef->Rows)
//		{
//			UE_LOG(LogTemp, Log, TEXT("Player Id: %s, Player Rank: %d"), *(*Row.PlayerId).ToString(), Row.Rank);
//		}
//	}
//
//	Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(QueryLeaderboardDelegateHandle);
//	QueryLeaderboardDelegateHandle.Reset();
//}
//
//void ANetworkFYPPlayerState::QueryLeaderboardFriends(FString StatName, FName LeaderboardName)
//{
//	// This function will retrieve a friend leaderboard with specific columns and a sorted column.
//	// For this course we are using a single Stat. 
//
//	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
//	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
//	IOnlineLeaderboardsPtr Leaderboards = Subsystem->GetLeaderboardsInterface();
//
//	// Check if player is logged in... 
//	const int LocalPlayerNum = 0;
//	FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(LocalPlayerNum);
//
//	if (!NetId || Identity->GetLoginStatus(*NetId) != ELoginStatus::LoggedIn)
//	{
//		return;
//	}
//
//	// Prepare arguments. Notice the column metadata is a stat and can be different than the sorted column
//	FOnlineLeaderboardReadRef FriendLeaderboardReadRef = MakeShared<FOnlineLeaderboardRead, ESPMode::ThreadSafe>();
//	FriendLeaderboardReadRef->LeaderboardName = LeaderboardName;
//	FriendLeaderboardReadRef->ColumnMetadata.Add(FColumnMetaData(FName(StatName), EOnlineKeyValuePairDataType::Int32));
//	FriendLeaderboardReadRef->SortedColumn = FName(StatName);
//
//	// Create a delegate handle and pass in the function to execute once the leaderboard is fetch. 
//	// The function here is the same as with the global leaderboard. 
//	QueryLeaderboardDelegateHandle =
//		Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateUObject(
//			this,
//			&ThisClass::OnHandleQueryLeaderboarComplete,
//			FriendLeaderboardReadRef));
//
//	// Try to read the leaderboard. If it fails, log the error, clear and reset the delegate. 
//	if (!Leaderboards->ReadLeaderboardsForFriends(LocalPlayerNum, FriendLeaderboardReadRef))
//	{
//		UE_LOG(LogTemp, Error, TEXT("Failed to read friend leaderboard."));
//		Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(QueryLeaderboardDelegateHandle);
//		QueryLeaderboardDelegateHandle.Reset();
//	}
//}

