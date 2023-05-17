// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPawn.h"

#include "AntiAliasedTextWidgetComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"

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

	MotionControllerLeft = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerLeft"));
	MotionControllerLeft->SetupAttachment(RootComponent);
	MotionControllerRight = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerRight"));
	MotionControllerRight->SetupAttachment(RootComponent);
}

/** Interpolate between A and B, applying an ease out/in function.  Exp controls the degree of the curve. */
template< class T >
UE_NODISCARD static FORCEINLINE_DEBUGGABLE T InterpEaseInOut( const T& A, const T& B, float Alpha, float ExpIn, float ExpOut )
{
	return FMath::Lerp<T>(A, B, Alpha < .5f ?
		(1.f - FGenericPlatformMath::Pow(1.f - Alpha * 2.f, ExpIn)) * 0.5f
		: FGenericPlatformMath::Pow(Alpha * 2.f - 1.f, ExpOut) * 0.5f + 0.5f);
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

	m_sumSize = meditationQueueSize - 1;
	m_targetZVelocity = fallVelocity;

	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AVRPawn::Landed);
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &AVRPawn::BecomeAirborne);

	m_prevLeftHandLocation = MotionControllerLeft->GetRelativeLocation();
	m_prevRightHandLocation = MotionControllerRight->GetRelativeLocation();
}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector left = MotionControllerLeft->GetRelativeLocation();
	FVector right = MotionControllerRight->GetRelativeLocation();
	
	FVector leftForce = left - m_prevLeftHandLocation;
	FVector rightForce = right - m_prevRightHandLocation;

	m_prevLeftHandLocation = left;
	m_prevRightHandLocation = right;

	if (leftForce.SquaredLength() < 5.f)
		leftForce *= FMath::Pow(leftForce.Length() / 5.f, 4);
	if (rightForce.SquaredLength() < 5.f)
		rightForce *= FMath::Pow(rightForce.Length() / 5.f, 4);
	
	FVector torque = FVector::CrossProduct(leftForce, MotionControllerLeft->GetRelativeLocation())
				+ FVector::CrossProduct(rightForce, MotionControllerRight->GetRelativeLocation());
	
	FVector acceleration = - (leftForce + rightForce);
	FVector angularAcceleration = (torque - angularVelocity * drag - FVector::CrossProduct(angularVelocity, angularVelocity * m_momentOfInertia)) / m_momentOfInertia;

	velocity += acceleration - velocity * drag;
	angularVelocity += angularAcceleration * DeltaTime;

	// // Update the position and rotation of the character
	FVector NewPosition = GetActorLocation() + velocity * DeltaTime;
	FQuat NewRotation = GetActorRotation().Quaternion(); // * FQuat::MakeFromEuler(angularVelocity * DeltaTime);
	
	SetActorLocationAndRotation(NewPosition, NewRotation);

	// Reset the applied forces and torques
	//TotalForce = FVector::ZeroVector;
	//TotalTorque = FVector::ZeroVector;
	
	//TickEvent.Broadcast(DeltaTime);
}

void AVRPawn::UpdateRelaxation(float DeltaSeconds)
{
	if (m_interpTime <= 1.f)
		relaxationValue = FMath::Lerp(m_prevAvg, m_currAvg, m_interpTime);
	m_interpTime += DeltaSeconds;
		
	if (ShouldChangeState())
	{
		bRelaxed = !bRelaxed;
		m_targetZVelocity = bRelaxed ? riseVelocity : fallVelocity;
	}
}


void AVRPawn::UpdateUpVelocity(float DeltaSeconds)
{
	// Ignore if falling but already on ground
	if (!bRelaxed && bGrounded)
		return;
	
	// Interpolate the velocity towards the target velocity
	if (!ReachedTargetVelocity())
		z = FMath::FInterpConstantTo(z, m_targetZVelocity, DeltaSeconds, m_interpSpeed);

	AddActorWorldOffset(DeltaSeconds * FVector(0.f, 0.f, z));
}

void AVRPawn::IntroUpdateUpVelocity(float DeltaSeconds)
{
	// Ignore if falling but already on ground
	if (!bRelaxed && bGrounded)
		return;
	
	// Interpolate the velocity towards the target velocity
	if (!ReachedTargetVelocity())
	{
		m_introAlpha += DeltaSeconds * m_interpSpeed;
		z = InterpEaseInOut(0., m_targetZVelocity, FMath::Clamp(m_introAlpha, 0.f, 1.f), .3f, .5f);
	}

	AddActorWorldOffset(DeltaSeconds * FVector(0.f, 0.f, z));
}

void AVRPawn::Landed(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	bGrounded = true;
	// reset velocity and target velocity
	//m_targetZVelocity = velocity.Z = 0.f;
}

void AVRPawn::BecomeAirborne(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	bGrounded = false;
}

bool AVRPawn::ReachedTargetVelocity()
{
	return z == m_targetZVelocity;
}

void AVRPawn::SetIntroInterpDuration(float value)
{
	interpDuration = value;
	m_interpSpeed = 1.f / interpDuration;

	UE_LOG(LogRelax, Log, TEXT("INTRO value %f interpspeed  %f"), value, m_interpSpeed)
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
	float sum = 0.f;
	
	for (int i = 0; i < m_meditationValues.Num() - 1; ++i)
		sum += m_meditationValues[i];
	
	m_prevAvg = m_currAvg;
	m_currAvg = sum / m_sumSize;
}

void AVRPawn::BindIntroTick()
{
	tickEvent.Clear();
	tickEvent.AddUObject(this, &AVRPawn::UpdateRelaxation);	
	tickEvent.AddUObject(this, &AVRPawn::IntroUpdateUpVelocity);
}

void AVRPawn::BindDefaultRiseTick()
{
	tickEvent.Clear();
	tickEvent.AddUObject(this, &AVRPawn::UpdateRelaxation);	
	tickEvent.AddUObject(this, &AVRPawn::UpdateUpVelocity);
}
