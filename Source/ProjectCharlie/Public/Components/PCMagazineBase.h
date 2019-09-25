// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCProjectileBase.h"
#include "PCMagazineBase.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class PROJECTCHARLIE_API APCMagazineBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APCMagazineBase();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components") // The static mesh component to display the magazine
	UStaticMeshComponent* MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // The mesh to show when the magazine is empty
	UStaticMesh* EmptyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // The mesh to show when the magazine is full
	UStaticMesh* FullMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // The location to offset from the socket of the gun
	FVector GunSocketOffsetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // The location to offset from the socket of the character's hand
	FVector HandSocketOffsetLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Offsets") // The rotation to offset from the socket of the gun
	FRotator GunSocketOffsetRotation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Offsets") // The rotation to offset from the socket of the character's hand
	FRotator HandSocketOffsetRotation;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // Maximum magazine capacity
	int MaxCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // The number of rounds left in the magazine
	int RoundsRemaining;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine") // The projectile class that this magazine can contain
	TSubclassOf<APCProjectileBase> ProjectileClass;


	UStaticMeshComponent* GetMagazineMeshComp();

	void ShowEmptyMesh();

	void ShowFullMesh();

	FVector GetGunSocketOffsetLocation();
	FVector GetHandSocketOffsetLocation();
	FRotator GetGunSocketOffsetRotation();
	FRotator GetHandSocketOffsetRotation();

	void DoGunOffset();
	void DoHandOffset();

	void Empty();

	void Refill();

	void UnloadOneRound();

	void LoadOneRound();

	bool IsEmpty();
};
