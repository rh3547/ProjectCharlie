// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectCharlieCharacter.generated.h"

class APCWeaponBase;

UCLASS(config=Game)
class AProjectCharlieCharacter : public ACharacter
{
	GENERATED_BODY()

	//======================================================================
	// Components
	//======================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FPCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Weapon;

	class USkeletalMeshComponent* CurrentWeaponMesh;

public:
	AProjectCharlieCharacter();

	//======================================================================
	// Public Variables
	//======================================================================

	/*
		Movement/Rotation Variables
		----------------------------------------------------------------
	*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float BaseWalkSpeed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float MaxSprintSpeed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float MaxCrouchSpeed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float AimWalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "View")
	float BaseTurnRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "View")
	float BaseLookUpRate;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	bool bIsSprinting;

	/*
		View Variables
		----------------------------------------------------------------
	*/
	FVector FPCameraDefaultLocation;

	FRotator FPCameraDefaultRotation;

	FVector FollowCameraDefaultLocation;

	FRotator FollowCameraDefaultRotation;

	FVector FollowCameraAimLocation;

	FRotator FollowCameraAimRotation;

	float CameraBoomDefaultLength;

	float CameraBoomAimLength;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "View")
	bool bIsFirstPerson;

	/*
		Weapon Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	bool bIsWeaponEquipped;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	bool bIsRifleEquipped;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	bool bCanAim;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	bool bIsAiming;

	bool bDoingSmoothAim;

	bool bDoingSmoothStopAimWeapon;

	bool bDoingSmoothStopAimCamera;

	FTimerHandle TimerHandle_ADS;

	FTimerHandle TimerHandle_StopADS;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	UAnimSequence* EquipRifleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	APCWeaponBase* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<APCWeaponBase> PrimaryWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<APCWeaponBase> SecondaryWeaponClass;

	/*
		Melee Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee")
	bool bIsMeleeEquipped;

	/*
		Other Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	float InteractDistance;

	UAnimInstance* AnimInstance; // Used to pass to other things such as the weapon for recoil animation

	UPROPERTY(EditDefaultsOnly, Category = "TEMP")
	TSubclassOf<AActor> FireEffectClass;

protected:

	//======================================================================
	// Private Functions
	//======================================================================

	/*
		Included Functions
		----------------------------------------------------------------
	*/
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	/*
		Movement/Rotation Functions
		----------------------------------------------------------------
	*/
	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	void StartSprint();

	void StopSprint();

	UFUNCTION(BlueprintCallable)
    void ToggleCrouch();

	/*
		View Functions
		----------------------------------------------------------------
	*/
	UFUNCTION(BlueprintCallable)
	void ToggleView();

	UFUNCTION(BlueprintCallable)
	void SetFirstPerson();

	UFUNCTION(BlueprintCallable)
	void SetThirdPerson();

	/*
		Weapon Functions
		----------------------------------------------------------------
	*/
	UFUNCTION(BlueprintCallable)
	void Aim();

	UFUNCTION(BlueprintCallable)
	void StopAim();

	void PostSmoothAim();

	void PostStopSmoothAim();

	UFUNCTION(BlueprintCallable)
	void EquipWeapon();

	void PostEquipWeapon();

	void LocalEquipWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipWeapon();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastEquipWeapon();

	void ChangeFiremode();

	UFUNCTION(BlueprintCallable)
	void StartFire();

	UFUNCTION(BlueprintCallable)
	void StopFire();

	/*
		Melee Functions
		----------------------------------------------------------------
	*/
	void EquipMelee();

	void MeleeAttack();

	/*
		Other Functions
		----------------------------------------------------------------
	*/
	/** Called for world interaction when interact key is pressed */
	void Interact();

	// Networking Test Example
	void TestFire();

	void LocalTestFire();

	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerTestFire();

	UFUNCTION(NetMulticast, Unreliable, WithValidation)
	void MulticastTestFire();

	UFUNCTION()
	void StopTestFire(AActor* Effect);

	// End Networking Test Example

public:

	//======================================================================
	// Public Functions
	//======================================================================

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

