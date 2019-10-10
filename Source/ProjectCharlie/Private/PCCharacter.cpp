// Fill out your copyright notice in the Description page of Project Settings.

#include "PCCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Public/Interactable.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "PCWeaponBase.h"
#include "Sound/SoundCue.h"
#include "Runtime/Engine/Public/EngineGlobals.h"

//////////////////////////////////////////////////////////////////////////
// APCCharacter

APCCharacter::APCCharacter()
{
	/*
		Initialize Values/Flags/etc.
		----------------------------------------------------------------
	*/
	PrimaryActorTick.bCanEverTick = true;
	NetUpdateFrequency = 30.0f;
	MinNetUpdateFrequency = 15.0f;

	bIsWeaponEquipped = false;
	bIsSprinting = false;
	bIsMeleeAttacking = false;
	bIsReloading = false;
	bIsEquipping = false;
	bDoingSmoothAim = false;
	bDoingSmoothStopAimWeapon = false;
	bCanAim = false;
	bCanFire = false;
	bIsDead = false;
	bIsLeaningLeft = false;
	bIsLeaningRight = false;
	bIsPeaking = false;
	LeanAmount = 0;
	PeakAmount = 0;

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = MaxCrouchSpeed;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Set the socket to attach the weapon to
	WeaponAttachSocketName = "RightHand";

	/*
		Initialize Components
		----------------------------------------------------------------
	*/
	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Configure melee colliders
	LeftMeleeCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("LeftMeleeCollider"));
	LeftMeleeCollider->InitCapsuleSize(7.0f, 14.0f);
	LeftMeleeCollider->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	LeftMeleeCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	LeftMeleeCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	LeftMeleeCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);

	LeftMeleeCollider->CanCharacterStepUpOn = ECB_No;
	LeftMeleeCollider->SetShouldUpdatePhysicsVolume(false);
	LeftMeleeCollider->SetCanEverAffectNavigation(false);
	LeftMeleeCollider->bDynamicObstacle = false;
	LeftMeleeCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftMeleeCollider->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("LeftMelee"));
	LeftMeleeCollider->SetRelativeLocation(FVector(-2.7f, -3.4f, 1.6f));
	LeftMeleeCollider->SetRelativeRotation(FRotator(-68.4f, 77.6f, -77.3f));
	LeftMeleeCollider->OnComponentBeginOverlap.AddDynamic(this, &APCCharacter::OnLeftMeleeCollide);

	RightMeleeCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RightMeleeCollider"));
	RightMeleeCollider->InitCapsuleSize(7.0f, 14.0f);
	RightMeleeCollider->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	RightMeleeCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	RightMeleeCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	RightMeleeCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);

	RightMeleeCollider->CanCharacterStepUpOn = ECB_No;
	RightMeleeCollider->SetShouldUpdatePhysicsVolume(false);
	RightMeleeCollider->SetCanEverAffectNavigation(false);
	RightMeleeCollider->bDynamicObstacle = false;
	RightMeleeCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightMeleeCollider->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("RightMelee"));
	RightMeleeCollider->SetRelativeLocation(FVector(4.4f, 3.4f, -0.67f));
	RightMeleeCollider->SetRelativeRotation(FRotator(-68.39f, 77.59f, -91.35f));
	RightMeleeCollider->OnComponentBeginOverlap.AddDynamic(this, &APCCharacter::OnRightMeleeCollide);
}

/*
	BeginPlay
	======================================================================
	Called after creation but right at the beginning of playing before
	the game starts.
	======================================================================
*/
void APCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Get the Player's Anim Instance and Set to Class Variable
	AnimInstance = GetMesh()->GetAnimInstance();

	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = MaxCrouchSpeed;

	// If a primary weapon is specified, spawn it in the "holster"
	if (PrimaryWeaponClass)
	{
		FActorSpawnParameters PWSpawnParams;
		PWSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PrimaryWeapon = GetWorld()->SpawnActor<APCWeaponBase>(PrimaryWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, PWSpawnParams);
		PrimaryWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, PrimaryWeapon->GetHolsterSocketName());
	}

	// If a secondary weapon is specified, spawn it in the "holster"
	if (SecondaryWeaponClass)
	{
		FActorSpawnParameters SWSpawnParams;
		SWSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SecondaryWeapon = GetWorld()->SpawnActor<APCWeaponBase>(SecondaryWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SWSpawnParams);
		SecondaryWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SecondaryWeapon->GetHolsterSocketName());
	}
}

/*
	Tick
	======================================================================
	Called every game tick.
	Currently used for smooth transitions.
	======================================================================
*/
void APCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth ADS Weapon Position
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

	LeanAmount = FMath::FInterpTo(LeanAmount, GetLeanAmount(), DeltaTime, 5.0f);
	PeakAmount = FMath::FInterpTo(PeakAmount, GetPeakAmount(), DeltaTime, 5.0f);
}

/*
	StartSprint
	======================================================================
	Start sprinting. Raises max movement speed to sprint speed while
	sprint input is held.
	======================================================================
*/
void APCCharacter::StartSprint()
{
	if (bWasJumping || bIsCrouched || bIsAiming)
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = MaxSprintSpeed;
	bIsSprinting = true;
}

/*
	StopSprint
	======================================================================
	Stop sprinting. Lowers max movement speed back to default walk speed.
	======================================================================
*/
void APCCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	bIsSprinting = false;
}

/*
	ToggleCrouch
	======================================================================
	Toggles the crouching state for the character.
	======================================================================
*/
void APCCharacter::ToggleCrouch()
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

/*
	LeanLeft
	======================================================================
	Lean to the left. This is a toggle function, call again to stop
	leaning.
	======================================================================
*/
void APCCharacter::LeanLeft()
{
	bIsLeaningLeft = !bIsLeaningLeft;
}

/*
	LeanRight
	======================================================================
	Lean to the right. This is a toggle function, call again to stop
	leaning.
	======================================================================
*/
void APCCharacter::LeanRight()
{
	bIsLeaningRight = !bIsLeaningRight;
}

/*
	GetLeanAmount
	======================================================================
	Get the value to lean based on lean flags.
	======================================================================
*/
float APCCharacter::GetLeanAmount()
{
	if (bIsLeaningRight && bIsLeaningLeft)
	{
		return 0;
	}
	else if (bIsLeaningRight)
	{
		return MaxLean;
	}
	else if (bIsLeaningLeft)
	{
		return MaxLean * -1;
	}
	else
	{
		return 0;
	}
}

/*
	Peak
	======================================================================
	Peak upwards. This is a toggle function, call again to stop
	peaking.
	======================================================================
*/
void APCCharacter::Peak()
{
	bIsPeaking = !bIsPeaking;
}

/*
	GetPeakAmount
	======================================================================
	Get the value to peak based on peak flags.
	======================================================================
*/
float APCCharacter::GetPeakAmount()
{
	if (bIsPeaking)
	{
		return MaxPeak;
	}
	else
	{
		return 0;
	}
}

/*
	Interact
	======================================================================
	Attempt to interact with an interactable object. Sends a raycast out
	from the head, if it hits anything with the IInteractable interface
	it will call its OnInteract function.
	======================================================================
*/
void APCCharacter::Interact()
{
	FHitResult OutHit;
	FVector Start = GetMesh()->GetSocketLocation("head");
	FVector ForwardVector = GetMesh()->GetForwardVector();
	FVector End = ((ForwardVector * InteractDistance) + Start);
	FCollisionQueryParams CollisionParams;

	// Draw a debug line to show the raycast
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

/*
	ToggleEquipWeapon
	======================================================================
	Toggles equipping and unequipping the current weapon (gun).
	This function delegates execution for networking.
	The LocalToggleEquipWeapon function below contains the actual
	equip logic.
	======================================================================
*/
void APCCharacter::ToggleEquipWeapon()
{
	if (bWasJumping || bIsAiming)
	{
		return;
	}

	if (Role != ROLE_Authority)
	{
		LocalToggleEquipWeapon();
		ServerToggleEquipWeapon();
	}
	else if (Role == ROLE_Authority)
	{
		LocalToggleEquipWeapon();
		MulticastToggleEquipWeapon();
	}
}

void APCCharacter::ServerToggleEquipWeapon_Implementation()
{
	MulticastToggleEquipWeapon();
}

void APCCharacter::MulticastToggleEquipWeapon_Implementation()
{
	if (!IsLocallyControlled())
	{
		LocalToggleEquipWeapon();
	}
}

bool APCCharacter::ServerToggleEquipWeapon_Validate()
{
	return true;
}

bool APCCharacter::MulticastToggleEquipWeapon_Validate()
{
	return true;
}

void APCCharacter::LocalToggleEquipWeapon()
{
	if (bIsWeaponEquipped)
	{
		UnequipWeapon();
	}
	else
	{
		EquipWeapon(PrimaryWeapon);
	}
}

void APCCharacter::EquipWeapon(APCWeaponBase* Weapon)
{
	if (bIsAiming || bIsReloading || bIsMeleeAttacking || bIsEquipping)
	{
		return;
	}

	// Get the Player's Anim Instance and Set to Class Variable [Here to fix nullptr] - On Begin play sets to null for some reason
	// TODO Fix cause its .bad.exe
	AnimInstance = GetMesh()->GetAnimInstance();

	bIsWeaponEquipped = true;
	bCanAim = false;
	bCanFire = false;
	bIsEquipping = true;

	CurrentWeapon = Weapon;

	if (CurrentWeapon)
	{
		// If there is a Player Animation Instance, pass it to the Weapon (For Recoil Management, etc.), and play the equip animation
		if (AnimInstance)
		{
			CurrentWeapon->SetPlayerAnimInstance(AnimInstance);
			AnimInstance->PlaySlotAnimationAsDynamicMontage(Weapon->GetEquipAnimation(), "UpperBody", 0.25f, 0.25f, 1.0f, 1, -1.0f, 0.203f);
		}

		CurrentWeapon->SetOwner(this);

		CurrentWeaponMesh = CurrentWeapon->GetGunMeshComp();

		GetWorldTimerManager().ClearTimer(TimerHandle_EquipWeapon);
		GetWorldTimerManager().SetTimer(TimerHandle_EquipWeapon, this, &APCCharacter::PostEquipWeapon, 1.0f, false);
	}
}

// Called with some delay after equipping/unequipping a weapon.
void APCCharacter::PostEquipWeapon()
{
	bCanAim = true;
	bCanFire = true;
	bIsEquipping = false;

	if (CurrentWeapon)
	{
		CurrentWeapon->PlayWeaponRaiseSound();
	}
}

void APCCharacter::TakeCurrentWeaponInHands()
{
	CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
	CurrentWeapon->SetHipTransform();
}

void APCCharacter::UnequipWeapon()
{
	if (bIsMeleeAttacking || bIsEquipping || bIsAiming || bIsReloading)
	{
		return;
	}

	bIsWeaponEquipped = false;
	bCanAim = false;
	bCanFire = false;
	bIsEquipping = true;

	if (CurrentWeapon)
	{
		// Play the equip animation
		if (AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(CurrentWeapon->GetEquipAnimation(), "UpperBody", 0.25f, 0.25f, 1.0f);
		}

		CurrentWeapon->PlayWeaponLowerSound();
	}
}

void APCCharacter::PutCurrentWeaponInHolster()
{
	CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetHolsterSocketName());
	CurrentWeapon->SetZeroTransform();
	bIsEquipping = false;
}

void APCCharacter::BeginReload()
{
	if (bIsMeleeAttacking || bIsEquipping || bIsReloading)
	{
		return;
	}
	
	if (CurrentWeapon)
	{
		bIsReloading = true;
		bCanFire = false;

		// Play the reload animation
		if (AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(CurrentWeapon->GetReloadAnimation(), "UpperBody", 0.25f, 0.25f, 1.0f);
		}
	}
}

void APCCharacter::TakeMagazineInHands()
{
	if (CurrentWeapon && CurrentWeapon->GetCurrentMagazine())
	{
		CurrentWeapon->GetCurrentMagazine()->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, MagazineHandSocketName);
		CurrentWeapon->GetCurrentMagazine()->DoHandOffset();
		CurrentWeapon->PlayMagEjectSound();
	}
}

void APCCharacter::PutMagazineInWeapon()
{
	if (CurrentWeapon && CurrentWeapon->GetCurrentMagazine())
	{
		CurrentWeapon->GetCurrentMagazine()->AttachToComponent(CurrentWeaponMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetMagazineSocketName());
		CurrentWeapon->GetCurrentMagazine()->DoGunOffset();
		CurrentWeapon->PlayMagInsertSound();
	}
}

void APCCharacter::FinishReload()
{
	CurrentWeapon->Reload();
	bCanFire = true;
	bIsReloading = false;
}

/*
	Aim
	======================================================================
	Aim down the sights of the current gun.
	======================================================================
*/
void APCCharacter::StartAim()
{
	if (bIsSprinting || !bCanAim || bWasJumping || bIsMeleeAttacking)
	{
		return;
	}

	bIsAiming = true;
	bDoingSmoothAim = true;
	bDoingSmoothStopAimWeapon = false;
	
	GetCharacterMovement()->SetJumpAllowed(false);
	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;

	GetWorldTimerManager().ClearTimer(TimerHandle_ADS);
	GetWorldTimerManager().SetTimer(TimerHandle_ADS, this, &APCCharacter::PostSmoothAim, 0.8f, false);
}

// Called with some delay after the smooth aim has started
void APCCharacter::PostSmoothAim()
{
	bDoingSmoothAim = false;
}

/*
	StopAim
	======================================================================
	Stop aiming down the sights of the current gun.
	======================================================================
*/
void APCCharacter::StopAim()
{
	bIsAiming = false;
	bDoingSmoothAim = false;
	bDoingSmoothStopAimWeapon = true;

	GetCharacterMovement()->SetJumpAllowed(true);
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

	GetWorldTimerManager().ClearTimer(TimerHandle_StopADS);
	GetWorldTimerManager().SetTimer(TimerHandle_StopADS, this, &APCCharacter::PostStopSmoothAim, 0.8f, false);
}

// Called with some delay after smooth "un-aim"
void APCCharacter::PostStopSmoothAim()
{
	bDoingSmoothStopAimWeapon = false;
}

/*
	ChangeFiremode
	======================================================================
	Change the fire mode of the current weapon.
	Delegates execution to the weapon to cycle through available modes.
	======================================================================
*/
void APCCharacter::ChangeFiremode()
{
	if (CurrentWeapon && !bIsMeleeAttacking && !bIsReloading && !bIsEquipping)
	{
		CurrentWeapon->ChangeFiremode();
	}
}

/*
	StartFire
	======================================================================
	Begins to fire the current weapon/gun. Called when the fire input
	is pressed. Firing continues while the fire button is held.
	Delegates the actual firing to the weapon.
	======================================================================
*/
void APCCharacter::StartFire()
{
	if (!bIsWeaponEquipped || bIsSprinting || bWasJumping || !bCanFire || bIsMeleeAttacking || bIsReloading)
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire(); // Call the fire function on the weapon
	}
}

/*
	StopFire
	======================================================================
	Stops firing the current weapon/gun. Called when the fire input
	is released. Firing will continue until the fire button is released.
	Delegates the actual stopping of firing to the weapon.
	======================================================================
*/
void APCCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

/*
	StartMeleeAttack
	======================================================================
	Start to perform a melee attack with the current melee weapon,
	or fists.
	======================================================================
*/
void APCCharacter::StartMeleeAttack()
{
	if (bIsMeleeAttacking || bIsEquipping || bIsAiming || bIsReloading)
	{
		return;
	}

	bIsMeleeAttacking = true;

	if (!bIsWeaponEquipped && AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(UnarmedMeleeAnimation, "UpperBody", 0.25f, 0.25f, 1.0f);
	}
	else if (CurrentWeapon && AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(CurrentWeapon->GetMeleeAnimation(), "UpperBody", 0.25f, 0.25f, 1.0f);
	}
}

/*
	StopMeleeAttack
	======================================================================
	Stop performing a melee attack with the current melee weapon,
	or fists.
	======================================================================
*/
void APCCharacter::StopMeleeAttack()
{
	bIsMeleeAttacking = false;
}

/*
	StartLeftMeleeHit
	======================================================================
	Start checking for actual melee hits for the left melee collider.
	======================================================================
*/
void APCCharacter::StartLeftMeleeHit()
{
	LeftMeleeCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

/*
	StopLeftMeleeHit
	======================================================================
	Stop checking for actual melee hits for the left melee collider.
	======================================================================
*/
void APCCharacter::StopLeftMeleeHit()
{
	LeftMeleeCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/*
	OnLeftMeleeCollide
	======================================================================
	Called when the left melee collider overlaps an object.
	======================================================================
*/
void APCCharacter::OnLeftMeleeCollide(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this)
	{
		APCCharacter::MeleeHitCast("LeftMelee");
	}
}

/*
	StartRightMeleeHit
	======================================================================
	Start checking for actual melee hits for the right melee collider.
	======================================================================
*/
void APCCharacter::StartRightMeleeHit()
{
	RightMeleeCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

/*
	StopRightMeleeHit
	======================================================================
	Stop checking for actual melee hits for the right melee collider.
	======================================================================
*/
void APCCharacter::StopRightMeleeHit()
{
	RightMeleeCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/*
	OnRightMeleeCollide
	======================================================================
	Called when the right melee collider overlaps an object.
	======================================================================
*/
void APCCharacter::OnRightMeleeCollide(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this)
	{
		APCCharacter::MeleeHitCast("RightMelee");
	}
}

/*
	MeleeHitCast
	======================================================================
	Perform an actual melee hit cast from the socket with the given name.
	======================================================================
*/
void APCCharacter::MeleeHitCast(FName SocketName)
{
	FHitResult OutHit;
	FVector Start = GetMesh()->GetSocketLocation(SocketName);
	FRotator SocketRotation = GetMesh()->GetSocketRotation(SocketName);
	float TraceDistance = 100.0f;
	FVector End = SocketRotation.Add(0.0f, 150.0f, 0.0f).RotateVector(Start) * TraceDistance;
	ECollisionChannel CollisionChannel = ECC_Visibility;
	FCollisionQueryParams CollisionParams(FName("Melee Hit Trace"), false, this);

	bool IsHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, CollisionChannel, CollisionParams);

	if (IsHit)
	{
		AActor* HitActor = Cast<AActor>(OutHit.Actor);
		if (HitActor)
		{
			UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, MeleeHitSound, OutHit.ImpactPoint, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
			UGameplayStatics::ApplyPointDamage(HitActor, 100.0f, GetActorForwardVector(), OutHit, nullptr, this, nullptr);
		}
	}
}

void APCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APCCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(APCCharacter, bIsAiming, COND_SkipOwner);
}

