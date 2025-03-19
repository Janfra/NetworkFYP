// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NetworkFYPController.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKFYP_API ANetworkFYPController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;

	UFUNCTION(BlueprintCallable)
	virtual bool SetLocalPause(bool bPause);

	UFUNCTION(BlueprintCallable)
	bool GetIsLocallyPaused();

	class ANetworkFYPPlayerState* GetNetworkPlayerState();

private:
	TObjectPtr<class ANetworkFYPPlayerState> ANetworkPlayerState;
};
