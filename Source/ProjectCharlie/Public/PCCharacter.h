// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PCCharacter.generated.h"

class APCWeaponBase;

UCLASS()
class PROJECTCHARLIE_API APCCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* CurrentWeaponMesh;

	// Sets default values for this character's properties
	APCCharacter();

	//======================================================================
	// Public Variables
	//======================================================================

	/*
		Health/State Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Health")
	float Health;

	UPROPERTY(BlueprintReadWrite, Category = "Health")
	bool bIsDead;

	/*
		Movement/Rotation Variables
		----------------------------------------------------------------
	*/
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
    float BaseWalkSpeed;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxSprintSpeed;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxCrouchSpeed;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
    float AimWalkSpeed;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIsSprinting;

	/*
		Weapon Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bIsWeaponEquipped;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bIsRifleEquipped;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bCanAim;

	FTimerHandle TimerHandle_EquipWeapon;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bIsAiming;

	bool bDoingSmoothAim;

	bool bDoingSmoothStopAimWeapon;

	FTimerHandle TimerHandle_ADS;

	FTimerHandle TimerHandle_StopADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	APCWeaponBase* CurrentWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponAttachSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<APCWeaponBase> PrimaryWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<APCWeaponBase> SecondaryWeaponClass;

	/*
		Melee Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Melee")
	bool bIsMeleeEquipped;

	/*
		Other Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	float InteractDistance;

	UAnimInstance* AnimInstance; // Used to pass to other things such as the weapon for recoil animation

protected:

	//======================================================================
	// Private Functions
	//======================================================================

	/*
		Included Functions
		----------------------------------------------------------------
	*/
	virtual void BeginPlay() override;

	/*
		Movement/Rotation Functions
		----------------------------------------------------------------
	*/
	UFUNCTION(BlueprintCallable)
	virtual void StartSprint();

	UFUNCTION(BlueprintCallable)
	virtual void StopSprint();

	UFUNCTION(BlueprintCallable)
	virtual void ToggleCrouch();

	/*
		Weapon Functions
		----------------------------------------------------------------
	*/
	UFUNCTION(BlueprintCallable)
	virtual void StartAim();

	UFUNCTION(BlueprintCallable)
	virtual void StopAim();

	virtual void PostSmoothAim();

	virtual void PostStopSmoothAim();

	UFUNCTION(BlueprintCallable)
	virtual void ToggleEquipWeapon();

	UFUNCTION(BlueprintCallable)
	virtual void EquipWeapon();

	virtual void PostEquipWeapon();

	UFUNCTION(BlueprintCallable)
	virtual void UnequipWeapon();

	virtual void LocalToggleEquipWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerToggleEquipWeapon();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastToggleEquipWeapon();

	virtual void ChangeFiremode();

	UFUNCTION(BlueprintCallable)
	virtual void StartFire();

	UFUNCTION(BlueprintCallable)
	virtual void StopFire();

	/*
		Melee Functions
		----------------------------------------------------------------
	*/
	UFUNCTION(BlueprintCallable)
	virtual void ToggleEquipMelee();

	UFUNCTION(BlueprintCallable)
	virtual void EquipMelee();

	UFUNCTION(BlueprintCallable)
	virtual void UnequipMelee();

	UFUNCTION(BlueprintCallable)
	virtual void StartMeleeAttack();

	UFUNCTION(BlueprintCallable)
	virtual void StopMeleeAttack();

	/*
		Other Functions
		----------------------------------------------------------------
	*/
	virtual void Interact();

public:	

	//======================================================================
	// Public Functions
	//======================================================================

	virtual void Tick(float DeltaTime) override;
};
