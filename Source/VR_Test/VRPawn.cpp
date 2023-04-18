// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPawn.h"

#include "AntiAliasedTextWidgetComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

// Sets default values
AVRPawn::AVRPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	
	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollider"));
	SphereCollider->SetupAttachment(RootComponent);

	TextWidget = CreateDefaultSubobject<UAntiAliasedTextWidgetComponent>("TextWidget");
	TextWidget->SetupAttachment(RootComponent);
}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();

	for (int i = 0; i < meditationQueueSize; ++i)
		m_meditationValues.PushFirst(0);

	SetInterpDuration(interpDuration);
	m_sumSize = meditationQueueSize - 1;
	m_targetZVelocity = fallVelocity;

	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AVRPawn::Landed);
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &AVRPawn::BecomeAirborne);
}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (m_interpTime <= 1.f)
		relaxationValue = FMath::Lerp(m_prevAvg, m_currAvg, m_interpTime);
	UE_LOG(LogTemp, Warning, TEXT("%f"), relaxationValue);
	m_interpTime += DeltaTime;
		
	if (ShouldChangeState())
	{
		bRelaxed = !bRelaxed;
		m_targetZVelocity = bRelaxed ? riseVelocity : fallVelocity;

		//if(bRelaxed && !GetCharacterMovement()->IsFalling())
		//	LaunchCharacter(FVector(0.f, 0.f, DeltaTime), false, false);
	}
	
	UpdateUpVelocity(DeltaTime);
}

bool AVRPawn::ReachedTargetVelocity()
{
	return velocity.Z != m_targetZVelocity;
}

void AVRPawn::UpdateUpVelocity(float DeltaSeconds)
{
	// Ignore if falling but already on ground
	if (!bRelaxed && bGrounded)
		return;
	
	// Interpolate the velocity towards the target velocity
	if (ReachedTargetVelocity())
		velocity.Z = FMath::FInterpConstantTo(velocity.Z, m_targetZVelocity, DeltaSeconds, m_interpSpeed);

	AddActorWorldOffset(DeltaSeconds * velocity);
}

void AVRPawn::Landed(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	bGrounded = true;
	// reset velocity adn target velocity
	m_targetZVelocity = velocity.Z = 0.f;
	UE_LOG(LogTemp, Log, TEXT("landed callback!!!"))
}

void AVRPawn::BecomeAirborne(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	bGrounded = false;
	UE_LOG(LogTemp, Log, TEXT("ariborne callback!!!"))
}

void AVRPawn::SetInterpDuration(float value)
{
	interpDuration = value;
	m_interpSpeed = (riseVelocity - fallVelocity) / interpDuration;
}

bool AVRPawn::ShouldChangeState()
{
	if (bRelaxed && relaxationValue < 50.f || !bRelaxed && relaxationValue >= 50.f)
	{
		int unrelaxedValueCount = 0;
	
		for (int i = 0; i < meditationQueueSize; ++i)
			if (m_meditationValues[i] < 50.f)
				unrelaxedValueCount++;

		const float unrelaxedRate = static_cast<float>(unrelaxedValueCount) / static_cast<float>(meditationQueueSize);

		return bRelaxed && unrelaxedRate >= oppositeStateThreshold
			|| !bRelaxed && 1 - unrelaxedRate >= oppositeStateThreshold;
	}
	
	return false;
}

void AVRPawn::RegisterValue(float value)
{
	m_meditationValues.EmplaceFirst(value);
	m_meditationValues.PopLast();
	// New value registered, so reset interpTime to 0.
	m_interpTime = 0.f;
}

void AVRPawn::AssignValue()
{
	m_prevAvg = m_meditationValues.First(); 
	m_currAvg = m_meditationValues.Last();
}

void AVRPawn::ComputeAvg()
{	
	const float first = m_meditationValues[0];
	float prev = m_meditationValues[1];
	float sum = 0.f;
	
	for (int i = 2; i < m_meditationValues.Num(); ++i)
	{
		sum += prev;
		prev = m_meditationValues[i];
	}

	const float last = prev;
	
	m_prevAvg = (sum + last) / m_sumSize; 
	m_currAvg = (sum + first) / m_sumSize;

/*
	float sum = 0.f;
	
	for (int i = 0; i < m_meditationValues.Num() - 1; ++i)
		sum += m_meditationValues[i];
	
	m_prevAvg = m_currAvg;
	m_currAvg = sum / m_sumSize;
*/
}