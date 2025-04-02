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
        UE_LOG(LogTemp, Log, TEXT("Searching for a session..."));
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

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    /* Use make shared so that is thread safe since the process is asynchronous */
    TSharedRef<FOnlineSessionSearch> Search = MakeShared<FOnlineSessionSearch>();

    // Remove the default search parameters that FOnlineSessionSearch sets up.
    Search->QuerySettings.SearchParams.Empty();

    Search->QuerySettings.Set(SearchKey, SearchValue, EOnlineComparisonOp::Equals); // Seach using our Key/Value pair
    FindSessionsDelegateHandle =
        Session->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(
            this,
            &ThisClass::HandleFindSessionsCompleted,
            Search));

    UE_LOG(LogTemp, Log, TEXT("Finding session."));

    /* Hardcoded player num for simplicity */
    const int LocalPlayerNum = 0;
    if (!Session->FindSessions(LocalPlayerNum, Search))
    {
        UE_LOG(LogTemp, Warning, TEXT("Find session failed"));
    }
}

void ANetworkFYPController::HandleFindSessionsCompleted(bool bWasSuccessful, TSharedRef<FOnlineSessionSearch> Search)
{
    // Tutorial 4: This function is triggered via the callback we set in FindSession once the session is found (or there is a failure)

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("Found session."));

        for (auto SessionInSearchResult : Search->SearchResults)
        {
            // Typically you want to check if the session is valid before joining. There is a bug in the EOS OSS where IsValid() returns false when the session is created on a DedicatedServer. 
            // Instead of customizing the engine for this tutorial, we're simply not checking if the session is valid. The code below should go in this if statement once the bug is fixed. 
            if (SessionInSearchResult.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("Valid session found! Bug has been fixed!"));
            }

            //Ensure the connection string is resolvable and store the info in ConnectInfo and in SessionToJoin
            if (Session->GetResolvedConnectString(SessionInSearchResult, NAME_GamePort, ConnectString))
            {
                SessionToJoin = &SessionInSearchResult;
            }

            // For the tutorial we will join the first session found automatically. Usually you would loop through all the sessions and determine which one is best to join. 
            break;
        }

        if (SessionToJoin != nullptr) 
        {
            JoinSession();
        }
        else 
        {
            UE_LOG(LogTemp, Warning, TEXT("Find Sessions failed, but returned successful")); //print warning in logs of failure
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Find Sessions failed.")); //print warning in logs of failure
    }

    Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
    FindSessionsDelegateHandle.Reset();
}

void ANetworkFYPController::JoinSession()
{
    // Tutorial 4: Join the session. 

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

    JoinSessionDelegateHandle =
        Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(
            this,
            &ThisClass::HandleJoinSessionCompleted));

    UE_LOG(LogTemp, Log, TEXT("Joining session."));
    const int LocalPlayerNum = 0;
    if (!Session->JoinSession(LocalPlayerNum, "SessionName", *SessionToJoin))
    {
        UE_LOG(LogTemp, Warning, TEXT("Join session failed"));
    }
}

void ANetworkFYPController::HandleJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    // Tutorial 4: This function is triggered via the callback we set in JoinSession once the session is joined (or there is a failure)

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Log, TEXT("Joined session."));
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
