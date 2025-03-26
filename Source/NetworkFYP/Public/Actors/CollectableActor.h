// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TriggerSphere.h"
#include "Components/ShapeComponent.h"
#include "CollectableActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDynamicCollected);

UCLASS()
class NETWORKFYP_API ACollectableActor : public ATriggerSphere
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACollectableActor();

/// <summary>
/// Properties Section
/// </summary>
public:
	UPROPERTY(BlueprintAssignable)
	FDynamicCollected OnDynamicCollected;

/// <summary>
/// Method Section
/// </summary>
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void AttemptToCollect(AActor* OtherActor);

	UFUNCTION()
	virtual void OnTriggerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnTriggerExit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
