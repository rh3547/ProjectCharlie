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

//////////////////////////////////////////////////////////////////////////
// AProjectCharlieCharacter

AProjectCharlieCharacter::AProjectCharlieCharacter()
{
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
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create the follow camera component
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create the first person camera component
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FName headSocketName = TEXT("FPCameraSocket");
	FPCamera->SetupAttachment(GetMesh(), headSocketName);
	FPCamera->bUsePawnControlRotation = true;
	FPCamera->SetAutoActivate(false);

	FRotator FMCamRot = FRotator(0.0f, 90.0f, -90.0f);
	FPCamera->SetRelativeRotation(FMCamRot);

	FVector FMCamLoc = FVector(0.0f, 7.0f, 0.0f);
	FPCamera->SetRelativeLocation(FMCamLoc);

	// Create the weapon skeletal mesh component
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->AttachTo(GetMesh(), TEXT("RightHand"), EAttachLocation::SnapToTargetIncludingScale, true);
	Weapon->SetVisibility(false);

	// Initialize values
	BaseWalkSpeed = 400.0f;
	MaxSprintSpeed = 600.0f;
	AimWalkSpeed = 300.0f;
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	InteractDistance = 160.0f;

	bIsWeaponEquipped = false;
	bIsRifleEquipped = false;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProjectCharlieCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	/*
		Movement input bindings
	*/
	PlayerInputComponent->BindAxis("MoveForward", this, &AProjectCharlieCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AProjectCharlieCharacter::MoveRight);

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

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AProjectCharlieCharacter::Interact);
	PlayerInputComponent->BindAction("EquipWeapon", IE_Pressed, this, &AProjectCharlieCharacter::EquipWeapon);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AProjectCharlieCharacter::Aim);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AProjectCharlieCharacter::StopAim);
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

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, true);

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

void AProjectCharlieCharacter::EquipWeapon()
{
	bIsWeaponEquipped = !bIsWeaponEquipped;
	bIsRifleEquipped = !bIsRifleEquipped;
	Weapon->ToggleVisibility();
}

void AProjectCharlieCharacter::Aim()
{
	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
	bIsAiming = true;
}

void AProjectCharlieCharacter::StopAim()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	bIsAiming = false;
}
