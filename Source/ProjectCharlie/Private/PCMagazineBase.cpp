// Fill out your copyright notice in the Description page of Project Settings.

#include "PCMagazineBase.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "PCProjectileBase.h"

// Sets default values
APCMagazineBase::APCMagazineBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create a Mesh Component for the magazine
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));

	if (FullMesh)
	{
		MeshComp->SetStaticMesh(FullMesh);
	}

	// Make Root the Mesh Component
	RootComponent = MeshComp;
}

void APCMagazineBase::BeginPlay()
{
	Super::BeginPlay();
}

UStaticMeshComponent* APCMagazineBase::GetMagazineMeshComp()
{
	return MeshComp;
}

void APCMagazineBase::ShowEmptyMesh()
{
	if (EmptyMesh)
	{
		MeshComp->SetStaticMesh(EmptyMesh);
	}
}

void APCMagazineBase::ShowFullMesh()
{
	if (FullMesh)
	{
		MeshComp->SetStaticMesh(FullMesh);
	}
}

FVector APCMagazineBase::GetGunSocketOffsetLocation()
{
	return GunSocketOffsetLocation;
}

FVector APCMagazineBase::GetHandSocketOffsetLocation()
{
	return HandSocketOffsetLocation;
}

FRotator APCMagazineBase::GetGunSocketOffsetRotation()
{
	return GunSocketOffsetRotation;
}

FRotator APCMagazineBase::GetHandSocketOffsetRotation()
{
	return HandSocketOffsetRotation;
}

void APCMagazineBase::DoGunOffset()
{
	RootComponent->SetRelativeLocationAndRotation(GunSocketOffsetLocation, GunSocketOffsetRotation);
}

void APCMagazineBase::DoHandOffset()
{
	RootComponent->SetRelativeLocationAndRotation(HandSocketOffsetLocation, HandSocketOffsetRotation);
}

void APCMagazineBase::Empty()
{
	RoundsRemaining = 0;
}

void APCMagazineBase::Refill()
{
	RoundsRemaining = MaxCapacity;
}

void APCMagazineBase::UnloadOneRound()
{
	RoundsRemaining = RoundsRemaining - 1;
}

void APCMagazineBase::LoadOneRound()
{
	RoundsRemaining = RoundsRemaining + 1;
}

bool APCMagazineBase::IsEmpty()
{
	return RoundsRemaining <= 0;
}
