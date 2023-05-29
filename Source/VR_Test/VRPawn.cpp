// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPawn.h"

#include "AntiAliasedTextWidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"

#define CHEAT_QUOTIENT 2.f
#define CHEAT_FACTOR 20.f
#define CHEAT_ANGULAR_FACTOR (CHEAT_FACTOR * 2.f)

DECLARE_LOG_CATEGORY_EXTERN(LogRelax, Log, All);
DEFINE_LOG_CATEGORY(LogRelax);

// Sets default values
AVRPawn::AVRPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	
	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollider"));
	SphereCollider->SetupAttachment(RootComponent);

	TextWidget = CreateDefaultSubobject<UAntiAliasedTextWidgetComponent>(TEXT("TextWidget"));
	TextWidget->SetupAttachment(RootComponent);

	MotionControllerLeft = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerLeft"));
	MotionControllerLeft->SetupAttachment(RootComponent);
	MotionControllerRight = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerRight"));
	MotionControllerRight->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	HMD = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HMD"));
	HMD->SetupAttachment(Camera);
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

	for (int i = 0; i < relaxationQueueSize; ++i)
		m_meditationValues.PushFirst(0);

	m_sumSize = relaxationQueueSize - 1;
	m_targetZVelocity = fallVelocity;

	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AVRPawn::Landed);
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &AVRPawn::BecomeAirborne);

	m_prevLeftHandLocation = MotionControllerLeft->GetRelativeLocation();
	m_prevRightHandLocation = MotionControllerRight->GetRelativeLocation();
	// temp
	m_targetZVelocity = riseVelocity;
	bRelaxed = true;
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

	// computing drag created by water/air resistance
	leftForce = CalculateDragForce(leftForce, DeltaTime);
	rightForce = CalculateDragForce(rightForce, DeltaTime);
	
	FVector centerOfMass = Camera->GetRelativeLocation();	centerOfMass.Z *= centerOfMassHeightRateRelativeToHMD;
	FVector leftRelPos = MotionControllerLeft->GetRelativeLocation() - centerOfMass;
	FVector rightRelPos = MotionControllerRight->GetRelativeLocation() - centerOfMass;
	
	FVector leftTorque = FVector::CrossProduct(leftForce, leftRelPos);
	FVector rightTorque = FVector::CrossProduct(rightForce, rightRelPos);

	FVector acceleration = -(leftForce + rightForce) / mass;
	acceleration = GetActorTransform().TransformVector(acceleration);

	FVector angularAcceleration = (leftTorque + rightTorque) / m_momentOfInertia; //(torque - FVector::CrossProduct(angularVelocity, angularVelocity * m_momentOfInertia)) / m_momentOfInertia;
	// constraint rotation only on z for now
	angularAcceleration = GetActorTransform().TransformVector({0.f, 0.f, angularAcceleration.Z});
	
	velocity = (velocity + acceleration * CHEAT_FACTOR) * (1 - drag * DeltaTime);
	angularVelocity = (angularVelocity + angularAcceleration * FMath::Square(DeltaTime) * CHEAT_ANGULAR_FACTOR) * (1 - DeltaTime * drag);

	// Update the position and rotation of the character
	FVector NewPosition = GetActorLocation() + velocity * DeltaTime;
	FQuat NewRotation = GetActorRotation().Quaternion() * FQuat::MakeFromEuler(angularVelocity * DeltaTime);
	
	SetActorLocationAndRotation(NewPosition, NewRotation);
	
	//TickEvent.Broadcast(DeltaTime);
}

FVector AVRPawn::CalculateDragForce(FVector Force, float DeltaTime) const
{
	// convert in m/s
	float m_s = 100.f * DeltaTime;
	Force /= m_s;
	
	/* Speed in drag equation is clamped to reflect reality and avoid big jumps caused by the squared velocity.
	 * Lowering the force and increasing the resulting acceleration permits faster movements and avoids big jumps.
	 */
	Force /= CHEAT_QUOTIENT;
	
	const float length = FMath::Clamp(Force.Length(), 0.f, m_maxHandSpeedThreshold);
	const float ratio = length / m_maxHandSpeedThreshold;
	const float cd = FMath::Lerp(cdMin, cdMax, ratio);	// drag coef and surface area proportional to speed for now
	const float A = FMath::Lerp(AMin, AMax, ratio);			// TODO will be made proportional to hand/controller orientation
	
	return Force.GetSafeNormal() * (.5f * p * cd * A * FMath::Square(length));
}

void AVRPawn::UpdateRelaxation(float DeltaSeconds)
{
	if (m_relaxationInterpTime <= 1.f)
		relaxationValue = FMath::Lerp(m_prevAvg, m_currAvg, m_relaxationInterpTime);
	m_relaxationInterpTime += DeltaSeconds;
		
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
		m_introZInterpValue += DeltaSeconds * m_interpSpeed;
		z = InterpEaseInOut(0.f, m_targetZVelocity, FMath::Clamp(m_introZInterpValue, 0.f, 1.f), .3f, .5f);
		UE_LOG(LogRelax, Log, TEXT("interpspeed %f, alpha=%f vel %f ?= target %f"), m_interpSpeed, m_introZInterpValue, z, m_targetZVelocity)
	}

	AddActorWorldOffset(DeltaSeconds * FVector(0.f, 0.f, z));
}

void AVRPawn::Landed(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	bGrounded = true;
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

void AVRPawn::SetIntroInterpDuration(float Value)
{
	interpDuration = Value;
	m_interpSpeed = 1.f / interpDuration;

	UE_LOG(LogRelax, Log, TEXT("INTRO value %f interpspeed  %f"), Value, m_interpSpeed)
}

void AVRPawn::SetInterpDuration(float Value)
{
	interpDuration = Value;
	m_interpSpeed = (riseVelocity - fallVelocity) / interpDuration;

	UE_LOG(LogRelax, Log, TEXT("FINAL interpspeed  %f"), m_interpSpeed)
}

bool AVRPawn::ShouldChangeState()
{
	if (bRelaxed && relaxationValue < 50.f || !bRelaxed && relaxationValue >= 50.f)
	{
		int unrelaxedValueCount = 0;
	
		for (int i = 0; i < relaxationQueueSize; ++i)
			if (m_meditationValues[i] < 50.f)
				unrelaxedValueCount++;

		const float unrelaxedRate = static_cast<float>(unrelaxedValueCount) / static_cast<float>(relaxationQueueSize);

		return bRelaxed && unrelaxedRate >= oppositeStateThreshold
			|| !bRelaxed && 1 - unrelaxedRate >= oppositeStateThreshold;
	}
	
	return false;
}

void AVRPawn::RegisterValue(float Value)
{
	m_meditationValues.EmplaceFirst(Value);
	m_meditationValues.PopLast();
	// New value registered, so reset interpTime to 0.
	m_relaxationInterpTime = 0.f;
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