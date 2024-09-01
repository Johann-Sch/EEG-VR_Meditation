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

FVector FFloatingData::CalculateDragForce(FVector DeltaPos, float DeltaTime) const
{
	// m/s converter
	float m_s = 100.f * DeltaTime;
	FVector velocity = DeltaPos / m_s;
	
	/* Speed in drag equation is clamped to reflect reality and avoid big jumps caused by the squared velocity.
	 * Lowering the force and increasing the resulting acceleration permits faster movements and avoids big jumps.
	 */
	velocity /= CHEAT_QUOTIENT;
	
	const float length = FMath::Clamp(velocity.Length(), 0.f, maxHandSpeedThreshold);	// magnitude of the velocity
	const float ratio = length / maxHandSpeedThreshold;					// drag coef and surface area proportional to speed for now
	const float cd = FMath::Lerp(cdMin, cdMax, ratio);					// drag coef
	const float A = FMath::Lerp(AMin, AMax, ratio);						// cross-sectional area // TODO will be made proportional to hand/controller orientation
	
	return velocity.GetSafeNormal() * (.5f * p * cd * A * FMath::Square(length));
}

void FMeditationData::LerpRelaxation(float DeltaTime)
{
	if (relaxationInterpTime <= 1.f)
		relaxationValue = FMath::Lerp(prevAvg, currAvg, relaxationInterpTime);
	relaxationInterpTime += DeltaTime;
}

void FMeditationData::ChangeState()
{
	bRelaxed = !bRelaxed;
	targetZVelocity = bRelaxed ? riseVelocity : fallVelocity;
}

void FMeditationData::Init()
{
	for (int i = 0; i < relaxationQueueSize; ++i)
		m_meditationValues.PushFirst(0);

	sumSize = relaxationQueueSize - 1;
	targetZVelocity = fallVelocity;
}

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

	md.Init();

	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AVRPawn::Landed);
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &AVRPawn::BecomeAirborne);

	m_prevLeftHandLocation = MotionControllerLeft->GetRelativeLocation();
	m_prevRightHandLocation = MotionControllerRight->GetRelativeLocation();

	fd.centerOfMass = Camera->GetRelativeLocation();
	fd.centerOfMass.Z *= fd.centerOfMassHeightRateRelativeToHMD; // We use a center of mass near shoulder height as we don't have legs information
}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	tickEvent.Broadcast(DeltaTime);
}

void AVRPawn::UpdateRelaxation(float DeltaTime)
{
	md.LerpRelaxation(DeltaTime);
		
	if (ShouldChangeState())
		md.ChangeState();
}

void AVRPawn::UpdateUpVelocity(float DeltaTime)
{
	// Ignore if falling but already on ground
	if (!md.bRelaxed && bGrounded)
		return;
	
	// Interpolate the velocity towards the target velocity
	if (!ReachedTargetVelocity())
		md.curZVelocity = FMath::FInterpConstantTo(md.curZVelocity, md.targetZVelocity,
			DeltaTime, md.interpSpeed);

	AddActorWorldOffset(DeltaTime * FVector(0.f, 0.f, md.curZVelocity));
}

void AVRPawn::IntroUpdateUpVelocity(float DeltaTime)
{
	// Ignore if falling but already on ground
	if (!md.bRelaxed && bGrounded)
		return;
	
	// Interpolate the velocity towards the target velocity
	if (!ReachedTargetVelocity())
	{
		md.introZInterpValue += DeltaTime * md.interpSpeed;
		md.curZVelocity = InterpEaseInOut(0.f, md.targetZVelocity,
			FMath::Clamp(md.introZInterpValue, 0.f, 1.f), .3f, .5f);
	}

	AddActorWorldOffset(DeltaTime * FVector(0.f, 0.f, md.curZVelocity));
}

void AVRPawn::UpdateFlyingVelocity(float DeltaTime)
{
	// hands location
	FVector left = MotionControllerLeft->GetRelativeLocation();
	FVector right = MotionControllerRight->GetRelativeLocation();
	// hands delta position, distance since last frame
	FVector leftForce = left - m_prevLeftHandLocation;
	FVector rightForce = right - m_prevRightHandLocation;
	m_prevLeftHandLocation = left;
	m_prevRightHandLocation = right;

	// applying drag created by water/air resistance to compute forces produced by the hands
	leftForce = fd.CalculateDragForce(leftForce, DeltaTime);
	rightForce = fd.CalculateDragForce(rightForce, DeltaTime);
	
	// hands position relative to shoulder height
	FVector leftRelPos = left - fd.centerOfMass;
	FVector rightRelPos = right - fd.centerOfMass;

	// torque resulting from hand movements 
	FVector leftTorque = FVector::CrossProduct(leftForce, leftRelPos);
	FVector rightTorque = FVector::CrossProduct(rightForce, rightRelPos);

	FVector acceleration = -(leftForce + rightForce) / fd.mass;		// acceleration resulting from hands pushing against imaginary water (=> opposite direction of the hand movement/force)
	acceleration = GetActorTransform().TransformVector(acceleration);	// get acceleration in world ref

	FVector angularAcceleration = (leftTorque + rightTorque) / fd.momentOfInertia;	// angular acculeration resulting from hands torques. The more similar the movements of the two hands, the lower the angular acceleration, and the opposite
	//(torque - FVector::CrossProduct(angularVelocity, angularVelocity * m_momentOfInertia)) / m_momentOfInertia;
	// constraint rotation only on z for now
	angularAcceleration = GetActorTransform().TransformVector({0.f, 0.f, angularAcceleration.Z});
	
	velocity = (velocity + acceleration * CHEAT_FACTOR) * (1 - fd.drag * DeltaTime);
	angularVelocity = (angularVelocity + angularAcceleration * FMath::Square(DeltaTime) * CHEAT_ANGULAR_FACTOR)
	* (1 - DeltaTime * fd.drag);

	// Update the position and rotation of the character
	FVector NewPosition = GetActorLocation() + velocity * DeltaTime;
	FQuat NewRotation = GetActorRotation().Quaternion() * FQuat::MakeFromEuler(angularVelocity * DeltaTime);
	
	SetActorLocationAndRotation(NewPosition, NewRotation);
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
	return md.curZVelocity == md.targetZVelocity;
}

void AVRPawn::SetIntroInterpDuration(float Value)
{
	md.interpDuration = Value;
	md.interpSpeed = 1.f / md.interpDuration;
}

void AVRPawn::SetInterpDuration(float Value)
{
	md.interpDuration = Value;
	md.interpSpeed = (md.riseVelocity - md.fallVelocity) / md.interpDuration;
}

bool AVRPawn::ShouldChangeState()
{
	if (md.bRelaxed && md.relaxationValue < 50.f || !md.bRelaxed && md.relaxationValue >= 50.f)
	{
		int unrelaxedValueCount = 0;
	
		for (int i = 0; i < md.relaxationQueueSize; ++i)
			if (md.m_meditationValues[i] < 50.f)
				unrelaxedValueCount++;

		const float unrelaxedRate = static_cast<float>(unrelaxedValueCount) / static_cast<float>(md.relaxationQueueSize);

		return md.bRelaxed && unrelaxedRate >= md.oppositeStateThreshold
			|| !md.bRelaxed && 1 - unrelaxedRate >= md.oppositeStateThreshold;
	}
	
	return false;
}

void AVRPawn::RegisterValue(float Value)
{
	md.m_meditationValues.EmplaceFirst(Value);
	md.m_meditationValues.PopLast();
	// New value registered, so reset interpTime to 0.
	md.relaxationInterpTime = 0.f;
}

void AVRPawn::AssignValue()
{
	md.prevAvg = md.m_meditationValues.First(); 
	md.currAvg = md.m_meditationValues.Last();
}

void AVRPawn::ComputeAvg()
{	
	float sum = 0.f;
	
	for (int i = 0; i < md.m_meditationValues.Num() - 1; ++i)
		sum += md.m_meditationValues[i];
	
	md.prevAvg = md.currAvg;
	md.currAvg = sum / md.sumSize;
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

void AVRPawn::BindFlyingTick()
{
	tickEvent.Clear();
	tickEvent.AddUObject(this, &AVRPawn::UpdateFlyingVelocity);
}
