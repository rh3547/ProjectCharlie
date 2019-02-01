// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProjectCharlieCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Public/Interactable.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"

//////////////////////////////////////////////////////////////////////////
// AProjectCharlieCharacter

AProjectCharlieCharacter::AProjectCharlieCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Initialize values
	BaseWalkSpeed = 400.0f;
	MaxSprintSpeed = 600.0f;
	MaxCrouchSpeed = 200.0f;
	AimWalkSpeed = 250.0f;
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	InteractDistance = 160.0f;

	bIsWeaponEquipped = false;
	bIsRifleEquipped = false;
	bIsSprinting = false;
	bIsFirstPerson = false;
	bDoingSmoothAim = false;
	bDoingSmoothStopAim = false;

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = MaxCrouchSpeed;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create the camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(GetMesh(), TEXT("FPCameraSocket"), EAttachLocation::SnapToTargetIncludingScale, true);
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 130.f));
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create the follow camera component
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create the first person camera component
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FPCamera->SetupAttachment(GetMesh(), TEXT("FPCameraSocket"));
	FPCamera->bUsePawnControlRotation = true;
	FPCamera->SetAutoActivate(false);
	FPCamera->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	FPCamera->SetRelativeLocation(FVector(0.0f, 7.0f, 0.0f));
	FPCamera->SetFieldOfView(95.0f);

	// Create the weapon skeletal mesh component
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->AttachTo(GetMesh(), TEXT("RightHand"), EAttachLocation::SnapToTargetIncludingScale, true);
	Weapon->SetVisibility(false);
}

void AProjectCharlieCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	/*
		Movement input bindings
	*/
	PlayerInputComponent->BindAxis("MoveForward", this, &AProjectCharlieCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AProjectCharlieCharacter::MoveRight);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AProjectCharlieCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AProjectCharlieCharacter::StopSprint);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AProjectCharlieCharacter::ToggleCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AProjectCharlieCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AProjectCharlieCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AProjectCharlieCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AProjectCharlieCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AProjectCharlieCharacter::OnResetVR);

	/*
		Action input bindings
	*/
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AProjectCharlieCharacter::Interact);
	PlayerInputComponent->BindAction("EquipWeapon", IE_Pressed, this, &AProjectCharlieCharacter::EquipWeapon);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AProjectCharlieCharacter::Aim);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AProjectCharlieCharacter::StopAim);
	PlayerInputComponent->BindAction("ChangeView", IE_Pressed, this, &AProjectCharlieCharacter::ToggleView);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AProjectCharlieCharacter::Fire);
}


void AProjectCharlieCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AProjectCharlieCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AProjectCharlieCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AProjectCharlieCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AProjectCharlieCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AProjectCharlieCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AProjectCharlieCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AProjectCharlieCharacter::Interact()
{
	FHitResult OutHit;
	FVector Start = FPCamera->GetComponentLocation();
	FVector ForwardVector = FPCamera->GetForwardVector();
	FVector End = ((ForwardVector * InteractDistance) + Start);
	FCollisionQueryParams CollisionParams;

	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, true);

	bool IsHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, CollisionParams);

	if (IsHit)
	{
		if (OutHit.bBlockingHit)
		{
			IInteractable* InteractableActor = Cast<IInteractable>(OutHit.Actor);
			if (InteractableActor)
			{
				InteractableActor->Execute_OnInteract(Cast<UObject>(InteractableActor));
			}
		}
	}
}

void AProjectCharlieCharacter::ToggleView()
{
	// Loop needed for multiplayer. Iterate over player controllers and get the locally controlled one
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalController())
		{
			APlayerCameraManager* CamManager = PlayerController->PlayerCameraManager;
			if (CamManager)
			{
				CamManager->StartCameraFade(0.0f, 1.0f, 0.1f, FColor::Black);

				if (bIsFirstPerson)
				{
					SetThirdPerson();
					bIsFirstPerson = false;
				}
				else
				{
					SetFirstPerson();
					bIsFirstPerson = true;
				}

				CamManager->StartCameraFade(1.0f, 0.0f, 0.1f, FColor::Black);
			}
		}
	}
}

void AProjectCharlieCharacter::SetFirstPerson() 
{
	FPCamera->SetActive(true);
	FollowCamera->SetActive(false);

	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AProjectCharlieCharacter::SetThirdPerson()
{
	CameraBoom->TargetArmLength = 300.0f;
	FPCamera->SetActive(false, true);
	FollowCamera->SetActive(true);

	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AProjectCharlieCharacter::EquipWeapon()
{
	bIsWeaponEquipped = !bIsWeaponEquipped;
	bIsRifleEquipped = !bIsRifleEquipped;
	Weapon->ToggleVisibility();

	if (EquipRifleAnimation)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(EquipRifleAnimation, "UpperBody", 0.0f);
		}
	}
}

// Called every frame
void AProjectCharlieCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// Smooth ADS Weapon Position
	if (bIsWeaponEquipped && bIsAiming)
	{
		Weapon->SetRelativeLocation(FMath::VInterpTo(Weapon->RelativeLocation, FVector(-1.136271f, 1.270507f, 0.485004f), DeltaTime, 6.0f));
		Weapon->SetRelativeRotation(FMath::RInterpTo(Weapon->RelativeRotation, FRotator(-17.552761f, 186.628464f, -5.161056f), DeltaTime, 6.0f));
	}
	else if (bIsWeaponEquipped && !bIsAiming)
	{
		Weapon->SetRelativeLocation(FMath::VInterpTo(Weapon->RelativeLocation, FVector(-1.479010f, 1.117747f, 0.095322f), DeltaTime, 6.0f));
		Weapon->SetRelativeRotation(FMath::RInterpTo(Weapon->RelativeRotation, FRotator(-7.215786f, 181.006836f, -0.126218f), DeltaTime, 6.0f));
	}

	// Smooth ADS Camera Position
	if (bIsFirstPerson && bIsWeaponEquipped && bIsAiming && bDoingSmoothAim)
	{
		FPCamera->SetRelativeLocation(FMath::VInterpTo(FPCamera->RelativeLocation, FVector(0.028555f, -0.885138f, -16.598156f), DeltaTime, 7.0f));
		FPCamera->SetRelativeRotation(FMath::RInterpTo(FPCamera->RelativeRotation, FRotator(90.000000f, -56.309914f, -146.310089f), DeltaTime, 7.0f));
	}
	else if (bIsFirstPerson && bIsWeaponEquipped && !bIsAiming && bDoingSmoothStopAim)
	{
		FPCamera->SetRelativeLocation(FMath::VInterpTo(FPCamera->RelativeLocation, FVector(0.0f, 7.0f, 0.0f), DeltaTime, 6.0f));
		FPCamera->SetRelativeRotation(FMath::RInterpTo(FPCamera->RelativeRotation, FRotator(0.0f, 90.0f, -90.0f), DeltaTime, 6.0f));
	}
}

void AProjectCharlieCharacter::Aim()
{
	if (!bIsFirstPerson)
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	bIsAiming = true;
	bDoingSmoothAim = true;
	bDoingSmoothStopAim = false;

	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
	FPCamera->AttachTo(Weapon, TEXT("ADS"), EAttachLocation::KeepWorldPosition);

	GetWorldTimerManager().ClearTimer(TimerHandle_ADS);
	GetWorldTimerManager().SetTimer(TimerHandle_ADS, this, &AProjectCharlieCharacter::ADS, 2.0f, false);
}

void AProjectCharlieCharacter::ADS()
{
	bDoingSmoothAim = false;
}

void AProjectCharlieCharacter::StopAim()
{
	if (!bIsFirstPerson)
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	bIsAiming = false;
	bDoingSmoothAim = false;
	bDoingSmoothStopAim = true;

	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	FPCamera->AttachTo(GetMesh(), TEXT("head"), EAttachLocation::KeepWorldPosition);

	GetWorldTimerManager().ClearTimer(TimerHandle_StopADS);
	GetWorldTimerManager().SetTimer(TimerHandle_StopADS, this, &AProjectCharlieCharacter::StopADS, 2.0f, false);
}

void AProjectCharlieCharacter::StopADS()
{
	bDoingSmoothStopAim = false;
}

void AProjectCharlieCharacter::Fire()
{
	if (bIsWeaponEquipped && bIsAiming)
	{
		if (FireAnimation)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, "Shoulders", 0.0f);
			}
		}
	}
}

void AProjectCharlieCharacter::Sprint()
{
	GetCharacterMovement()->MaxWalkSpeed = MaxSprintSpeed;
	bIsSprinting = true;
}

void AProjectCharlieCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	bIsSprinting = false;
}

void AProjectCharlieCharacter::ToggleCrouch()
{
    if (bIsSprinting)
    {
        return;
    }

    if (!bIsCrouched)
    {
		Crouch();
    }
    else
    {
		UnCrouch();
    }
}

void AProjectCharlieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectCharlieCharacter, bIsFirstPerson);
}
