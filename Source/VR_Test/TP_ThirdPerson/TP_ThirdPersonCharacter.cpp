// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_ThirdPersonCharacter.h"

#include <deque>

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ATP_ThirdPersonCharacter

ATP_ThirdPersonCharacter::ATP_ThirdPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATP_ThirdPersonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ATP_ThirdPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ATP_ThirdPersonCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ATP_ThirdPersonCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ATP_ThirdPersonCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ATP_ThirdPersonCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ATP_ThirdPersonCharacter::TouchStopped);
}

void ATP_ThirdPersonCharacter::BeginPlay()
{
	Super::BeginPlay();

	for (int i = 0; i < meditationQueueSize; ++i)
		m_meditationValues.PushFirst(0);

	m_interpSpeed = (riseVelocity - fallVelocity) / interpDuration;
	m_sumSize = meditationQueueSize - 1;
	m_targetZVelocity = fallVelocity;

	LandedDelegate.AddDynamic(this, &ATP_ThirdPersonCharacter::Landed);
}

void ATP_ThirdPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (m_interpTime <= 1.f)
		relaxationValue = FMath::Lerp(m_prevAvg, m_currAvg, m_interpTime);
		//relaxationValue = m_prevAvg * (1 - m_interpTime) - m_currAvg * m_interpTime;
	
	m_interpTime += DeltaSeconds;
		
	if (ShouldChangeState())
	{
		bRelaxed = !bRelaxed;
		m_targetZVelocity = bRelaxed ? riseVelocity : fallVelocity;

		if(bRelaxed && !GetCharacterMovement()->IsFalling())
			LaunchCharacter(FVector(0.f, 0.f, DeltaSeconds), false, false);
	}
	
	UpdateUpVelocity(DeltaSeconds);
}

void ATP_ThirdPersonCharacter::UpdateUpVelocity(float DeltaSeconds)
{
	// Ignore if falling but already on ground
	if (!bRelaxed && !GetCharacterMovement()->IsFalling())
		return;

	auto& z = GetCharacterMovement()->Velocity.Z;
	// Interpolate the velocity towards the target velocity
	if (z != m_targetZVelocity)
	{
		if (m_curZVelocity != m_targetZVelocity)
			m_curZVelocity = FMath::FInterpConstantTo(m_curZVelocity, m_targetZVelocity, DeltaSeconds, m_interpSpeed);
		z = m_curZVelocity;
	}
	
	// UE_LOG(LogTemp, Log, TEXT("%s %f != %f?"), bRelaxed ? TEXT("rise") : TEXT("fall"), z, m_targetZVelocity);
	// UE_LOG(LogTemp, Log, TEXT("XXXXXX %f != %f, prev-vel = %f"), m_curZVelocity, m_targetZVelocity, GetCharacterMovement()->Velocity.Z);
	// UE_LOG(LogTemp, Log, TEXT("VVVVVV %f == %f"), m_curZVelocity, m_targetZVelocity);
}

void ATP_ThirdPersonCharacter::Landed(const FHitResult& Hit)
{
	if(bRelaxed)
		LaunchCharacter(FVector(0.f, 0.f, m_curZVelocity), false, false);
}

bool ATP_ThirdPersonCharacter::ShouldChangeState()
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

void ATP_ThirdPersonCharacter::RegisterValue(float value)
{
	//TEMP
	value = FMath::Abs(value) * 100;
	// UE_LOG(LogTemp, Log, TEXT("registering %f"), value);
	m_meditationValues.EmplaceFirst(value);
	m_meditationValues.PopLast();
	// New value registered, so reset interpTime to 0.
	m_interpTime = 0.f;
}

void ATP_ThirdPersonCharacter::AssignValue()
{
	m_prevAvg = m_meditationValues.First(); 
	m_currAvg = m_meditationValues.Last();
}

void ATP_ThirdPersonCharacter::ComputeAvg()
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

	if (m_meditationValues.Num() == 4)
	{
		UE_LOG(LogTemp, Log,TEXT("bRelaxed%d. prevAvg=%f, currAvg=%f --- v1=%f, v2=%f, v3=%f, v4=%f"), bRelaxed, m_prevAvg, m_currAvg, 
			m_meditationValues[0], m_meditationValues[1], m_meditationValues[2], m_meditationValues[3]);
	}

}

void ATP_ThirdPersonCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ATP_ThirdPersonCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ATP_ThirdPersonCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ATP_ThirdPersonCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ATP_ThirdPersonCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATP_ThirdPersonCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
