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
}

void APCWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

USkeletalMeshComponent* APCWeaponBase::GetGunMeshComp()
{
	return MeshComp;
}

void APCWeaponBase::Fire()
{
	AActor* MyOwner = GetOwner(); //need to setup in editor PlayerPawn - Set Owner in BP Implementation

	if (MyOwner && ProjectileClass)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName); // "Muzzle" with base pack
		FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocketName); //was #out

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams); //was #eyerotation

		//Play Fire (Recoil) Animation on the Player's Animation Controller "AnimInstance"
		if (FireAnimation) 
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, "Shoulders", 0.0f);
		}
		
	}

	if (DebugWeaponDrawing > 0)
	{
		//Place DEBUG statements here.
	}

	PlayFireEffects();

	LastFireTime = GetWorld()->TimeSeconds; //Set the last time we fired our weapon (used for fire rate check)
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
