// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCWeaponBase.generated.h"

class USkeletalMeshComponent; //forward declare
class UDamageType;
class UParticleSystem;
class USoundCue;

UENUM(BlueprintType)
enum class EFiremode : uint8
{
	SINGLE_ACTION UMETA(DisplayName = "Single Action"),
	SEMI_AUTO UMETA(DisplayName = "Semi Auto"),
	THREE_ROUND_BURST UMETA(DisplayName = "Three Round Burst"),
	FULLY_SEMI_AUTO UMETA(DisplayName = "Daddy Mode")
};

/*
	Used for animation movement state tracking, not for reload/equip/fire animations.
*/
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	LONG_GUN UMETA(DisplayName = "Long Gun"),
	LONG_GUN_PISTOL_GRIP UMETA(DisplayName = "Long Gun - Pistol Grip"),
	PISTOL UMETA(DisplayName = "Pistol"),
	LEVER_ACTION UMETA(DisplayName = "Lever Action")
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FName ShellEjectSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UParticleSystem* ImpactEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UAnimSequence* EquipAnimation;

	void PlayFireEffects();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<EFiremode> FireModes;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	EFiremode CurrentFireMode;

	int ShotCounter; //Counts how many shots, used for firemodes

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") //Rate of Fire in Rounds Per Minute
	float RateOfFire; //Rounds Per Minute

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float AimSpeed; // Arbitrary number, 8.0f is ideal for a Glock17

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float HipInertiaModifier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float AimInertiaModifier;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") //Set the Player's Recoil Animation
	UAnimSequence* FireAnimation;

	//Gun offsets
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector HipLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FRotator HipRotation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector AimLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FRotator AimRotation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector ADSOffsetVector;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector MuzzleFlashScale;

	float LastFireTime; //Private for fire rate
	float TimeBetweenShots; //Private for fire rate

	UAudioComponent* FireAudioComponent;

	UAudioComponent* ShellEjectAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	USoundCue* FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	USoundCue* ShellEjectSound;

	virtual void Fire(); // Replaced by "StartFire()". Fire() is not protected

	FTimerHandle TimerHandle_TimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AActor> ProjectileClass;

	UAnimInstance* PlayerAnimInstance; //Player Mesh's Animation Controller - Saved as a Class Variable
	UAnimInstance* AnimInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType WeaponType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UAnimSequence* SingleFireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UAnimSequence* AutoFireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UParticleSystem* ShellEjectEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UAnimSequence* ArmsPose;

public:
	USkeletalMeshComponent* GetGunMeshComp();

	FVector GetHipLocation();
	FVector GetAimLocation();
	FRotator GetHipRotation();
	FRotator GetAimRotation();
	FVector GetADSOffset();

	float GetAimSpeed();

	UAnimSequence* GetEquipAnimation();

	void SetHipTransform();
	void SetAimTransform();

	void SetPlayerAnimInstance(UAnimInstance* InAnimInstance); //Setter for the controlling Player's Animation Controller

	void StartFire();
	void StopFire();

	void ChangeFiremode();

	UFUNCTION(BlueprintCallable)
	void PlayShellEjectEffect();
};
