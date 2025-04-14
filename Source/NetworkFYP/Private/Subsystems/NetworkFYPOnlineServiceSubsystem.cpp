// Created as part of Final Year Project by Janfranco, available in github.com/Janfra/NetworkFYP


#include "Subsystems/NetworkFYPOnlineServiceSubsystem.h"

#include "Online/CoreOnline.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineError.h"
#include "Online/OnlineServices.h"
#include "Online/ExternalUI.h"
#include "NetworkUtils.h"
#include "NetworkFYPGameInstance.h"
#include "Online/SessionsEOSGSTypes.h"

DEFINE_LOG_CATEGORY(LogNetworkFYPOnlineServiceSubsystem);

const FPlayerStatName FOnlinePlayerStats::NumberOfCollectedCoins = FName(TEXT("NUMBEROFCOLLECTEDCOINS"));

const FLeaderboardName FOnlinePlayerLeaderboards::CollectedCoins = FName(TEXT("COINCOLLECTORSLEADERBOARD"));

UNetworkFYPOnlineServiceSubsystem* UNetworkFYPOnlineServiceSubsystem::TryGetSubsystem(TObjectPtr<UObject> Caller)
{
	if (!Caller) 
	{
		return nullptr;
	}

	UWorld* World = Caller->GetWorld();
	if (!World) 
	{
		return nullptr;
	}

	UNetworkFYPGameInstance* GameInstance = Cast<UNetworkFYPGameInstance>(World->GetGameInstance());
	if (!GameInstance) 
	{
		return nullptr;
	}

	UNetworkFYPOnlineServiceSubsystem* OnlineSubsystem = GameInstance->GetSubsystem<UNetworkFYPOnlineServiceSubsystem>();
	if (!OnlineSubsystem)
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Online service subsystem is null"));
		return nullptr;
	}

	return OnlineSubsystem;
}

/// <summary>
/// For simplicity, the subsystem is only
///	created on clients and standalone games, not servers. This function
///	is often used to limit creation of subsystems to a server or client.
///	Be sure to null-check subsystem before usage!
/// </summary>
bool UNetworkFYPOnlineServiceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_SERVER
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}

void UNetworkFYPOnlineServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Log, TEXT("NetworkFYPOnlineServiceSubsystem initialized."));
	Super::Initialize(Collection);

	InitializeOnlineServices();
}

void UNetworkFYPOnlineServiceSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("NetworkFYPOnlineServiceSubsystem deinitialized."));

	// Unbind event handles and reset struct info
	OnlineServicesInfoInternal->Reset();

	delete OnlineServicesInfoInternal;

	// Deinitialize parent class
	Super::Deinitialize();
}

void UNetworkFYPOnlineServiceSubsystem::RegisterLocalOnlineUser(FPlatformUserId PlatformUserId, FLoggedInCallback OnLoggedInCallback)
{
	FAuthGetLocalOnlineUserByPlatformUserId::Params GetUserParams;
	GetUserParams.PlatformUserId = PlatformUserId;
	if (OnlineServicesInfoInternal->AuthInterface.IsValid()) 
	{
		if (OnlineUserInfos.Contains(PlatformUserId)) 
		{
			auto User = GetOnlineUserInfo(PlatformUserId);
			if (User && OnlineServicesInfoInternal->AuthInterface->IsLoggedIn(User->AccountId))
			{
				UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Warning, TEXT("User is already logged in."));
				return;
			}
		}

		LoginUser(PlatformUserId, OnLoggedInCallback);
	}
	else 
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Auth Interface pointer invalid."));
	}
}

void UNetworkFYPOnlineServiceSubsystem::LoginUser(FPlatformUserId PlatformUserId, FLoggedInCallback OnLoggedInCallback)
{
	if (OnlineServicesInfoInternal->AuthInterface.IsValid())
	{
		FString AuthType;
		FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);

		/* Fallback if no arguments are given to the command line */
		if (AuthType.IsEmpty())
		{
			FAuthLogin::Params LoginParams;
			LoginParams.PlatformUserId = PlatformUserId;
			LoginParams.CredentialsId = "AccountPortal";
			LoginParams.CredentialsType = LoginCredentialsType::AccountPortal;
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Logging into EOS..."));

			OnlineServicesInfoInternal->AuthInterface->Login(MoveTemp(LoginParams)).OnComplete(this, &ThisClass::OnHandleLoginCompleted, OnLoggedInCallback);
		}
		else
		{

		}
	}
	else 
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Auth Interface pointer invalid."));
	}
}

void UNetworkFYPOnlineServiceSubsystem::OnHandleLoginCompleted(const TOnlineResult<FAuthLogin>& LoginResult, FLoggedInCallback OnLoggedInCallback)
{
	if (LoginResult.IsOk())
	{
		const FAuthLogin::Result& EOSOnlineUser = LoginResult.GetOkValue();
		TSharedRef<FAccountInfo> OnlineUserInfo = EOSOnlineUser.AccountInfo;
		FAccountInfo OnlineUserInfoContent = *OnlineUserInfo;
		if (!OnlineUserInfos.Contains(OnlineUserInfoContent.PlatformUserId))
		{
			UOnlineUserInfo* NewUser = CreateAndRegisterUserInfo(OnlineUserInfoContent.AccountId.GetHandle(),
				OnlineUserInfoContent.PlatformUserId, OnlineUserInfoContent.AccountId, OnlineUserInfoContent.AccountId.GetOnlineServicesType());
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Local User Registered %s"), *(NewUser->DebugInfoToString()));

			// Notify with callback that user has been successfully logged in and registered
			OnLoggedInCallback.ExecuteIfBound();
		}
		else 
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Warning, TEXT("Local User with platform user id %d already registered."), OnlineUserInfoContent.PlatformUserId.GetInternalId());
		}

		FAuthGetLocalOnlineUserByPlatformUserId::Params GetUserParams;
		GetUserParams.PlatformUserId = OnlineUserInfoContent.PlatformUserId;
		TOnlineResult<FAuthGetLocalOnlineUserByPlatformUserId> AuthGetResult = OnlineServicesInfoInternal->AuthInterface->GetLocalOnlineUserByPlatformUserId(MoveTemp(GetUserParams));
		if (AuthGetResult.IsOk())
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Successfully retrieved Local Online User"));
		}
		else 
		{
			FOnlineError ErrorResult = AuthGetResult.GetErrorValue();
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Get Local Online User Error: %s"), *ErrorResult.GetLogString());
		}

		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Successfully logged in and registered user info"));
	}
	else 
	{
		FOnlineError ErrorResult = LoginResult.GetErrorValue();
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Login Error: %s"), *ErrorResult.GetLogString());
	}
}

TObjectPtr<UOnlineUserInfo> UNetworkFYPOnlineServiceSubsystem::GetOnlineUserInfo(FPlatformUserId PlatformUserId)
{
	TObjectPtr<UOnlineUserInfo> OnlineUser;

	if (OnlineUserInfos.Contains(PlatformUserId))
	{
		OnlineUser = *OnlineUserInfos.Find(PlatformUserId);
	}
	else 
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Could not find user with Platform User Id: %d"), PlatformUserId.GetInternalId());
		OnlineUser = nullptr;
	}

	return OnlineUser;
}

void UNetworkFYPOnlineServiceSubsystem::FindSessions(FPlatformUserId PlatformUserId, FName SearchKey, FString SearchValue)
{
	if (OnlineServicesInfoInternal->SessionInterface.IsValid()) 
	{
		auto OnlineUser = GetOnlineUserInfo(PlatformUserId);
		if (!OnlineUser) 
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Warning, TEXT("User finding session must have already been registered as a online user - Unable to find session"));
			return;
		}

		FFindSessions::Params FindSessionParams;

		// Find session for the given user
		FindSessionParams.LocalAccountId = OnlineUser->AccountId;

		// Find session with the given filter
		FFindSessionsSearchFilter filter;
		filter.Key = SearchKey;
		filter.Value = SearchValue;
		filter.ComparisonOp = ESchemaAttributeComparisonOp::Equals;
		FindSessionParams.Filters.Add(filter);

		// Enable finding lobbies instead of dedicated server sessions when in P2P mode
		if (NetworkUtils::IsP2PMode()) 
		{
			FFindSessionsSearchFilter lobbyFilter;
			// Key name taken from subsystem plugin defined key values
			lobbyFilter.Key = FName(TEXT("LOBBYSEARCH"));
			lobbyFilter.Value = true;
			lobbyFilter.ComparisonOp = ESchemaAttributeComparisonOp::Equals;
			FindSessionParams.Filters.Add(lobbyFilter);

			// Confirm in logs that is searching for lobbies
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Initiating lobby search..."));
		}
		else 
		{
			// Log for sessions instead
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Initiating session search..."));
		}

		OnlineServicesInfoInternal->SessionInterface->FindSessions(MoveTemp(FindSessionParams))
			.OnComplete(this, &ThisClass::OnHandleFindSessionCompleted, OnlineUser);
	}
	else 
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Session Interface pointer invalid."));
	}
}

void UNetworkFYPOnlineServiceSubsystem::OnHandleFindSessionCompleted(const TOnlineResult<FFindSessions>& FindResult, TObjectPtr<UOnlineUserInfo> OnlineUser)
{
	if (FindResult.IsOk())
	{
		const FFindSessions::Result& FoundSessions = FindResult.GetOkValue();
		TArray<FOnlineSessionId> FoundSessionsIds = FoundSessions.FoundSessionIds;

		if (NetworkUtils::IsP2PMode())
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Lobby search was successful!"));
		}
		else
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Sessions search was successful!"));
		}

		if (!FoundSessionsIds.IsEmpty())
		{
			if (NetworkUtils::IsP2PMode())
			{
				UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Joining one of found lobbies..."));
			}
			else
			{
				UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Joining one of found sessions..."));
			}

			for (FOnlineSessionId SessionId : FoundSessionsIds)
			{
				/* Join first session in this case */
				JoinSession(SessionId, OnlineUser);
				break;
			}
		}
		else if (NetworkUtils::IsP2PMode())
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Could not find a lobby, creating one instead..."));

			// Create a lobby if no lobbies are found
			
			// May need to verify user first by intiating autb session?
			//if (OnlineServicesInfoInternal->AuthInterface.IsValid()) 
			//{
			//	OnlineServicesInfoInternal->AuthInterface->BeginVerifiedAuthSession()
			//}

			CreateLobby(OnlineUser);
		}
	}
	else 
	{
		FOnlineError Error = FindResult.GetErrorValue();
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Find Session Error: %s"), *Error.GetLogString());
	}
}

void UNetworkFYPOnlineServiceSubsystem::JoinSession(FOnlineSessionId SessionId, TObjectPtr<UOnlineUserInfo> OnlineUser)
{
	if (!OnlineUser) 
	{
		return;
	}

	FJoinSession::Params JoinSessionParams;
	JoinSessionParams.LocalAccountId = OnlineUser->AccountId;
	JoinSessionParams.SessionId = SessionId;
	JoinSessionParams.SessionName = TEXT("TESTINGSESSION");

	if (OnlineServicesInfoInternal->SessionInterface.IsValid()) 
	{
		if (NetworkUtils::IsP2PMode())
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Joining lobby..."));
		}
		else
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Joining session..."));
		}

		OnlineServicesInfoInternal->SessionInterface->JoinSession(MoveTemp(JoinSessionParams)).OnComplete(this, &ThisClass::OnHandleJoinSessionCompleted, OnlineUser);
	}
	else 
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Session Interface pointer invalid."));
	}
}

void UNetworkFYPOnlineServiceSubsystem::OnHandleJoinSessionCompleted(const TOnlineResult<FJoinSession>& JoinResult, TObjectPtr<UOnlineUserInfo> OnlineUser)
{
	if (JoinResult.IsOk()) 
	{
		if (NetworkUtils::IsP2PMode())
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Successfully joined lobby!"));
		}
		else
		{
			UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Successfully joined session!"));
		}
	}
	else 
	{
		FOnlineError Error = JoinResult.GetErrorValue();
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Join Session Error: %s"), *Error.GetLogString());
	}
}

#if P2PMODE
void UNetworkFYPOnlineServiceSubsystem::CreateLobby(TObjectPtr<UOnlineUserInfo> OnlineUser, FName SearchKey, FString SearchValue)
{
	if (OnlineServicesInfoInternal->SessionInterface.IsValid() && OnlineUser) 
	{
		FCreateSession::Params CreateSessionParams;

		// Set common session values
		CreateSessionParams.SessionName = TEXT("TESTINGSESSION");
		CreateSessionParams.bAllowSanctionedPlayers = false;
		CreateSessionParams.bPresenceEnabled = true;
		CreateSessionParams.LocalAccountId = OnlineUser->AccountId;

		// Set session settings
		FSessionSettings SessionSettings;
		SessionSettings.NumMaxConnections = 2;
		SessionSettings.bAllowNewMembers = true;
		SessionSettings.JoinPolicy = ESessionJoinPolicy::Public;
		SessionSettings.SchemaName = TEXT("TESTINGSESSION");

		FCustomSessionSetting CustomTag;
		CustomTag.Data = SearchValue;
		CustomTag.Visibility = ESchemaAttributeVisibility::Public;
		SessionSettings.CustomSettings.Add(SearchKey, CustomTag);

		CreateSessionParams.SessionSettings = SessionSettings;

		OnlineServicesInfoInternal->SessionInterface->CreateSession(MoveTemp(CreateSessionParams)).OnComplete(this, &ThisClass::OnHandleCreateLobbyCompleted, OnlineUser);
	}
}

void UNetworkFYPOnlineServiceSubsystem::OnHandleCreateLobbyCompleted(const TOnlineResult<FCreateSession>& CreateResult, TObjectPtr<UOnlineUserInfo> OnlineUser)
{
	if (CreateResult.IsOk()) 
	{
		if (OnlineServicesInfoInternal->SessionInterface.IsValid()) 
		{
			FAddSessionMember::Params AddMemberParams;
			AddMemberParams.LocalAccountId = OnlineUser->AccountId;
			AddMemberParams.SessionName = TEXT("Testing Session");

			// Add the lobby creator just in case as recommended by documentation
			OnlineServicesInfoInternal->SessionInterface->AddSessionMember(MoveTemp(AddMemberParams)).OnComplete(this, &ThisClass::OnHandleAddSessionMemberCompleted, OnlineUser);
		}
	}
	else 
	{
		FOnlineError Error = CreateResult.GetErrorValue();
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Create Lobby Error: %s"), *Error.GetLogString());
	}
}

void UNetworkFYPOnlineServiceSubsystem::OnHandleAddSessionMemberCompleted(const TOnlineResult<FAddSessionMember>& AddResult, TObjectPtr<UOnlineUserInfo> OnlineUser)
{
	if (AddResult.IsOk())
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Log, TEXT("Added session member successfully"));
	}
	else
	{
		FOnlineError Error = AddResult.GetErrorValue();
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Create Lobby Error: %s"), *Error.GetLogString());
	}
}
#endif

void UNetworkFYPOnlineServiceSubsystem::InitializeOnlineServices()
{
	OnlineServicesInfoInternal = new FOnlineServicesInfo();

	// Initialize Services ptr
	OnlineServicesInfoInternal->OnlineServices = UE::Online::GetServices();
	check(OnlineServicesInfoInternal->OnlineServices.IsValid());

	// Verify Services type
	OnlineServicesInfoInternal->OnlineServiceType = OnlineServicesInfoInternal->OnlineServices->GetServicesProvider();
	if (OnlineServicesInfoInternal->OnlineServices.IsValid())
	{
		// Initialize Interface ptrs
		OnlineServicesInfoInternal->SessionInterface = OnlineServicesInfoInternal->OnlineServices->GetSessionsInterface();
		check(OnlineServicesInfoInternal->SessionInterface.IsValid());

		OnlineServicesInfoInternal->AuthInterface = OnlineServicesInfoInternal->OnlineServices->GetAuthInterface();
		check(OnlineServicesInfoInternal->AuthInterface.IsValid());

		OnlineServicesInfoInternal->ExternalUIInterface = OnlineServicesInfoInternal->OnlineServices->GetExternalUIInterface();
		check(OnlineServicesInfoInternal->ExternalUIInterface.IsValid());
	}
	else 
	{
		UE_LOG(LogNetworkFYPOnlineServiceSubsystem, Error, TEXT("Error: Failed to initialize services."));
	}
}

void UNetworkFYPOnlineServiceSubsystem::UpdateStat(FPlayerStatName StatName, int32 StatValue)
{
}

void UNetworkFYPOnlineServiceSubsystem::QueryLeaderboardGlobal(FLeaderboardName LeaderboardName)
{
}

void UNetworkFYPOnlineServiceSubsystem::QueryLeaderboardFriends(FLeaderboardName LeaderboardName)
{
}

TObjectPtr<UOnlineUserInfo> UNetworkFYPOnlineServiceSubsystem::CreateOnlineUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, FAccountId AccountId, EOnlineServices Services)
{
	TObjectPtr<UOnlineUserInfo> NewUser = NewObject<UOnlineUserInfo>(this);
	NewUser->LocalUserIndex = LocalUserIndex;
	NewUser->PlatformUserId = PlatformUserId;
	NewUser->AccountId = AccountId;
	NewUser->Services = Services;
	return NewUser;
}

TObjectPtr<UOnlineUserInfo> UNetworkFYPOnlineServiceSubsystem::CreateAndRegisterUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, FAccountId AccountId, EOnlineServices Services)
{
	TObjectPtr<UOnlineUserInfo> NewUser = CreateOnlineUserInfo(LocalUserIndex, PlatformUserId, AccountId, Services);
	OnlineUserInfos.Add(PlatformUserId, NewUser);
	return NewUser;
}

UOnlineUserInfo::UOnlineUserInfo()
{
}

const FString UOnlineUserInfo::DebugInfoToString()
{
	int32 UserIndex = this->LocalUserIndex;
	int32 PlatformId = this->PlatformUserId;
	TArray<FStringFormatArg> FormatArgs;
	FormatArgs.Add(FStringFormatArg(UserIndex));
	FormatArgs.Add(FStringFormatArg(PlatformId));
	return FString::Format(TEXT("LocalUserNumber: {0}, PlatformUserId: {1}"), FormatArgs);
}
