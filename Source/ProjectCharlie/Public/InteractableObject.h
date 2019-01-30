// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractableObject.generated.h"

UCLASS()
class PROJECTCHARLIE_API AInteractableObject : public AActor, public IInteractable
{
	GENERATED_BODY()

protected:
UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
    USceneComponent* SceneComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:	
	// Sets default values for this actor's properties
	AInteractableObject();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** IInteractable interface function */
	bool OnInteract();
	virtual bool OnInteract_Implementation() override;
};
