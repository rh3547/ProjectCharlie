// Fill out your copyright notice in the Description page of Project Settings.

#include "PCWeaponBase.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

//Weapon Debug Command
static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(TEXT("PC.DebugWeapons"), DebugWeaponDrawing, TEXT("Draw Debug Lines for Weapons"), ECVF_Cheat);

// Sets default values
APCWeaponBase::APCWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create a Skeletal Mesh Component for the Weapon
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	
	//Default MuzzleSocket Name
	MuzzleSocketName = "Muzzle";

	//Make Root the Mesh Component
	RootComponent = MeshComp;

	AnimInstance = nullptr;

	ShotCounter = 0;
}

void APCWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (FireModes.Num() != 0) 
	{
		CurrentFireMode = FireModes[0];
	}
	else {
		CurrentFireMode = EFiremode::SEMI_AUTO;
	}
	

	TimeBetweenShots = 60 / RateOfFire;
}

USkeletalMeshComponent* APCWeaponBase::GetGunMeshComp()
{
	return MeshComp;
}

void APCWeaponBase::Fire()
{
	if (((CurrentFireMode == EFiremode::SEMI_AUTO || CurrentFireMode == EFiremode::SINGLE_ACTION) && ShotCounter < 1) || (CurrentFireMode == EFiremode::FULLY_SEMI_AUTO) || (CurrentFireMode == EFiremode::THREE_ROUND_BURST && ShotCounter < 3)) {

		AActor* MyOwner = GetOwner(); //need to setup in editor PlayerPawn - Set Owner in BP Implementation

		if (MyOwner && ProjectileClass)
		{
			FVector EyeLocation;
			FRotator EyeRotation;
			MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

			FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName); // "Muzzle" with base pack
			FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocketName); //was #out

			//Spawn Even If Collisions
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//Spawn a Projectile
			GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams); //was #eyerotation
			
			//Increment ShotCounter by 1
			ShotCounter++; 

			//Play the Recoil Animation
			if (FireAnimation)
			{
				AnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, "Shoulders", 0.0f);
			}

		}

		if (DebugWeaponDrawing > 0)
		{
			//Place DEBUG statements here.
		}

		//Play other effects, such as muzzle flash, sound, etc.
		PlayFireEffects();

		LastFireTime = GetWorld()->TimeSeconds; //Set the last time we fired our weapon (used for fire rate check)
	}
}

void APCWeaponBase::StartFire()
{
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

void APCWeaponBase::SetPlayerAnimInstance(UAnimInstance* PlayerAnimInstance)
{
	AnimInstance = PlayerAnimInstance;
}

void APCWeaponBase::PlayFireEffects() {
	//Play Muzzle Effect
	if (MuzzleEffect) //prevent crash if unassigned
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
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

void APCWeaponBase::SetHipTransform()
{
	RootComponent->SetRelativeLocationAndRotation(HipLocation, HipRotation);
}

void APCWeaponBase::SetAimTransform()
{
	RootComponent->SetRelativeLocationAndRotation(AimLocation, AimRotation);
}
