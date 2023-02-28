// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPawn.h"

// Sets default values
AVRPawn::AVRPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

bool AVRPawn::ShouldStopRelaxed(float DeltaTime)
{
	m_unrelaxedDuration += DeltaTime;
	m_unrelaxedFrames.Enqueue({DeltaTime, GetWorld()->GetTimeSeconds()});

	while (GetWorld()->GetTimeSeconds() - m_unrelaxedFrames.Peek()->timestamp > 5.f)
	{
		FFrameStep frameStep;
		m_unrelaxedFrames.Dequeue(frameStep);
		m_unrelaxedDuration -= frameStep.dt;
	}

	return m_unrelaxedDuration > 2.5f;
}

