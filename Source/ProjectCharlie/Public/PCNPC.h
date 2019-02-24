// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCCharacter.h"
#include "PCNPC.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTCHARLIE_API APCNPC : public APCCharacter
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
};
