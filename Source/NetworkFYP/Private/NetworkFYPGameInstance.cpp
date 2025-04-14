// Created as part of Final Year Project by Janfranco, available in github.com/Janfra/NetworkFYP


#include "NetworkFYPGameInstance.h"
#include "Actors/NetworkFYPController.h"

DEFINE_LOG_CATEGORY(LogNetworkFYPGameInstance);

void UNetworkFYPGameInstance::Init()
{
	UE_LOG(LogNetworkFYPGameInstance, Log, TEXT("NetworkFYPGameInstance initialized."));
	Super::Init();
}

void UNetworkFYPGameInstance::Shutdown()
{
	UE_LOG(LogNetworkFYPGameInstance, Log, TEXT("NetworkFYPGameInstance shutdown."));
	Super::Shutdown();
}

UNetworkFYPGameInstance::UNetworkFYPGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ANetworkFYPController* UNetworkFYPGameInstance::GetPrimaryPlayerController() const
{
	return Cast<ANetworkFYPController>(Super::GetPrimaryPlayerController(false));
}
