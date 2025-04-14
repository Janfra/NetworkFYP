// Created as part of Final Year Project by Janfranco, available in github.com/Janfra/NetworkFYP

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NetworkFYPGameInstance.generated.h"

// Forward Declare classes
class ANetworkFYPController;
class UObject;

DECLARE_LOG_CATEGORY_EXTERN(LogNetworkFYPGameInstance, Log, All);

/**
 * 
 */
UCLASS()
class NETWORKFYP_API UNetworkFYPGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
protected:

	/// <summary>
	/// Called to initialize game instance on game startup
	/// </summary>
	virtual void Init()	override;

	/// <summary>
	/// Called to shutdown game instance on game exit
	/// </summary>
	virtual void Shutdown() override;

public:

	UNetworkFYPGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	ANetworkFYPController* GetPrimaryPlayerController() const;
};
