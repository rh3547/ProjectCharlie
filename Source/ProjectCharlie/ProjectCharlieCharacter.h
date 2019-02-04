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

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FPCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Weapon;

	class USkeletalMeshComponent* CurrentWeaponMesh;

public:
	AProjectCharlieCharacter();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	float BaseLookUpRate;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "View")
	bool bIsFirstPerson;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapons")
	bool bIsWeaponEquipped;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapons")
	bool bIsRifleEquipped;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	UAnimSequence* EquipRifleAnimation;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Weapons")
	bool bIsAiming;

	bool bCanAim;

	bool bDoingSmoothAim;

	bool bDoingSmoothStopAimWeapon;

	bool bDoingSmoothStopAimCamera;

	FTimerHandle TimerHandle_ADS;

	FTimerHandle TimerHandle_StopADS;

	FVector FPCameraDefaultLocation;

	FRotator FPCameraDefaultRotation;

	//Rob's weapon shit
	APCWeaponBase* CurrentWeapon;

	UPROPERTY(VisibleDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocketName;

	//TODO For multiple weapon types
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<APCWeaponBase> WeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "TEMP")
	TSubclassOf<AActor> FireEffectClass;

	UAnimInstance* AnimInstance; //Used to pass to other things such as the weapon for recoil animation

protected:

	virtual void BeginPlay() override;

	/** Resets HMD orientation in VR. */
	void OnResetVR();

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

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	/** Called for world interaction when interact key is pressed */
	void Interact();

	UFUNCTION(BlueprintCallable)
	void Aim();

	UFUNCTION(BlueprintCallable)
	void StopAim();

	void PostSmoothAim();

	void PreStopSmoothAim();

	void PostStopSmoothAim();

	UFUNCTION(BlueprintCallable)
	void EquipWeapon();

	void PostEquipWeapon();

	void LocalEquipWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipWeapon();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastEquipWeapon();

    UFUNCTION(BlueprintCallable)
	void ToggleView();

	UFUNCTION(BlueprintCallable)
	void SetFirstPerson();

	UFUNCTION(BlueprintCallable)
	void SetThirdPerson();

	void Sprint();

	void StopSprint();

	UFUNCTION(BlueprintCallable)
    void ToggleCrouch();

	UFUNCTION(BlueprintCallable)
	void StartFire();

	UFUNCTION(BlueprintCallable)
	void StopFire();

	// Networking Test Example
	// ==============================================
	void TestFire();

	void LocalTestFire();

	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerTestFire();

	UFUNCTION(NetMulticast, Unreliable, WithValidation)
	void MulticastTestFire();

	UFUNCTION()
	void StopTestFire(AActor* Effect);

	// End Networking Test Example
	// ==============================================

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

