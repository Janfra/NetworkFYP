// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSessionClient.h"
#include "NetworkFYPController.generated.h"

 //Need to forward declare classes used 
class FOnlineSessionSearch;
class FOnlineSessionSearchResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDynamicSessionFound, FString, SessionName);

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ANetworkFYPController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay();

	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;

	UFUNCTION(BlueprintCallable)
	virtual bool SetLocalPause(bool bPause);

	UFUNCTION(BlueprintCallable)
	bool GetIsLocallyPaused();

	class ANetworkFYPPlayerState* GetNetworkPlayerState();

	UPROPERTY(BlueprintAssignable)
	FDynamicSessionFound OnDynamicSessionFound;

protected:
	/// <summary>
	/// Signs player into EOS Game Services
	/// </summary>
	void Login();

	/// <summary>
	/// Callback method. Called when signing into EOS Game Services completes.
	/// </summary>
	/// <returns></returns>
	void OnHandleLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/// <summary>
	/// Function to find EOS sessions. Hardcoded attribute key/value pair to keep things simple
	/// </summary>
	/// <param name="SearchKey">Key of Pair value to search for</param>
	/// <param name="SearchValue">Value associated to key to search for</param>
	void FindSessions(FName SearchKey = "KeyName", FString SearchValue = "KeyValue");

	/// <summary>
	/// Callback function. This function will run when the session is found.
	/// </summary>
	/// <param name="bWasSuccessful">Was the session found</param>
	/// <param name="Search">Found results</param>
	void OnHandleFindSessionsCompleted(bool bWasSuccessful, TSharedRef<FOnlineSessionSearch> Search);

	/// <summary>
	/// Function to join the EOS session. 
	/// </summary>
	void JoinSession();

	/// <summary>
	/// Callback function. This function will run when the session is joined. 
	/// </summary>
	/// <param name="SessionName"></param>
	/// <param name="Result"></param>
	void OnHandleJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	#pragma region P2P Only Section
#if P2PMODE
	void CreateLobby(FName KeyName = "KeyName", FString KeyValue = "KeyValue");

	void OnHandleCreateLobbyCompleted(FName EOSLobbyName, bool bWasSuccessful);

	void SetupNotifications();

	void OnHandleParticipantChanged(FName EOSLobbyName, const FUniqueNetId& NetId, bool bJoined);
#endif
#pragma endregion

/// <summary>
/// Property Section
/// </summary>
protected:
	/// <summary>
	/// Delegate to bind callback event for login
	/// </summary>
	FDelegateHandle LoginDelegateHandle;

	/// <summary>
	/// Delegate to bind callback event for join session.
	/// </summary>
	FDelegateHandle JoinSessionDelegateHandle;

	/// <summary>
	/// Delegate to bind callback event for when sessions are found.
	/// </summary>
	FDelegateHandle FindSessionsDelegateHandle;

	/// <summary>
	/// This is the connection string for the client to connect to the dedicated server.
	/// </summary>
	FString ConnectString;

	/// <summary>
	/// This is used to store the session to join information from the search. You could pass it as a paramter to JoinSession() instead. 
	/// </summary>
	FOnlineSessionSearchResult* SessionToJoin;

	#pragma region P2P Only Section
#if P2PMODE
	/// <summary>
	/// Delegate to bind callback event for creating lobby
	/// </summary>
	FDelegateHandle CreateLobbyDelegateHandle;

	/// <summary>
	/// Name for the lobby
	/// </summary>
	FString LobbyName = "LobbyName";
#endif
#pragma endregion

private:
	/// <summary>
	/// Keeps a reference to the network player state for convenience
	/// </summary>
	TObjectPtr<class ANetworkFYPPlayerState> ANetworkPlayerState;
};
