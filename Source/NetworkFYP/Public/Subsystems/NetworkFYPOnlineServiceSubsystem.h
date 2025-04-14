// Created as part of Final Year Project by Janfranco, available in github.com/Janfra/NetworkFYP

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Online/OnlineServices.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/Auth.h"
#include "Online/Sessions.h"

#include "NetworkFYPOnlineServiceSubsystem.generated.h"

DECLARE_DELEGATE(FLoggedInCallback);

DECLARE_LOG_CATEGORY_EXTERN(LogNetworkFYPOnlineServiceSubsystem, Log, All);

using namespace UE::Online;
typedef FName FPlayerStatName;
typedef FName FLeaderboardName;

struct FOnlinePlayerStats 
{
	static const FPlayerStatName NumberOfCollectedCoins;
};

struct FOnlinePlayerLeaderboards 
{
	static const FLeaderboardName CollectedCoins;
};

/**
 * 
 */
UCLASS()
class NETWORKFYP_API UNetworkFYPOnlineServiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	
	/// <summary>
	/// Helper function to attempt to retrieve this subsystem via the given caller world
	/// </summary>
	/// <param name="Caller">Caller to access world</param>
	/// <returns>Subsystem if found, otherwise nullptr</returns>
	static UNetworkFYPOnlineServiceSubsystem* TryGetSubsystem(TObjectPtr<UObject> Caller);

	/// <summary>
	/// Called to determine whether the Subsystem should be created
	/// </summary>
	/// <param name="Outer">Owner of this object</param>
	/// <returns>Whether to create this subsystem</returns>
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/// <summary>
	/// Called to initialize Game Instance Subsystem
	/// </summary>
	/// <param name="Collection"></param>
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/// <summary>
	/// Called to deinitialize Game Instance Subsystem
	/// </summary>
	virtual void Deinitialize() override;
	
	/// <summary>
	/// Called to register local online user with the Game Instance Subsystem
	/// </summary>
	/// <param name="PlatformUserId"></param>
	void RegisterLocalOnlineUser(FPlatformUserId PlatformUserId, FLoggedInCallback OnLoggedInCallback = nullptr);

	/// <summary>
	/// Called to retrieve online user info for this platform user id
	/// </summary>
	/// <param name="PlatformUserId"></param>
	/// <returns></returns>
	TObjectPtr<UOnlineUserInfo> GetOnlineUserInfo(FPlatformUserId PlatformUserId);

	void FindSessions(FPlatformUserId PlatformUserId, FName SearchKey = "KeyName", FString SearchValue = "KeyValue");

	void JoinSession(FOnlineSessionId SessionId, TObjectPtr<UOnlineUserInfo> OnlineUser);

#if P2PMODE
	void CreateLobby(TObjectPtr<UOnlineUserInfo> OnlineUser, FName KeyName = "KeyName", FString KeyValue = "KeyValue");
#endif

protected:

	struct FOnlineServicesInfo 
	{
		/// <summary>
		/// Access Interfaces through this pointer, can be used to login and display external UI interfaces
		/// </summary>
		IOnlineServicesPtr OnlineServices = nullptr;

		/// <summary>
		/// Auth interface
		/// </summary>
		IAuthPtr AuthInterface = nullptr;

		/// <summary>
		/// 
		/// </summary>
		IExternalUIPtr ExternalUIInterface = nullptr;

		/// <summary>
		/// Session interface
		/// </summary>
		ISessionsPtr SessionInterface = nullptr;

		/// <summary>
		/// Online service implementation type
		/// </summary>
		EOnlineServices OnlineServiceType = UE::Online::EOnlineServices::Epic;

		/// <summary>
		/// Reset struct to initial settings
		/// </summary>
		void Reset() 
		{
			OnlineServices.Reset();
			AuthInterface.Reset();
			ExternalUIInterface.Reset();
			SessionInterface.Reset();
			OnlineServiceType = UE::Online::EOnlineServices::Epic;
		}
	};

	/// <summary>
	/// Pointer to an internal struct containing relevant online services pointers
	/// </summary>
	FOnlineServicesInfo* OnlineServicesInfoInternal = nullptr;

	/// <summary>
	/// Called to initialize online services and interface pointers
	/// </summary>
	void InitializeOnlineServices();

	/// <summary>
	/// Called show login UI to user
	/// </summary>
	/// <param name="PlatformUserId"></param>
	void LoginUser(FPlatformUserId PlatformUserId, FLoggedInCallback OnLoggedInCallback);

	void OnHandleLoginCompleted(const TOnlineResult<FAuthLogin>& LoginResult, FLoggedInCallback OnLoggedInCallback);

	void OnHandleFindSessionCompleted(const TOnlineResult<FFindSessions>& FindResult, TObjectPtr<UOnlineUserInfo> OnlineUser);

	void OnHandleJoinSessionCompleted(const TOnlineResult<FJoinSession>& JoinResult, TObjectPtr<UOnlineUserInfo> OnlineUser);

#if P2PMODE
	void OnHandleCreateLobbyCompleted(const TOnlineResult<FCreateSession>& CreateResult, TObjectPtr<UOnlineUserInfo> OnlineUser);
	
	void OnHandleAddSessionMemberCompleted(const TOnlineResult<FAddSessionMember>& AddResult, TObjectPtr<UOnlineUserInfo> OnlineUser);
#endif

	void UpdateStat(FPlayerStatName StatName, int32 StatValue);

	void QueryLeaderboardGlobal(FLeaderboardName LeaderboardName);

	void QueryLeaderboardFriends(FLeaderboardName LeaderboardName);

	TObjectPtr<UOnlineUserInfo> CreateOnlineUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, FAccountId AccountId, EOnlineServices Services);

	TObjectPtr<UOnlineUserInfo> CreateAndRegisterUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, FAccountId AccountId, EOnlineServices Services);

	TMap<FPlatformUserId, TObjectPtr<UOnlineUserInfo>> OnlineUserInfos;

	TMap<FPlayerStatName, FLeaderboardName> AssociatedLeaderboards;

	friend UOnlineUserInfo;
};

UCLASS()
class NETWORKFYP_API UOnlineUserInfo : public UObject
{
	GENERATED_BODY()
	
public:
	UOnlineUserInfo();

	const int32 InvalidIndex = -1;

	/// <summary>
	/// Called to obtain OnlineUserInfo as a string
	/// </summary>
	/// <returns>This object information as a readable string</returns>
	const FString DebugInfoToString();

private:
	int32 LocalUserIndex = InvalidIndex;
	FPlatformUserId PlatformUserId;
	FAccountId AccountId;
	EOnlineServices Services = EOnlineServices::None;

	friend UNetworkFYPOnlineServiceSubsystem;
};