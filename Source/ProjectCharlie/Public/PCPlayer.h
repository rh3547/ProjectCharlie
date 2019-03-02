// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCCharacter.h"
#include "PCPlayer.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCHARLIE_API APCPlayer : public APCCharacter
{
	GENERATED_BODY()
	
	//======================================================================
	// Components
	//======================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FPCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* FPCameraBoom;

public:
	APCPlayer();

	//======================================================================
	// Public Variables
	//======================================================================

	/*
		Movement/Rotation Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseLookUpRate;

	/*
		View Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	FVector FPCameraDefaultLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	FRotator FPCameraDefaultRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	FVector FollowCameraDefaultLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	FRotator FollowCameraDefaultRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	FVector FollowCameraAimLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	FRotator FollowCameraAimRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	float CameraBoomDefaultLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "View")
	float CameraBoomAimLength;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "View")
	bool bIsFirstPerson;

	/*
		Weapon Variables
		----------------------------------------------------------------
	*/
	bool bDoingSmoothStopAimCamera;

	/*
		Other Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditDefaultsOnly, Category = "TEMP")
	TSubclassOf<AActor> FireEffectClass;

	/*
		Debug Variables
		----------------------------------------------------------------
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug")
	bool bAimLock;

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
	virtual void StartAim() override;

	virtual void StopAim() override;

	virtual void PostStopSmoothAim() override;

	virtual void PostSmoothAim() override;

	/*
		Other Functions
		----------------------------------------------------------------
	*/

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