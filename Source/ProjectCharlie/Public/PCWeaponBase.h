// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCWeaponBase.generated.h"

class USkeletalMeshComponent; //forward declare
class UDamageType;
class UParticleSystem;

UENUM(BlueprintType)
enum class EFiremode : uint8
{
	SINGLE_ACTION UMETA(DisplayName = "Single Action"),
	SEMI_AUTO UMETA(DisplayName = "Semi Auto"),
	THREE_ROUND_BURST UMETA(DisplayName = "Three Round Burst"),
	FULLY_SEMI_AUTO UMETA(DisplayName = "Daddy Mode")
};

UCLASS()
class PROJECTCHARLIE_API APCWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APCWeaponBase();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType; //Radial, Point, etc

	//MuzzleEffect Stuff
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* ImpactEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	void PlayFireEffects();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<EFiremode> FireModes;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") //Rate of Fire in Rounds Per Minute
	float RateOfFire; //Rounds Per Minute

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") //Set the Player's Recoil Animation
	UAnimSequence* FireAnimation;

	//UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "1", ClampMax = "1000000"))
	//uint32 RoundsPerMinute;

	//Gun offsets
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector HipLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FRotator HipRotation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector AimLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FRotator AimRotation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector ADSOffsetVector;

	float LastFireTime; //Private for fire rate
	float TimeBetweenShots; //Private for fire rate

	//UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire(); //Replaced by "StartFire()". Fire() is not protected

	FTimerHandle TimerHandle_TimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AActor> ProjectileClass;

	UAnimInstance* AnimInstance; //Player Mesh's Animation Controller - Saved as a Class Variable

public:	
	USkeletalMeshComponent* GetGunMeshComp();

	FVector GetHipLocation();
	FVector GetAimLocation();
	FRotator GetHipRotation();
	FRotator GetAimRotation();
	FVector GetADSOffset();

	void SetHipTransform();
	void SetAimTransform();

	void SetPlayerAnimInstance(UAnimInstance* PlayerAnimInstance); //Setter for the controlling Player's Animation Controller

	void StartFire();
	void StopFire();
};
