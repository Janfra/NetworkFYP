// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/NetworkProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ANetworkProjectile::ANetworkProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	/* Define sphere component for collision */ 
	const float DEFAULT_SPHERE_RADIUS = 37.5f;
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(DEFAULT_SPHERE_RADIUS);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = SphereComponent;

	// Server register projectile impact on hit event to notify clients.
	if (GetLocalRole() == ENetRole::ROLE_Authority) 
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &ANetworkProjectile::OnProjectileImpact);
	}

	/* Define default Mesh for visual representation */
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMesh->SetupAttachment(RootComponent);

	if (DefaultMesh.Succeeded()) 
	{
		StaticMesh->SetStaticMesh(DefaultMesh.Object);
		StaticMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -DEFAULT_SPHERE_RADIUS));
		StaticMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	}

	/* Define default Particle Effect for explosion */
	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (DefaultExplosionEffect.Succeeded())
	{
		ExplosionEffect = DefaultExplosionEffect.Object;
	}

	/* Define the default Projectile Movement Component */
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->SetUpdatedComponent(SphereComponent);
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 1500.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	/* Define default Damage Data */
	DamageType = UDamageType::StaticClass();
	Damage = 10.0f;

}

// Called when the game starts or when spawned
void ANetworkProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void ANetworkProjectile::Destroyed()
{
	FVector spawnLocation = GetActorLocation();
	UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, spawnLocation, FRotator::ZeroRotator, true, EPSCPoolMethod::AutoRelease);
}

void ANetworkProjectile::OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor) 
	{
		const FVector PushDirection = (OtherActor->GetActorLocation() - GetActorLocation());
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, PushDirection.GetSafeNormal(), Hit, GetInstigator()->Controller, this, DamageType);
	}

	Destroy();
}

