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
	bIsRifleEquipped = false;
	bIsSprinting = false;
	bDoingSmoothAim = false;
	bDoingSmoothStopAimWeapon = false;
	bCanAim = false;
	bIsMeleeEquipped = false;
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
		EquipWeapon();
	}
}

void APCCharacter::EquipWeapon()
{
	bIsWeaponEquipped = true;
	bIsRifleEquipped = true;
	bCanAim = false;

	//Spawn A Default Weapon
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentWeapon = GetWorld()->SpawnActor<APCWeaponBase>(PrimaryWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (CurrentWeapon)
	{
		// Pass the Player's Animation Instance to the Weapon (For Recoil Management, etc.)
		if (AnimInstance)
		{
			CurrentWeapon->SetPlayerAnimInstance(AnimInstance);
		}

		CurrentWeapon->SetOwner(this);
		CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);

		CurrentWeapon->SetHipTransform();

		CurrentWeaponMesh = CurrentWeapon->GetGunMeshComp();

		if (AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(CurrentWeapon->GetEquipAnimation(), "UpperBody", 0.0f);
		}

		GetWorldTimerManager().ClearTimer(TimerHandle_EquipWeapon);
		GetWorldTimerManager().SetTimer(TimerHandle_EquipWeapon, this, &APCCharacter::PostEquipWeapon, 1.5f, false);
	}
}

// Called with some delay after equipping/unequipping a weapon.
void APCCharacter::PostEquipWeapon()
{
	bCanAim = true;
}

void APCCharacter::UnequipWeapon()
{
	bIsWeaponEquipped = false;
	bIsRifleEquipped = false;
	bCanAim = false;

	CurrentWeaponMesh = nullptr;
	CurrentWeapon->Destroy();
}

/*
	Aim
	======================================================================
	Aim down the sights of the current gun.
	======================================================================
*/
void APCCharacter::StartAim()
{
	if (bIsSprinting || !bCanAim || bWasJumping)
	{
		return;
	}

	bIsAiming = true;
	bDoingSmoothAim = true;
	bDoingSmoothStopAimWeapon = false;
	
	GetCharacterMovement()->SetJumpAllowed(false);
	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;

	GetWorldTimerManager().ClearTimer(TimerHandle_ADS);
	GetWorldTimerManager().SetTimer(TimerHandle_ADS, this, &APCCharacter::PostSmoothAim, 2.0f, false);
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
	GetWorldTimerManager().SetTimer(TimerHandle_StopADS, this, &APCCharacter::PostStopSmoothAim, 2.0f, false);
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
	if (CurrentWeapon) {
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
	if (!bIsWeaponEquipped || bIsSprinting || bWasJumping)
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
	ToggleEquipMelee
	======================================================================
	Toggles melee mode. If no melee weapon is "owned", fists will be
	used. If the character has a melee weapon, then that weapon will be
	equipped.
	======================================================================
*/
void APCCharacter::ToggleEquipMelee()
{
	if (bIsMeleeEquipped)
	{
		UnequipMelee();
	}
	else
	{
		EquipMelee();
	}
}

void APCCharacter::EquipMelee()
{
	bIsMeleeEquipped = true;
	bIsWeaponEquipped = false;
	bIsRifleEquipped = false;
}

void APCCharacter::UnequipMelee()
{
	bIsMeleeEquipped = false;
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

}

/*
	StopMeleeAttack
	======================================================================
	Stop performing a melee attack with the current melee weapon,
	or fists.
	======================================================================
*/
void APCCharacter::StopMeleeAttack() {

}

void APCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APCCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(APCCharacter, bIsAiming, COND_SkipOwner);
}

