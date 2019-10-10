// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCMagazineBase.h"
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

	// General weapon vars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<EFiremode> FireModes;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	EFiremode CurrentFireMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<APCMagazineBase> MagazineClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	APCMagazineBase* CurrentMagazine;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FName MagazineSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") // Rate of Fire in Rounds Per Minute
	float RateOfFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon") // Arbitrary number, 8.0f is ideal for a Glock17
	float AimSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float HipInertiaModifier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float AimInertiaModifier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	
	// Animations
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Animations")
	UAnimSequence* EquipAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Animations")
	UAnimSequence* ReloadAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Animations") // Set the Player's Recoil Animation
	UAnimSequence* FireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Animations")
	UAnimSequence* SingleFireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Animations")
	UAnimSequence* AutoFireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Animations")
	UAnimSequence* MeleeAnimation;


	// Firing/Muzzle effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Effects")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* FireSound;

	UAudioComponent* FireAudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Effects")
	UParticleSystem* ImpactEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Effects")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	FVector MuzzleFlashScale;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* EmptyMagSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* MagEjectSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* MagInsertSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* WeaponRaiseSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* WeaponLowerSound;

	UAudioComponent* MagazineAudioComponent;
	UAudioComponent* MagazineEjectAudioComponent;
	UAudioComponent* MagazineInsertAudioComponent;
	UAudioComponent* WeaponRaiseAudioComponent;
	UAudioComponent* WeaponLowerAudioComponent;


	// Shell eject effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Effects")
	UParticleSystem* ShellEjectEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Effects")
	USoundCue* ShellEjectSound;

	UAudioComponent* ShellEjectAudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Effects")
	FName ShellEjectSocketName;

	
	// Holster vars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon (Other)")
	FName HolsterSocketName;


	// Gun offsets
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Offsets")
	FVector HipLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Offsets")
	FRotator HipRotation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Offsets")
	FVector AimLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Offsets")
	FRotator AimRotation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Offsets")
	FVector ADSOffsetVector;

	UAnimInstance* PlayerAnimInstance; //Player Mesh's Animation Controller - Saved as a Class Variable
	UAnimInstance* AnimInstance;
	int ShotCounter; // Counts how many shots, used for firemodes
	float LastFireTime; //Private for fire rate
	float TimeBetweenShots; //Private for fire rate
	FTimerHandle TimerHandle_TimeBetweenShots;

	virtual void Fire(); // Replaced by "StartFire()". Fire() is not protected
	void PlayFireEffects();

public:
	USkeletalMeshComponent* GetGunMeshComp();

	FVector GetHipLocation();
	FVector GetAimLocation();
	FRotator GetHipRotation();
	FRotator GetAimRotation();
	FVector GetADSOffset();

	FName GetHolsterSocketName();
	FName GetMagazineSocketName();

	float GetAimSpeed();

	UAnimSequence* GetEquipAnimation();
	UAnimSequence* GetReloadAnimation();
	UAnimSequence* GetMeleeAnimation();

	void SetHipTransform();
	void SetAimTransform();
	void SetZeroTransform();

	void SetPlayerAnimInstance(UAnimInstance* InAnimInstance); //Setter for the controlling Player's Animation Controller

	void StartFire();
	void StopFire();

	void Reload();

	void ChangeFiremode();
	
	UFUNCTION(BlueprintCallable)
	void PlayShellEjectEffect();

	TArray<EFiremode> GetFireModes();

	APCMagazineBase* GetCurrentMagazine();

	void PlayMagEjectSound();
	void PlayMagInsertSound();
	void PlayWeaponRaiseSound();
	void PlayWeaponLowerSound();
};
