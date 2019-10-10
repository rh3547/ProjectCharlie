// Fill out your copyright notice in the Description page of Project Settings.

#include "PCWeaponBase.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "PCProjectileBase.h"

#include "PCCharacter.h"

//Weapon Debug Command
static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(TEXT("PC.DebugWeapons"), DebugWeaponDrawing, TEXT("Draw Debug Lines for Weapons"), ECVF_Cheat);

// Sets default values
APCWeaponBase::APCWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//Create a Skeletal Mesh Component for the Weapon
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));

	//Default MuzzleSocket Name
	MuzzleSocketName = "Muzzle";
	ShellEjectSocketName = "ShellEject";

	//Make Root the Mesh Component
	RootComponent = MeshComp;

	PlayerAnimInstance = nullptr;
	AnimInstance = nullptr;

	ShotCounter = 0;

	// Create audio componenent for playing weapon sounds
	FireAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudioComponent"));
	FireAudioComponent->bAutoActivate = false;

	ShellEjectAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ShellEjectAudioComponent"));
	ShellEjectAudioComponent->bAutoActivate = false;

	MagazineAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MagazineAudioComponent"));
	MagazineAudioComponent->bAutoActivate = false;

	MagazineEjectAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MagazineEjectAudioComponent"));
	MagazineEjectAudioComponent->bAutoActivate = false;

	MagazineInsertAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MagazineInsertAudioComponent"));
	MagazineInsertAudioComponent->bAutoActivate = false;

	WeaponRaiseAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("WeaponRaiseAudioComponent"));
	WeaponRaiseAudioComponent->bAutoActivate = false;

	WeaponLowerAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("WeaponLowerAudioComponent"));
	WeaponLowerAudioComponent->bAutoActivate = false;
}

void APCWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	AnimInstance = MeshComp->GetAnimInstance();

	if (FireModes.Num() != 0)
	{
		CurrentFireMode = FireModes[0];
	}
	else {
		CurrentFireMode = EFiremode::SEMI_AUTO;
	}

	TimeBetweenShots = 60 / RateOfFire;

	FireAudioComponent->AttachTo(MeshComp, MuzzleSocketName);
	ShellEjectAudioComponent->AttachTo(MeshComp, ShellEjectSocketName);
	MagazineAudioComponent->AttachTo(MeshComp, MagazineSocketName);
	MagazineEjectAudioComponent->AttachTo(MeshComp, MagazineSocketName);
	MagazineInsertAudioComponent->AttachTo(MeshComp, MagazineSocketName);
	WeaponRaiseAudioComponent->AttachTo(MeshComp, MagazineSocketName);
	WeaponLowerAudioComponent->AttachTo(MeshComp, MagazineSocketName);

	if (FireSound->IsValidLowLevelFast())
	{
		FireAudioComponent->SetSound(FireSound);
	}

	if (ShellEjectSound->IsValidLowLevelFast())
	{
		ShellEjectAudioComponent->SetSound(ShellEjectSound);
	}

	if (EmptyMagSound->IsValidLowLevelFast())
	{
		MagazineAudioComponent->SetSound(EmptyMagSound);
	}
	
	if (MagEjectSound->IsValidLowLevelFast())
	{
		MagazineEjectAudioComponent->SetSound(MagEjectSound);
	}
	
	if (MagInsertSound->IsValidLowLevelFast())
	{
		MagazineInsertAudioComponent->SetSound(MagInsertSound);
	}

	if (WeaponRaiseSound->IsValidLowLevelFast())
	{
		WeaponRaiseAudioComponent->SetSound(WeaponRaiseSound);
	}

	if (WeaponLowerSound->IsValidLowLevelFast())
	{
		WeaponLowerAudioComponent->SetSound(WeaponLowerSound);
	}

	if (MagazineClass)
	{
		FActorSpawnParameters SWSpawnParams;
		SWSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentMagazine = GetWorld()->SpawnActor<APCMagazineBase>(MagazineClass, FVector::ZeroVector, FRotator::ZeroRotator, SWSpawnParams);
		CurrentMagazine->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, MagazineSocketName);
		CurrentMagazine->DoGunOffset();
	}
	else
	{
		CurrentMagazine = nullptr;
	}
}

USkeletalMeshComponent* APCWeaponBase::GetGunMeshComp()
{
	return MeshComp;
}

void APCWeaponBase::Fire()
{
	// Shots remaining check. This one is to ensure firing stops if in the middle of automatic fire
	if (CurrentMagazine == nullptr || CurrentMagazine->IsEmpty())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
		ShotCounter = 0;
		return;
	}

	if (((CurrentFireMode == EFiremode::SEMI_AUTO || CurrentFireMode == EFiremode::SINGLE_ACTION) && ShotCounter < 1) || (CurrentFireMode == EFiremode::FULLY_SEMI_AUTO) || (CurrentFireMode == EFiremode::THREE_ROUND_BURST && ShotCounter < 3))
	{
		AActor* MyOwner = GetOwner(); // Need to setup in editor PlayerPawn - Set Owner in BP Implementation

		if (MyOwner && CurrentMagazine->ProjectileClass)
		{
			FVector EyeLocation;
			FRotator EyeRotation;
			MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

			FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
			FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocketName);

			// Spawn Even If Collisions
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Instigator = Cast<APawn>(MyOwner);

			// Spawn a Projectile
			AActor* ProjectileActor = GetWorld()->SpawnActor<AActor>(CurrentMagazine->ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);
			APCProjectileBase* ProjectileBase = Cast<APCProjectileBase>(ProjectileActor);
			ProjectileBase->SetOrigin(MeshComp->GetSocketLocation(MuzzleSocketName));

			// Increment ShotCounter by 1
			ShotCounter++;

			// Handle ammo use
			CurrentMagazine->UnloadOneRound();
		}

		if (DebugWeaponDrawing > 0)
		{
			//Place DEBUG statements here.
		}

		// Play other effects, such as muzzle flash, sound, etc.
		PlayFireEffects();

		LastFireTime = GetWorld()->TimeSeconds; //Set the last time we fired our weapon (used for fire rate check)
	}
}

void APCWeaponBase::StartFire()
{
	// Shots remaining check. This one is to ensure the empty sound is played only once.
	if (CurrentMagazine == nullptr || CurrentMagazine->IsEmpty())
	{
		if (MagazineAudioComponent)
		{
			MagazineAudioComponent->Play();
		}

		return;
	}

	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f); //Clamp to 0+

	//Calls the Fire() function with a delay based on the time last fired (to correspond with fire rate).
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &APCWeaponBase::Fire, TimeBetweenShots, true, FirstDelay); //Full Auto

}

void APCWeaponBase::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
	ShotCounter = 0;
}

void APCWeaponBase::ChangeFiremode()
{
	if (FireModes.Num() <= 1) { //If only one mode or zero, return with current mode;
		return;
	}

	for (int i = 0; i < FireModes.Num(); i++) {
		if (CurrentFireMode == FireModes[i] && i < FireModes.Num()-1) { //If found fire mode, increment by 1
			CurrentFireMode = FireModes[i + 1];
			break;
		}
		else if(i == (FireModes.Num() - 1)) { //If at last index, set to first fire mode
			CurrentFireMode = FireModes[0];
			break;
		}
	}
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("This is an on screen message!"));
}

TArray<EFiremode> APCWeaponBase::GetFireModes()
{
	return FireModes;
}

void APCWeaponBase::Reload()
{
	if (CurrentMagazine)
	{
		CurrentMagazine->Refill();
	}
}

void APCWeaponBase::SetPlayerAnimInstance(UAnimInstance* InAnimInstance)
{
	PlayerAnimInstance = InAnimInstance;
}

void APCWeaponBase::PlayFireEffects() {
	//Play Muzzle Effect
	if (MuzzleEffect) //prevent crash if unassigned
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	//Play the Recoil Animation
	if (FireAnimation)
	{
		PlayerAnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, "Shoulders", 0.0f);
	}

	if (SingleFireAnimation && CurrentFireMode == EFiremode::SEMI_AUTO)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(SingleFireAnimation, "Fire", 0.0f);
	}
	else if (AutoFireAnimation && CurrentFireMode == EFiremode::FULLY_SEMI_AUTO)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(AutoFireAnimation, "Fire", 0.0f);
	}

	if (FireAudioComponent)
	{
		FireAudioComponent->Play();
	}

	//Camera Shake
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void APCWeaponBase::PlayShellEjectEffect()
{
	if (ShellEjectEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(ShellEjectEffect, MeshComp, ShellEjectSocketName);
	}

	if (ShellEjectAudioComponent)
	{
		ShellEjectAudioComponent->Play();
	}
}

void APCWeaponBase::SetAimTransform()
{
	RootComponent->SetRelativeLocationAndRotation(AimLocation, AimRotation);
}

FVector APCWeaponBase::GetHipLocation()
{
	return HipLocation;
}

FVector APCWeaponBase::GetAimLocation()
{
	return AimLocation;
}

FRotator APCWeaponBase::GetHipRotation()
{
	return HipRotation;
}

FRotator APCWeaponBase::GetAimRotation()
{
	return AimRotation;
}

FVector APCWeaponBase::GetADSOffset()
{
	return ADSOffsetVector;
}

float APCWeaponBase::GetAimSpeed()
{
	return AimSpeed;
}

void APCWeaponBase::SetHipTransform()
{
	RootComponent->SetRelativeLocationAndRotation(HipLocation, HipRotation);
}

void APCWeaponBase::SetZeroTransform()
{
	RootComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
}

UAnimSequence* APCWeaponBase::GetEquipAnimation()
{
	return EquipAnimation;
}

UAnimSequence* APCWeaponBase::GetReloadAnimation()
{
	return ReloadAnimation;
}

UAnimSequence* APCWeaponBase::GetMeleeAnimation()
{
	return MeleeAnimation;
}

FName APCWeaponBase::GetHolsterSocketName()
{
	return HolsterSocketName;
}

FName APCWeaponBase::GetMagazineSocketName()
{
	return MagazineSocketName;
}

APCMagazineBase* APCWeaponBase::GetCurrentMagazine()
{
	return CurrentMagazine;
}

void APCWeaponBase::PlayMagEjectSound()
{
	if (MagazineEjectAudioComponent)
	{
		MagazineEjectAudioComponent->Play();
	}
}

void APCWeaponBase::PlayMagInsertSound()
{
	if (MagazineInsertAudioComponent)
	{
		MagazineInsertAudioComponent->Play();
	}
}

void APCWeaponBase::PlayWeaponRaiseSound()
{
	if (WeaponRaiseAudioComponent)
	{
		WeaponRaiseAudioComponent->Play();
	}
}

void APCWeaponBase::PlayWeaponLowerSound()
{
	if (WeaponLowerAudioComponent)
	{
		WeaponLowerAudioComponent->Play();
	}
}
