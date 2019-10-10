// Fill out your copyright notice in the Description page of Project Settings.

#include "PCPlayer.h"
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

APCPlayer::APCPlayer()
{
	/*
		Initialize Values/Flags/etc.
		----------------------------------------------------------------
	*/
	PrimaryActorTick.bCanEverTick = true;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	BaseTurnRate = 1.0f;
	BaseLookUpRate = 1.0f;

	bIsFirstPerson = false;
	bDoingSmoothStopAimCamera = false;
	bAimLock = false;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	FPCameraDefaultRotation = FRotator(0.0f, 90.0f, -90.0f);

	FollowCameraDefaultLocation = FVector(0.0f, 0.0f, 0.0f);
	FollowCameraDefaultRotation = FRotator(0.0f, 0.0f, 0.0f);
	FollowCameraAimLocation = FVector(58.177422f, 50.657249f, 24.756479f);
	FollowCameraAimRotation = FRotator(0.0f, 0.0f, -4.554169f);
	CameraBoomDefaultLength = 300.0f;
	CameraBoomAimLength = 150.0f;

	/*
		Initialize Components
		----------------------------------------------------------------
	*/

	// Create the camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(GetMesh(), TEXT("FPCameraSocket"), EAttachLocation::SnapToTargetIncludingScale, true);

	// Create the follow camera component
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	
	// Create the first person camera component
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FPCamera->SetupAttachment(GetMesh(), TEXT("FPCameraSocket"));
	FPCamera->bUsePawnControlRotation = true;
	FPCamera->SetAutoActivate(false);
}

/*
	BeginPlay
	======================================================================
	Called after creation but right at the beginning of playing before
	the game starts.
	======================================================================
*/
void APCPlayer::BeginPlay()
{
	Super::BeginPlay();

	FPCamera->SetRelativeLocation(FPCameraDefaultLocation);
	FPCamera->SetRelativeRotation(FPCameraDefaultRotation);

	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	CameraBoom->TargetArmLength = CameraBoomDefaultLength; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
}

/*
	Tick
	======================================================================
	Called every game tick.
	Currently used for smooth transitions, etc.
	======================================================================
*/
void APCPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth ADS Camera Position
	if (bIsFirstPerson && bIsWeaponEquipped && bIsAiming && bDoingSmoothAim)
	{
		FPCamera->SetRelativeLocation(FMath::VInterpTo(FPCamera->RelativeLocation, CurrentWeapon->GetADSOffset(), DeltaTime, CurrentWeapon->GetAimSpeed()));
	}
	else if (bIsFirstPerson && bIsWeaponEquipped && !bIsAiming && bDoingSmoothStopAimCamera)
	{
		FPCamera->SetRelativeLocation(FMath::VInterpTo(FPCamera->RelativeLocation, FPCameraDefaultLocation, DeltaTime, CurrentWeapon->GetAimSpeed()));
	}

	if (!bIsFirstPerson && bIsWeaponEquipped && bIsAiming)
	{
		FollowCamera->SetRelativeLocation(FMath::VInterpTo(FollowCamera->RelativeLocation, FollowCameraAimLocation, DeltaTime, 4.0f));
		FollowCamera->SetRelativeRotation(FMath::RInterpTo(FollowCamera->RelativeRotation, FollowCameraAimRotation, DeltaTime, 4.0f));
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, CameraBoomAimLength, DeltaTime, 4.0f);
	}
	else if (!bIsFirstPerson && bIsWeaponEquipped && !bIsAiming)
	{
		FollowCamera->SetRelativeLocation(FMath::VInterpTo(FollowCamera->RelativeLocation, FollowCameraDefaultLocation, DeltaTime, 4.0f));
		FollowCamera->SetRelativeRotation(FMath::RInterpTo(FollowCamera->RelativeRotation, FollowCameraDefaultRotation, DeltaTime, 4.0f));
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, CameraBoomDefaultLength, DeltaTime, 4.0f);
	}
}

/*
	SetupPlayerInputComponent
	======================================================================
	Setup the input component to bind input actions to functions.
	======================================================================
*/
void APCPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	/*
		Movement input bindings
	*/
	PlayerInputComponent->BindAxis("MoveForward", this, &APCPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APCPlayer::MoveRight);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &APCPlayer::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &APCPlayer::StopSprint);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &APCPlayer::ToggleCrouch);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APCPlayer::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &APCPlayer::StopJumping);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APCPlayer::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &APCPlayer::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APCPlayer::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &APCPlayer::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &APCPlayer::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &APCPlayer::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &APCPlayer::OnResetVR);

	/*
		Action input bindings
	*/
	PlayerInputComponent->BindAction("ChangeView", IE_Pressed, this, &APCPlayer::ToggleView);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &APCPlayer::Interact);
	PlayerInputComponent->BindAction("LeanLeft", IE_Pressed, this, &APCPlayer::LeanLeft);
	PlayerInputComponent->BindAction("LeanLeft", IE_Released, this, &APCPlayer::LeanLeft);
	PlayerInputComponent->BindAction("LeanRight", IE_Pressed, this, &APCPlayer::LeanRight);
	PlayerInputComponent->BindAction("LeanRight", IE_Released, this, &APCPlayer::LeanRight);
	PlayerInputComponent->BindAction("Peak", IE_Pressed, this, &APCPlayer::Peak);
	PlayerInputComponent->BindAction("Peak", IE_Released, this, &APCPlayer::Peak);

	// Weapon bindings
	//PlayerInputComponent->BindAction("EquipWeapon", IE_Pressed, this, &APCPlayer::ToggleEquipWeapon);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &APCPlayer::StartAim);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &APCPlayer::StopAim);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APCPlayer::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &APCPlayer::StopFire);
	PlayerInputComponent->BindAction("Firemode", IE_Released, this, &APCPlayer::ChangeFiremode);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &APCPlayer::BeginReload);

	// Melee bindings
	PlayerInputComponent->BindAction("Melee", IE_Pressed, this, &APCPlayer::StartMeleeAttack);

	// Test for networking, leave for now
	PlayerInputComponent->BindAction("Test", IE_Pressed, this, &APCPlayer::TestFire);
}

void APCPlayer::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void APCPlayer::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void APCPlayer::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void APCPlayer::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APCPlayer::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

/*
	MoveForward
	======================================================================
	Adds input movement to our character for the forward/backward axis.
	======================================================================
*/
void APCPlayer::MoveForward(float Value)
{
	ForwardAxisValue = Value;

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

/*
	MoveRight
	======================================================================
	Adds input movement to our character for the right/left axis.
	======================================================================
*/
void APCPlayer::MoveRight(float Value)
{
	RightAxisValue = Value;

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

/*
	ToggleView
	======================================================================
	Toggles between first and third person by switching cameras with a
	brief fade in/out. Also changes how input affects movement/rotation
	in each view.
	======================================================================
*/
void APCPlayer::ToggleView()
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

/*
	SetFirstPerson
	======================================================================
	Set the view to first person.
	======================================================================
*/
void APCPlayer::SetFirstPerson()
{
	FPCamera->SetActive(true);
	FollowCamera->SetActive(false);

	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 2500.0f, 0.0f);
}

/*
	SetThirdPerson
	======================================================================
	Sets the view to third person.
	======================================================================
*/
void APCPlayer::SetThirdPerson()
{
	CameraBoom->TargetArmLength = CameraBoomDefaultLength;
	FPCamera->SetActive(false, true);
	FollowCamera->SetActive(true);

	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

/*
	Aim
	======================================================================
	Aim down the sights of the current gun.
	If in first person, the camera is moved to the ADS position, 
	otherwise the rotation input is changed to rotate the character
	towards their aim.
	======================================================================
*/
void APCPlayer::StartAim()
{
	Super::StartAim();

	bDoingSmoothStopAimCamera = false;

	if (!bIsFirstPerson)
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	FName RearSightName = "RearSight";
	FPCamera->AttachTo(CurrentWeaponMesh, RearSightName, EAttachLocation::KeepWorldPosition);
}

void APCPlayer::PostSmoothAim()
{
	Super::PostSmoothAim();
}

/*
	StopAim
	======================================================================
	Stop aiming down the sights of the current gun.
	If in first person, the camera is moved back to the default position,
	otherwise the rotation input is changed back to the default third
	person input rotation.
	======================================================================
*/
void APCPlayer::StopAim()
{
	if (!bAimLock)
	{
		Super::StopAim();

		bDoingSmoothStopAimCamera = true;

		if (!bIsFirstPerson)
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}

		FName HeadSocketName = "head";
		FPCamera->AttachTo(GetMesh(), HeadSocketName, EAttachLocation::KeepWorldPosition);
	}
}

// Called with some delay after smooth "un-aim"
void APCPlayer::PostStopSmoothAim()
{
	Super::PostStopSmoothAim();

	bDoingSmoothStopAimCamera = false;
}

/*
	Interact
	======================================================================
	Attempt to interact with an interactable object. Sends a raycast out
	from the head, if it hits anything with the IInteractable interface
	it will call its OnInteract function.
	======================================================================
*/
void APCPlayer::Interact()
{
	FHitResult OutHit;
	FVector Start = FPCamera->GetComponentLocation();
	FVector ForwardVector = FPCamera->GetForwardVector();
	FVector End = ((ForwardVector * InteractDistance) + Start);
	FCollisionQueryParams CollisionParams;

	// Draw a debug line to show the raycast
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


// Networking Test Example
// ==============================================

/*
	This is the function you should call to initiate the action every time.
	Don't call the network implementations directly.
*/
void APCPlayer::TestFire()
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
void APCPlayer::LocalTestFire()
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
void APCPlayer::ServerTestFire_Implementation()
{
	MulticastTestFire(); // Announce the action to all clients and self
}

/*
	Called when the server tells all clients that the action has triggered.
*/
void APCPlayer::MulticastTestFire_Implementation()
{
	// This ensures the local action is only called if this instance is not locally controlled.
	// Meaning if you triggered the action yourself and already showed the effects, it will
	// prevent them from being executed again on yourself
	if (!IsLocallyControlled())
	{
		LocalTestFire();
	}
}

bool APCPlayer::ServerTestFire_Validate()
{
	return true;
}

bool APCPlayer::MulticastTestFire_Validate()
{
	return true;
}

void APCPlayer::StopTestFire(AActor* Effect)
{
	Effect->Destroy();
}

// End Networking Test Example
// ==============================================

void APCPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APCPlayer, bIsFirstPerson, COND_SkipOwner);
}