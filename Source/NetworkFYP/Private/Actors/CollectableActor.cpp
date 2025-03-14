// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/CollectableActor.h"
#include "GameMode/TeamCollectionGameState.h"
#include "GameMode/TeamPlayerState.h"
#include "GameFramework/Character.h"

// Sets default values
ACollectableActor::ACollectableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetNetDormancy(ENetDormancy::DORM_Initial);
	bReplicates = true;
}

// Called when the game starts or when spawned
void ACollectableActor::BeginPlay()
{
	Super::BeginPlay();

	UShapeComponent* collision = GetCollisionComponent();
	collision->SetGenerateOverlapEvents(true);
	collision->OnComponentBeginOverlap.AddDynamic(this, &ACollectableActor::OnTriggerEnter);
	collision->OnComponentEndOverlap.AddDynamic(this, &ACollectableActor::OnTriggerExit);
}

void ACollectableActor::OnTriggerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) 
	{
		return;
	}

	ACharacter* character = Cast<ACharacter>(OtherActor);
	if (!character) 
	{
		return;
	}

	if (UWorld* world = GetWorld()) 
	{
		if (ATeamCollectionGameState* gameState = Cast<ATeamCollectionGameState>(world->GetGameState())) 
		{
			if (ATeamPlayerState* playerState = Cast<ATeamPlayerState>(character->GetPlayerState())) 
			{
				/* Testing score: update score and destroy */
				SetNetDormancy(ENetDormancy::DORM_Awake);
				FCollectionScoreData collectionData;
				collectionData.CollectedActor = this;
				gameState->RegisterCollectionScore(playerState, collectionData);

				if (GetLocalRole() == ENetRole::ROLE_Authority) 
				{
					Destroy();
				}
			}
		}
	}
}

void ACollectableActor::OnTriggerExit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

