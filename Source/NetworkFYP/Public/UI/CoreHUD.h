

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Logging/LogMacros.h"
#include "Components/PanelWidget.h"
#include "CoreHUD.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNetworkFYPUI, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDynamicUpdatedControlledPawn, APawn*, NewPawn);

USTRUCT(BlueprintType)
struct FUserWidgetCreationData 
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> WidgetType;

	UPROPERTY(EditDefaultsOnly)
	ESlateVisibility Visibility;
};

UENUM(BlueprintType) 
enum class EInputModeType : uint8 
{
	Game,
	GameAndUI,
	UI,
};

USTRUCT(BlueprintType)
struct FDisplayConfiguration
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool IsRootWidgetFocused;

	UPROPERTY(EditDefaultsOnly)
	EInputModeType InputMode;

	UPROPERTY(EditDefaultsOnly)
	bool IsMouseDisplayed;

	UPROPERTY(EditDefaultsOnly)
	bool IsMouseHiddenOnCapture;

	UPROPERTY(EditDefaultsOnly)
	EMouseLockMode MouseLockMode;
};

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ACoreHUD : public AHUD
{
	GENERATED_BODY()

/// <summary>
/// Properties Section
/// </summary>
public:

	FORCEINLINE FDisplayConfiguration GetDisplayConfiguration() const { return DisplayConfiguration; }
	
	UPROPERTY(BlueprintAssignable)
	FDynamicUpdatedControlledPawn OnNewPlayerPawn;
	
	/* For convience, set the configuration set in editor */
	void SetDisplayConfiguration(FDisplayConfiguration NewDisplayConfiguration);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FDisplayConfiguration DisplayConfiguration;

	/* Widget that contains the Panel that will contain all other widgets */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDRootWidgetType;
	TObjectPtr<UUserWidget> RootWidget;
	TObjectPtr<UPanelWidget> RootPanel;

	/* Widgets that need to be created and attached to the HUD for usage */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TArray<FUserWidgetCreationData> ContainedWidgetsData;
	TArray<TObjectPtr<UUserWidget>> ContainedWidgetsInstances;

/// <summary>
/// Methods Section
/// </summary>
protected:
	
	virtual void BeginPlay() override;

	/// <summary>
	/// Creates and adds widgets to this HUD
	/// </summary>
	void InitialiseContainedWidgets();

	/// <summary>
	/// Applies the display configuration settings that has been set
	/// </summary>
	void ApplyDisplayConfiguration();

	/// <summary>
	/// Notifies UIs about the HUD new Pawn. NOTE: Alternatively use PlayerState event
	/// </summary>
	/// <param name="OldPawn"></param>
	/// <param name="NewPawn"></param>
	UFUNCTION()
	void OnPlayerPawnUpdated(APawn* OldPawn, APawn* NewPawn);
};
