// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRPawn.generated.h"

struct FFrameStep
{
	float dt;
	float timestamp;
};

UCLASS()
class VR_TEST_API AVRPawn : public APawn
{
	GENERATED_BODY()

	TQueue<FFrameStep> m_unrelaxedFrames;
	float m_unrelaxedDuration = 0.f;

public:
	// Sets default values for this pawn's properties
	AVRPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	bool ShouldStopRelaxed(float DeltaTime);
};
