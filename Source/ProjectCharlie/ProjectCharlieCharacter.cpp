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
#include "PCWeaponBase.h"

//////////////////////////////////////////////////////////////////////////
// AProjectCharlieCharacter

AProjectCharlieCharacter::AProjectCharlieCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	// Initialize values
	BaseWalkSpeed = 400.0f;
	MaxSprintSpeed = 600.0f;
	MaxCrouchSpeed = 200.0f;
	AimWalkSpeed = 250.0f;
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;

	InteractDistance = 160.0f;

	bIsWeaponEquipped = false;
	bIsRifleEquipped = false;
	bIsSprinting = false;
	bIsFirstPerson = false;
	bDoingSmoothAim = false;
	bDoingSmoothStopAimWeapon = false;
	bDoingSmoothStopAimCamera = false;
	bCanAim = false;

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
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
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

	//FPCameraDefaultLocation = FVector(0.0f, 7.0f, 0.0f);
	FPCameraDefaultLocation = FVector(0.0f, 8.699828f, 0.0f);
	FPCameraDefaultRotation = FRotator(0.0f, 90.0f, -90.0f);

	FPCamera->SetRelativeLocation(FPCameraDefaultLocation);
	FPCamera->SetRelativeRotation(FPCameraDefaultRotation);
	FPCamera->SetFieldOfView(90.0f);

	// Get the socket to attach the weapon to. -Rob
	WeaponAttachSocketName = "RightHand"; //"WeaponSocket"
}


void AProjectCharlieCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Get the Player's Anim Instance and Set to Class Variable
	AnimInstance = GetMesh()->GetAnimInstance();

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
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AProjectCharlieCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AProjectCharlieCharacter::StopFire);
	PlayerInputComponent->BindAction("Firemode", IE_Released, this, &AProjectCharlieCharacter::ChangeFiremode);

	PlayerInputComponent->BindAction("Test", IE_Pressed, this, &AProjectCharlieCharacter::TestFire);
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
	if (bWasJumping || bIsAiming)
	{
		return;
	}

	if (Role != ROLE_Authority)
	{
		LocalEquipWeapon();
		ServerEquipWeapon();
	}
	else if (Role == ROLE_Authority)
	{
		LocalEquipWeapon();
		MulticastEquipWeapon();
	}
}

void AProjectCharlieCharacter::LocalEquipWeapon() 
{
	bIsWeaponEquipped = !bIsWeaponEquipped;
	bIsRifleEquipped = !bIsRifleEquipped;

	if (bIsWeaponEquipped)
	{
		//Spawn A Default Weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<APCWeaponBase>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			//Pass the Player's Animation Instance to the Weapon (For Recoil Management, etc.)
			if (AnimInstance)
			{
				CurrentWeapon->SetPlayerAnimInstance(AnimInstance);
			}

			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);

			CurrentWeapon->SetHipTransform();

			CurrentWeaponMesh = CurrentWeapon->GetGunMeshComp();
		}
	}
	else
	{
		CurrentWeaponMesh = nullptr;
		CurrentWeapon->Destroy();

	}

	if (EquipRifleAnimation)
	{
		if (AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(EquipRifleAnimation, "UpperBody", 0.0f);

			FTimerHandle TimerHandle_EquipWeapon;
			GetWorldTimerManager().ClearTimer(TimerHandle_EquipWeapon);
			GetWorldTimerManager().SetTimer(TimerHandle_EquipWeapon, this, &AProjectCharlieCharacter::PostEquipWeapon, false, 1.5f);
		}
	}
}

void AProjectCharlieCharacter::PostEquipWeapon()
{
	bCanAim = bIsWeaponEquipped;
}

void AProjectCharlieCharacter::ServerEquipWeapon_Implementation()
{
	MulticastEquipWeapon();
}

void AProjectCharlieCharacter::MulticastEquipWeapon_Implementation()
{
	if (!IsLocallyControlled())
	{
		LocalEquipWeapon();
	}
}

bool AProjectCharlieCharacter::ServerEquipWeapon_Validate()
{
	return true;
}

bool AProjectCharlieCharacter::MulticastEquipWeapon_Validate()
{
	return true;
}

// Called every frame
void AProjectCharlieCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	 //Smooth ADS Weapon Position
	if (bIsWeaponEquipped && bIsAiming && CurrentWeaponMesh && bDoingSmoothAim)
	{
		CurrentWeaponMesh->SetRelativeLocation(FMath::VInterpTo(CurrentWeaponMesh->RelativeLocation, CurrentWeapon->GetAimLocation(), DeltaTime, 11.0f));
		CurrentWeaponMesh->SetRelativeRotation(FMath::RInterpTo(CurrentWeaponMesh->RelativeRotation, CurrentWeapon->GetAimRotation(), DeltaTime, 11.0f));
	}
	else if (bIsWeaponEquipped && !bIsAiming && CurrentWeaponMesh && bDoingSmoothStopAimWeapon)
	{
		CurrentWeaponMesh->SetRelativeLocation(FMath::VInterpTo(CurrentWeaponMesh->RelativeLocation, CurrentWeapon->GetHipLocation(), DeltaTime, 5.0f));
		CurrentWeaponMesh->SetRelativeRotation(FMath::RInterpTo(CurrentWeaponMesh->RelativeRotation, CurrentWeapon->GetHipRotation(), DeltaTime, 5.0f));
	}

	// Smooth ADS Camera Position
	if (bIsFirstPerson && bIsWeaponEquipped && bIsAiming && bDoingSmoothAim)
	{
		FVector RearSightSocketLocation = CurrentWeaponMesh->GetSocketLocation("RearSight");
		FVector FrontSightSocketLocation = CurrentWeaponMesh->GetSocketLocation("FrontSight");
		FRotator ADSRotator = UKismetMathLibrary::FindLookAtRotation(RearSightSocketLocation, FrontSightSocketLocation);

		FPCamera->SetRelativeLocation(FMath::VInterpTo(FPCamera->RelativeLocation, CurrentWeapon->GetADSOffset(), DeltaTime, 6.0f));
		//FPCamera->SetRelativeRotation(FMath::RInterpTo(FPCamera->RelativeRotation, ADSRotator, DeltaTime, 7.0f));
	}
	else if (bIsFirstPerson && bIsWeaponEquipped && !bIsAiming && bDoingSmoothStopAimCamera)
	{
		FPCamera->SetRelativeLocation(FMath::VInterpTo(FPCamera->RelativeLocation, FPCameraDefaultLocation, DeltaTime, 6.0f));
		FPCamera->SetRelativeRotation(FMath::RInterpTo(FPCamera->RelativeRotation, FPCameraDefaultRotation, DeltaTime, 6.0f));
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
	bDoingSmoothStopAimWeapon = false;
	bDoingSmoothStopAimCamera = false;

	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;

	FName RearSightName = "RearSight";
	FPCamera->AttachTo(CurrentWeaponMesh, RearSightName, EAttachLocation::KeepWorldPosition);

	GetWorldTimerManager().ClearTimer(TimerHandle_ADS);
	GetWorldTimerManager().SetTimer(TimerHandle_ADS, this, &AProjectCharlieCharacter::PostSmoothAim, 2.0f, false);
}

void AProjectCharlieCharacter::ChangeFiremode()
{
	if (CurrentWeapon) {
		CurrentWeapon->ChangeFiremode();
	}
}

void AProjectCharlieCharacter::PostSmoothAim()
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
	bDoingSmoothStopAimCamera = true;
	bDoingSmoothStopAimWeapon = true;

	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

	FName HeadSocketName = "head";
	FPCamera->AttachTo(GetMesh(), HeadSocketName, EAttachLocation::KeepWorldPosition);

	GetWorldTimerManager().ClearTimer(TimerHandle_StopADS);
	GetWorldTimerManager().SetTimer(TimerHandle_StopADS, this, &AProjectCharlieCharacter::PostStopSmoothAim, 0.5f, false);
}

void AProjectCharlieCharacter::PreStopSmoothAim()
{
	bDoingSmoothStopAimWeapon = true;
	GetWorldTimerManager().ClearTimer(TimerHandle_StopADS);
	GetWorldTimerManager().SetTimer(TimerHandle_StopADS, this, &AProjectCharlieCharacter::PostStopSmoothAim, 2.0f, false);
}

void AProjectCharlieCharacter::PostStopSmoothAim()
{
	bDoingSmoothStopAimWeapon = false;
	bDoingSmoothStopAimCamera = false;
}

void AProjectCharlieCharacter::StartFire()
{
	if (!bIsWeaponEquipped || bIsSprinting || bWasJumping)
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire(); // Call the fire function on the weapon
	}
}

void AProjectCharlieCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AProjectCharlieCharacter::Sprint()
{
	if (bWasJumping || bIsCrouched)
	{
		return;
	}

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
    if (bIsSprinting || bWasJumping)
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

// Networking Test Example
// ==============================================

/*
	This is the function you should call to initiate the action every time.
	Don't call the network implementations directly.
*/
void AProjectCharlieCharacter::TestFire() 
{
	// Only called when initiated from a client
	if (Role != ROLE_Authority)
	{
		LocalTestFire(); // Trigger for self
		ServerTestFire(); // Tell server that you triggered
	}

	// Only called when initiated from server
	else if (Role == ROLE_Authority)
	{
		LocalTestFire(); // Trigger for self (on server)
		MulticastTestFire(); // Tell all clients (and self...) that you triggered
	}
}

/*
	This simply activates the desired client side effects, no networking involved.
*/
void AProjectCharlieCharacter::LocalTestFire()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Effect = GetWorld()->SpawnActor<AActor>(FireEffectClass, GetActorLocation(), GetActorRotation(), SpawnParams);
	Effect->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("pelvis"));

	FTimerHandle TimerHandle_TestFire;
	FTimerDelegate TimerDel;
	TimerDel.BindUFunction(this, FName("StopTestFire"), Effect);
	GetWorldTimerManager().ClearTimer(TimerHandle_TestFire);
	GetWorldTimerManager().SetTimer(TimerHandle_TestFire, TimerDel, 5.0f, false);
}

/*
	Called when the client tells the server it has triggered
*/
void AProjectCharlieCharacter::ServerTestFire_Implementation()
{
	MulticastTestFire(); // Announce the action to all clients and self
}

/*
	Called when the server tells all clients that the action has triggered.
*/
void AProjectCharlieCharacter::MulticastTestFire_Implementation()
{
	// This ensures the local action is only called if this instance is not locally controlled.
	// Meaning if you triggered the action yourself and already showed the effects, it will
	// prevent them from being executed again on yourself
	if (!IsLocallyControlled())
	{
		LocalTestFire();
	}
}

bool AProjectCharlieCharacter::ServerTestFire_Validate()
{
	return true;
}

bool AProjectCharlieCharacter::MulticastTestFire_Validate()
{
	return true;
}

void AProjectCharlieCharacter::StopTestFire(AActor* Effect) 
{
	Effect->Destroy();
}

// End Networking Test Example
// ==============================================

void AProjectCharlieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AProjectCharlieCharacter, bIsFirstPerson, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AProjectCharlieCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AProjectCharlieCharacter, bIsAiming, COND_SkipOwner);
}