// Created as part of Final Year Project by Janfranco, available in github.com/Janfra/NetworkFYP

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "NetworkFYPGameSession.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ANetworkFYPGameSession : public AGameSession
{
	GENERATED_BODY()

/// <summary>
/// Method Section
/// </summary>
public:
	ANetworkFYPGameSession();
 
protected:
	virtual void BeginPlay();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

	virtual bool ProcessAutoLogin() override;

	virtual void NotifyLogout(const APlayerController* ExitingPlayer) override;

	virtual void RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite) override;

	void OnHandleRegisterPlayerCompleted(FName EOSSessionName, const TArray<FUniqueNetIdRef>& PlayerIds, bool bWasSuccesful);

	virtual void UnregisterPlayer(const APlayerController* ExitingPlayer) override;

	void OnHandleUnregisterPlayerCompleted(FName EOSSessionName, const TArray<FUniqueNetIdRef>& PlayerIds, bool bWasSuccesful);

	void CreateDedicatedServerSession(FName KeyName = "KeyName", FString KeyValue = "KeyValue");

	void OnHandleCreateSessionCompleted(FName EOSSessionName, bool bWasSuccessful);

	void StartSession();

	void OnHandleStartSessionCompleted(FName EOSSessionName, bool bWasSuccessful);

	void EndSession();

	void OnHandleEndSessionCompleted(FName EOSSessionName, bool bWasSuccessful);

	void DestroySession();

	void OnHandleDestroySessionCompleted(FName EOSSessionName, bool bWasSuccesful);

/// <summary>
/// Properties Section
/// </summary>
protected:
	/// <summary>
	/// Delegate to bind callback event for creating session
	/// </summary>
	FDelegateHandle CreateSessionDelegateHandle;

	FDelegateHandle RegisterPlayerDelegateHandle;

	FDelegateHandle UnregisterPlayerDelegateHandle;

	FDelegateHandle StartSessionDelegateHandle;

	FDelegateHandle EndSessionDelegateHandle;

	FDelegateHandle DestroySessionDelegateHandle;

	/// <summary>
	/// Hardcoded for simplicity
	/// </summary>
	FName SessionName = "SessionName";

	/// <summary>
	/// Hardcoded for simplicity
	/// </summary>
	const int MaxNumberOfPlayersInSession = 2;

	/// <summary>
	/// Keep track of player count
	/// </summary>
	int NumberOfPlayersInSession = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Content, meta = (AllowedClasses = "World"))
	FSoftObjectPath Level;

	bool bSessionExists = false;

};
