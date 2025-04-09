// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/NetworkFYPController.h"
#include "GameMode/NetworkFYPPlayerState.h"

/* EOS Headers */
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
/**/

#include "NetworkUtils.h"

void ANetworkFYPController::BeginPlay()
{
    Super::BeginPlay();

    // On BeginPlay call our login function. This is only on the GameClient, not on the DedicatedServer.
    if (GetNetMode() != ENetMode::NM_DedicatedServer) 
    {
        Login();
    }
}

bool ANetworkFYPController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate)
{
	return Super::SetPause(bPause, CanUnpauseDelegate);
}

bool ANetworkFYPController::SetLocalPause(bool bPause) 
{
	bool bResult = false;
	if (IsLocalPlayerController())
	{
		if (ANetworkFYPPlayerState* playerState = GetNetworkPlayerState()) 
		{
			playerState->SetIsLocallyPaused(bPause);
			bResult = true;
		}
	}
	
	return bResult;
}

bool ANetworkFYPController::GetIsLocallyPaused()
{
	bool bResult = false;
	if (ANetworkFYPPlayerState* playerState = GetNetworkPlayerState()) 
	{
		bResult = playerState->GetIsLocallyPaused();
	}

	return bResult;
}

ANetworkFYPPlayerState* ANetworkFYPController::GetNetworkPlayerState()
{
	if (!ANetworkPlayerState)
	{
		ANetworkPlayerState = GetPlayerState<ANetworkFYPPlayerState>();
	}

	return ANetworkPlayerState;
}

void ANetworkFYPController::Login() 
{
    /*
        This function will access the EOS OSS via the OSS identity interface to log first into Epic Account Services, and then into Epic Game Services.
        It will bind a delegate to handle the callback event once login call succeeeds or fails.
        All functions that access the OSS will have this structure: 1-Get OSS interface, 2-Bind delegate for callback and 3-Call OSS interface function (which will call the correspongin EOS OSS function)
        */
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface(); // This is the generic OSS interface that will access the EOS OSS.

    // If you're logged in, don't try to login again.
    // This can happen if your player travels to a dedicated server or different maps as BeginPlay() will be called each time.

    const int LocalPlayerNum = 0;
    FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(LocalPlayerNum);

    if (NetId != nullptr && Identity->GetLoginStatus(LocalPlayerNum) == ELoginStatus::LoggedIn)
    {
        return;
    }

    /* This binds a delegate so we can run our function when the callback completes. 0 represents the player number.
    You should parametrize this Login function and pass the parameter here for splitscreen.
    */
    LoginDelegateHandle =
        Identity->AddOnLoginCompleteDelegate_Handle(LocalPlayerNum, FOnLoginCompleteDelegate::CreateUObject(this, &ANetworkFYPController::OnHandleLoginCompleted));

    // Grab command line parameters. If empty call hardcoded login function - Hardcoded login function useful for Play In Editor. 
    FString AuthType;
    FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);

    if (!AuthType.IsEmpty()) //If parameter is NOT empty we can autologin.
    {
        /*
        In most situations you will want to automatically log a player in using the parameters passed via CLI.
        For example, using the exchange code for the Epic Games Store.
        */
        UE_LOG(LogTemp, Log, TEXT("Logging into EOS...")); // Log to the UE logs that we are trying to log in. 

        if (!Identity->AutoLogin(LocalPlayerNum))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to login... ")); // Log to the UE logs that we are trying to log in.
            // Clear our handle and reset the delegate.
            Identity->ClearOnLoginCompleteDelegate_Handle(LocalPlayerNum, LoginDelegateHandle);
            LoginDelegateHandle.Reset();
        }
    }
    else
    {
        /*
        Fallback if the CLI parameters are empty.Useful for PIE.
        The type here could be developer if using the DevAuthTool, ExchangeCode if the game is launched via the Epic Games Launcher, etc...
        */
        FOnlineAccountCredentials Credentials("AccountPortal", "", "");
        UE_LOG(LogTemp, Log, TEXT("Logging into EOS...")); // Log to the UE logs that we are trying to log in. 

        if (!Identity->Login(LocalPlayerNum, Credentials))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to login... ")); // Log to the UE logs that we are trying to log in.
            // Clear our handle and reset the delegate. 
            Identity->ClearOnLoginCompleteDelegate_Handle(LocalPlayerNum, LoginDelegateHandle);
            LoginDelegateHandle.Reset();
        }
    }
}

void ANetworkFYPController::OnHandleLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    /*
    Tutorial 2: This function handles the callback from logging in. You should not proceed with any EOS features until this function is called.
    This function will remove the delegate that was bound in the Login() function.
    */
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("Login callback completed!"));

        if (NetworkUtils::IsP2PMode()) 
        {
            UE_LOG(LogTemp, Log, TEXT("Searching for a lobby..."));
        }
        else 
        {
            UE_LOG(LogTemp, Log, TEXT("Searching for a session..."));
        }

        // Maybe via button or player action? Maybe add parameters here. For now just try to join game directly
        FindSessions();
    }
    else //Login failed
    {
        // If your game is online only, you may want to return an errror to the user and return to a menu that uses a different GameMode/PlayerController.

        UE_LOG(LogTemp, Warning, TEXT("EOS login failed.")); //Print sign in failure in logs as a warning.
    }

    Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);
    LoginDelegateHandle.Reset();
}

void ANetworkFYPController::FindSessions(FName SearchKey, FString SearchValue)
{
    // Tutorial 4: This function will find our EOS Session that was created by our Dedicated Server. 
    // Tutorial 7: This function will find our EOS lobby. Note that at the OSS layer we are using a Session that is marked as a lobby.  Code is similar with minor tweaks

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    /* Use make shared so that is thread safe since the process is asynchronous */
    TSharedRef<FOnlineSessionSearch> Search = MakeShared<FOnlineSessionSearch>();

    // Remove the default search parameters that FOnlineSessionSearch sets up.
    Search->QuerySettings.SearchParams.Empty();

    Search->QuerySettings.Set(SearchKey, SearchValue, EOnlineComparisonOp::Equals); // Seach using our Key/Value pair
    if (NetworkUtils::IsP2PMode()) 
    {
        Search->QuerySettings.Set(TEXT("SEARCH_LOBBIES"), true, EOnlineComparisonOp::Equals);
        UE_LOG(LogTemp, Log, TEXT("Finding lobby..."));
    }
    else 
    {
        UE_LOG(LogTemp, Log, TEXT("Finding session..."));
    }

    FindSessionsDelegateHandle =
        Session->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(
            this,
            &ThisClass::OnHandleFindSessionsCompleted,
            Search));

    /* Hardcoded player num for simplicity */
    const int LocalPlayerNum = 0;
    if (!Session->FindSessions(LocalPlayerNum, Search))
    {
        if (NetworkUtils::IsP2PMode())
        {
            UE_LOG(LogTemp, Log, TEXT("Finding lobby failed"));
        }
        else 
        {
            UE_LOG(LogTemp, Warning, TEXT("Find session failed"));
        }

        Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
        FindSessionsDelegateHandle.Reset();
    }
}

void ANetworkFYPController::OnHandleFindSessionsCompleted(bool bWasSuccessful, TSharedRef<FOnlineSessionSearch> Search)
{
    // Tutorial 4: This function is triggered via the callback we set in FindSession once the session is found (or there is a failure)
    // Tutorial 7: Same as before, finding the lobby here has the similar code as finding a session. 

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    if (bWasSuccessful)
    {
        if (Search->SearchResults.Num() == 0) 
        {
            if (NetworkUtils::IsP2PMode())
            {
                // If there are no lobbies, create one.
                CreateLobby();
            }
            return;
        }

        if (NetworkUtils::IsP2PMode())
        {
            UE_LOG(LogTemp, Log, TEXT("Found lobby."));
        }
        else 
        {
            UE_LOG(LogTemp, Log, TEXT("Found session."));
        }

        for (auto SessionInSearchResult : Search->SearchResults)
        {
            // Typically you want to check if the session is valid before joining. There is a bug in the EOS OSS where IsValid() returns false when the session is created on a DedicatedServer. 
            // Instead of customizing the engine, we're simply not checking if the session is valid. The code below should go in this if statement once the bug is fixed. 
            if (SessionInSearchResult.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("Valid session found! Bug has been fixed!"));
            }

            // Ensure the connection string is resolvable and store the info in ConnectInfo and in SessionToJoin
            if (Session->GetResolvedConnectString(SessionInSearchResult, NAME_GamePort, ConnectString))
            {
                SessionToJoin = &SessionInSearchResult;
            }

            // In this case join the first session found automatically. Usually you would loop through all the sessions and determine which one is best to join. 
            break;
        }

        if (SessionToJoin != nullptr) 
        {
            JoinSession();
        }
        else 
        {
            if (NetworkUtils::IsP2PMode())
            {
                UE_LOG(LogTemp, Warning, TEXT("Find Lobby failed, but returned successful")); //print warning in logs for failure
            }
            else 
            {
                UE_LOG(LogTemp, Warning, TEXT("Find Sessions failed, but returned successful")); //print warning in logs for failure
            }
        }
    }
    else
    {
        if (NetworkUtils::IsP2PMode())
        {
            UE_LOG(LogTemp, Log, TEXT("Find Lobby failed.")); //print warning in logs for failure
        }
        else 
        {
            UE_LOG(LogTemp, Warning, TEXT("Find Sessions failed.")); //print warning in logs of failure
        }
    }

    Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
    FindSessionsDelegateHandle.Reset();
}

void ANetworkFYPController::JoinSession()
{
    // Tutorial 4: Join the session. 
    // Tutorial 7: Same code is used to join the lobby - just some tweaks to the logging 

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    JoinSessionDelegateHandle =
        Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(
            this,
            &ThisClass::OnHandleJoinSessionCompleted));

    if (NetworkUtils::IsP2PMode())
    {
        UE_LOG(LogTemp, Log, TEXT("Joining lobby."));
    }
    else 
    {
        UE_LOG(LogTemp, Log, TEXT("Joining session."));
    }

    const int LocalPlayerNum = 0;
    if (!Session->JoinSession(LocalPlayerNum, "SessionName", *SessionToJoin))
    {
        if (NetworkUtils::IsP2PMode())
        {
            UE_LOG(LogTemp, Warning, TEXT("Join lobby failed"));
        }
        else 
        {
            UE_LOG(LogTemp, Warning, TEXT("Join session failed"));
        }

        Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
        JoinSessionDelegateHandle.Reset();
    }
}

void ANetworkFYPController::OnHandleJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    // Tutorial 4: This function is triggered via the callback we set in JoinSession once the session is joined (or there is a failure)

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        if (NetworkUtils::IsP2PMode())
        {
            UE_LOG(LogTemp, Log, TEXT("Joined lobby."));
        }
        else 
        {
            UE_LOG(LogTemp, Log, TEXT("Joined session."));
        }

        if (GEngine)
        {
            // For the purposes of this tutorial overriding the ConnectString to point to localhost as we are testing locally. In a real game no need to override. Make sure you can connect over UDP to the ip:port of your server!
            ConnectString = "127.0.0.1:7777";

            FURL DedicatedServerURL(nullptr, *ConnectString, TRAVEL_Absolute);
            FString DedicatedServerJoinError;
            auto DedicatedServerJoinStatus = GEngine->Browse(GEngine->GetWorldContextFromWorldChecked(GetWorld()), DedicatedServerURL, DedicatedServerJoinError);
            if (DedicatedServerJoinStatus == EBrowseReturnVal::Failure)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to browse for dedicated server. Error is: %s"), *DedicatedServerJoinError);
            }

            // To be thorough here you should modify your derived UGameInstance to handle the NetworkError and TravelError events. 
            // As we are testing locally, and for the purposes of keeping this tutorial simple, this is omitted. 
        }
    }
    else 
    {
        ConnectString = "";
    }

    OnDynamicSessionFound.Broadcast(ConnectString);
    Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
    JoinSessionDelegateHandle.Reset();
}

#pragma region P2P Only Section
#if P2PMODE

void ANetworkFYPController::CreateLobby(FName KeyName, FString KeyValue)
{
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    // For now passing a hardcoded level to load
    CreateLobbyDelegateHandle =
        Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(
            this,
            &ThisClass::OnHandleCreateLobbyCompleted, FSoftObjectPath("Game/Content/ThirdPerson/Maps/ThirdPersonMap?listen")));

    TSharedRef<FOnlineSessionSettings> SessionSettings = MakeShared<FOnlineSessionSettings>();
    SessionSettings->NumPublicConnections = 2; //We will test our sessions with 2 players to keep things simple
    SessionSettings->bShouldAdvertise = true; //This creates a public match and will be searchable.
    SessionSettings->bUsesPresence = false;   //No presence on dedicated server. This requires a local user.
    SessionSettings->bAllowJoinViaPresence = false;
    SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
    SessionSettings->bAllowInvites = false;    //Allow inviting players into session. This requires presence and a local user. 
    SessionSettings->bAllowJoinInProgress = false; //Once the session is started, no one can join.
    SessionSettings->bIsDedicated = false; //Session created on dedicated server.
    SessionSettings->bUseLobbiesIfAvailable = true; //For P2P we will use a lobby instead of a session
    SessionSettings->bUseLobbiesVoiceChatIfAvailable = true; //We will also enable voice
    SessionSettings->bUsesStats = true; //Needed to keep track of player stats.
    SessionSettings->Settings.Add(KeyName, FOnlineSessionSetting((KeyValue), EOnlineDataAdvertisementType::ViaOnlineService));

    UE_LOG(LogTemp, Log, TEXT("Creating Lobby..."));

    const int LocalPlayerNum = 0;
    if (!Session->CreateSession(LocalPlayerNum, FName(LobbyName), *SessionSettings))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create Lobby!"));
    }
}

void ANetworkFYPController::OnHandleCreateLobbyCompleted(FName EOSLobbyName, bool bWasSuccessful, FSoftObjectPath Level)
{
    /* Callback function for lobby created */
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    if (bWasSuccessful && Level.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("Lobby: %s Created!"), *EOSLobbyName.ToString());
        FString Map = Level.GetAssetPathString();
        FURL TravelURL;
        TravelURL.Map = Map;
        GetWorld()->Listen(TravelURL);
        SetupNotifications(); // Setup our listeners for lobby notification events 
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create lobby!"));
    }

    // Clear our handle and reset the delegate. 
    Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateLobbyDelegateHandle);
    CreateLobbyDelegateHandle.Reset();
}

void ANetworkFYPController::SetupNotifications()
{
    // Tutorial 7: EOS Lobbies are great as there are notifications sent for our backend when there are changes to lobbies (ex: Participant Joins/Leaves, lobby or lobby member data is updated, etc...) 
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    // Notification for when a participant joins/leaves the lobby. The approach is similar for other notifications. 
    Session->AddOnSessionParticipantsChangeDelegate_Handle(FOnSessionParticipantsChangeDelegate::CreateUObject(
        this,
        &ThisClass::OnHandleParticipantChanged));
}

void ANetworkFYPController::OnHandleParticipantChanged(FName EOSLobbyName, const FUniqueNetId& NetId, bool bJoined)
{
    // Tutorial 7: Callback function called when participants join/leave. 
    if (bJoined)
    {
        UE_LOG(LogTemp, Log, TEXT("A player has joined Lobby: %s"), *LobbyName);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("A player has left Lobby: %s"), *LobbyName);
    }
}

#endif
#pragma endregion
