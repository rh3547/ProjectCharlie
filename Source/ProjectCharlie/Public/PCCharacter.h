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

	// Sets default values for this character's properties
	APCCharacter();

	//======================================================================
	// Public Variables
	//======================================================================

	/*
		Status Variables
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float BaseWalkSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxSprintSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxCrouchSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float AimWalkSpeed;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIsSprinting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIsLeaningLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIsLeaningRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LeanAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxLean;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIsPeaking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float PeakAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxPeak;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float ForwardAxisValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float RightAxisValue;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	APCWeaponBase* CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* CurrentWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponAttachSocketName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<APCWeaponBase> PrimaryWeaponClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
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

	UFUNCTION(BlueprintCallable)
	virtual void LeanLeft();

	UFUNCTION(BlueprintCallable)
	virtual void LeanRight();

	UFUNCTION(BlueprintCallable)
	float GetLeanAmount();

	UFUNCTION(BlueprintCallable)
	virtual void Peak();

	UFUNCTION(BlueprintCallable)
	float GetPeakAmount();

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
