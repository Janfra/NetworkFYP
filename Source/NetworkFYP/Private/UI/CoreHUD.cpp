
#include "UI/CoreHUD.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/NetworkFYPGameMode.h"

DEFINE_LOG_CATEGORY(LogNetworkFYPUI);

void ACoreHUD::BeginPlay()
{
	InitialiseContainedWidgets();
	ApplyDisplayConfiguration();

	if (AController* controller = GetOwningPlayerController())
	{
		controller->OnPossessedPawnChanged.AddDynamic(this, &ACoreHUD::OnPlayerPawnUpdated);
	}
}

void ACoreHUD::InitialiseContainedWidgets()
{
	if (!HUDRootWidgetType)
	{
		return;
	}

	APlayerController* owningPlayer = GetOwningPlayerController();
	if (!owningPlayer) 
	{
		UE_LOG(LogNetworkFYPUI, Error, TEXT("Unable to get owning player controller"));
		return;
	}

	RootWidget = CreateWidget(owningPlayer, HUDRootWidgetType, TEXT("HUDPanel"));
	if (!RootWidget)
	{
		UE_LOG(LogNetworkFYPUI, Error, TEXT("Unable to create root widget"));
		return;
	}

	RootPanel = Cast<UPanelWidget>(RootWidget->GetRootWidget());
	if (!RootPanel) 
	{
		UE_LOG(LogNetworkFYPUI, Error, TEXT("Unable to find root panel"));
		return;
	}

	RootWidget->AddToViewport();
	for(FUserWidgetCreationData widgetData : ContainedWidgetsData)
	{
		UUserWidget* widgetInstance = CreateWidget(owningPlayer, widgetData.WidgetType);
		if (!widgetInstance) 
		{
			continue;
		}

		ContainedWidgetsInstances.Add(widgetInstance);
		RootPanel->AddChild(widgetInstance);
		widgetInstance->SetVisibility(widgetData.Visibility);
	}
}

void ACoreHUD::SetDisplayConfiguration(FDisplayConfiguration NewDisplayConfiguration)
{
	DisplayConfiguration = NewDisplayConfiguration;
	ApplyDisplayConfiguration();
}

void ACoreHUD::ApplyDisplayConfiguration()
{
	APlayerController* owningPlayer = GetOwningPlayerController();
	if (!owningPlayer) 
	{
		UE_LOG(LogNetworkFYPUI, Error, TEXT("Unable to get owning player controller"));
		return;
	}

	switch (DisplayConfiguration.InputMode) 
	{
		case EInputModeType::Game:
		{
			FInputModeGameOnly gameMode;
			gameMode.SetConsumeCaptureMouseDown(false);
			owningPlayer->SetInputMode(gameMode);
		}
			break;

		case EInputModeType::GameAndUI:
		{
			FInputModeGameAndUI gameAndUIMode;
			gameAndUIMode.SetHideCursorDuringCapture(DisplayConfiguration.IsMouseHiddenOnCapture);
			gameAndUIMode.SetLockMouseToViewportBehavior(DisplayConfiguration.MouseLockMode);
			if (DisplayConfiguration.IsRootWidgetFocused)
			{
				gameAndUIMode.SetWidgetToFocus(RootWidget->TakeWidget());
			}
			owningPlayer->SetInputMode(gameAndUIMode); 
		}
			break;

		case EInputModeType::UI: 
		{
			FInputModeUIOnly uiMode;
			uiMode.SetLockMouseToViewportBehavior(DisplayConfiguration.MouseLockMode);
			if (DisplayConfiguration.IsRootWidgetFocused)
			{
				uiMode.SetWidgetToFocus(RootWidget->TakeWidget());
			}
			owningPlayer->SetInputMode(uiMode);
		}
			break;
	}

	owningPlayer->SetShowMouseCursor(DisplayConfiguration.IsMouseDisplayed);
}

void ACoreHUD::OnPlayerPawnUpdated(APawn* OldPawn, APawn* NewPawn)
{
	if (!NewPawn) 
	{
		return;
	}
		
	OnNewPlayerPawn.Broadcast(NewPawn);
}
