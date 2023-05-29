// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Deque.h"
#include "GameFramework/Pawn.h"
#include "VRPawn.generated.h"

DECLARE_EVENT_OneParam(AVRPawn, TickEvent, float)

UCLASS()
class VR_TEST_API AVRPawn : public APawn
{
	GENERATED_BODY()

	/** Queue of the meditationQueueSize last meditation values stored. End up using a Deque to read the values to compute averages */
	TDeque<float> m_meditationValues;

	FVector m_prevLeftHandLocation;
	FVector m_prevRightHandLocation;
	float m_momentOfInertia = 4.f;

	float m_maxHandSpeedThreshold = 2.f;
	/** Min drag coefficient produced by hand movement (when hands parallel to hand direction) */
	float cdMin = 0.1f;
	/** Max drag coefficient produced by hand movement (when hands perpendiculqr to hand direction) */
	float cdMax = 1.2f;
	/** ρ - rhô representing water density */
	float p = 1000.f;
	/** Min hand surface area in m²*/
	float AMin = 0.0015f;
	/** Max hand surface area in m²*/
	float AMax = 0.0145f;
	/** mass of the character */
	float mass = 65.0f;
	/** Current timer used for lerping the relaxation value */
	float m_relaxationInterpTime = 0;
	/** Interpolation speed based on the interpolation duration chosed by the user */
	float m_interpSpeed;
	/** Relaxation average of the last "meditationQueueSize" - 1 frames */
	float m_currAvg = 0;
	/** Relaxation average of the last "meditationQueueSize" frames, except the most recent one */
	float m_prevAvg = 0;
	/** Up velocity the Character is currently aiming for */
	float m_targetZVelocity;
	/** Current up velocity of the Character */
	float m_curZVelocity;
	/** Current lerping value used to reach target Z velocity during intro */
	float m_introZInterpValue = 0.f;
	/** Number of values m_prevAvg and m_currAvg bases their average on. = meditationQueueSize - 1 */
	int m_sumSize;
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* SphereCollider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* TextWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* MotionControllerRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* MotionControllerLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* HMD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	TickEvent tickEvent;
public:
	// Sets default values for this pawn's properties
	AVRPawn();

	UPROPERTY(BlueprintReadOnly)
	FVector velocity;
	UPROPERTY(BlueprintReadWrite)
	FVector angularVelocity;
	/** Rise velocity when relaxed */
	UPROPERTY(EditAnywhere, meta = (ClampMin="0"), Category = "Meditation")
	float riseVelocity;
	/** Fall velocity when not relaxed */
	UPROPERTY(EditAnywhere, meta = (ClampMax="0"), Category = "Meditation")
	float fallVelocity;
	/** Required rate of the values corresponding to the opposite state to change state (relaxed state > 50 not relaxed state < 50) */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"), Category = "Meditation")
	float oppositeStateThreshold;
	/** Time/Duration it should take to reach the target velocity (rise or fall velocity) when changing state */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin="0"), Category = "Meditation")
	float interpDuration;
	/** Drag applied to movement produced by controllers / hand movement in air */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"), Category = "Meditation")
	float drag = .5f;
	/** At what percentage of the HMD height the center of mass will considered to be? */
	UPROPERTY(EditAnywhere, meta = (ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"), Category = "Meditation")
	float centerOfMassHeightRateRelativeToHMD = 0.8f;
	/** Current relaxation value, based on the current frame's interpolation between m_prevAvg and m_currAvg */
	UPROPERTY(BlueprintReadOnly)
	float relaxationValue;
	UPROPERTY(BlueprintReadOnly)
	float z;
	/** Number of stored relaxation values, decides how many previous values should be used to compute the relaxation average */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin="1"), Category = "Meditation")
	int relaxationQueueSize;
	UPROPERTY(BlueprintReadOnly)
	bool bRelaxed = false;
	UPROPERTY(BlueprintReadOnly)
	bool bGrounded = false;

	bool temp = true;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/**
	 * Calculates Drag force created from the hand movement, using the Drag Equation.
	 * @param Force		Force
	 * @param DeltaTime	DeltaTime
	 * @return			Drag force
	 */
	FVector CalculateDragForce(FVector Force, float DeltaTime) const;
	/**
	 * Calculates the new relaxation value and evaluates whether the relaxed state should change.
	 * @param DeltaTime	DeltaTime
	 */
	void UpdateRelaxation(float DeltaTime);
	/**
	 * Calculates the new up velocity and sets the characters z-velocity with it.
	 * @param DeltaTime	DeltaTime
	 */
	void UpdateUpVelocity(float DeltaTime);
	/**
	 * Introduction that calculates the new up velocity and sets the characters z-velocity with it.
	 * @param DeltaTime	DeltaTime
	 */
	void IntroUpdateUpVelocity(float DeltaTime);
	/**
	 * To use during flying in air step, calculates the new velocity and sets the characters velocity with it.
	 * @param DeltaTime	DeltaTime
	 */
	void UpdateFlyingVelocity(float DeltaTime);
	/**
	 * Called when Landing. 
	 */
	UFUNCTION()
	void Landed(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	/**
	 * Called when Becoming aribone. 
	 */
	UFUNCTION()
	void BecomeAirborne(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * Evaluates whether the target up velocity has been reached or not.
	 * @return True if velocity equals target velocity.
	 */
	UFUNCTION(BlueprintCallable)
	bool ReachedTargetVelocity();
	/**
	 * Sets the duration the interpolation to reach the max rise speed during the intro should take.
	 * @param Value			duration
	 */
	UFUNCTION(BlueprintCallable)
	void SetIntroInterpDuration(float Value);
	/**
	 * Sets the duration the interpolation to reach the max rise speed should take.
	 * @param Value			duration
	 */
	UFUNCTION(BlueprintCallable)
	void SetInterpDuration(float Value);
	/**
	 * Evaluates whether or not bRelaxed should change.
	 * @return True if bRelaxed should get inverted. False otherwise.
	 */
	UFUNCTION(BlueprintCallable)
	bool ShouldChangeState();
	/**
	 * Registers a new value into m_meditationValues, and pops/deletes the last one.
	 * @param Value			New value to be registered
	 */
	UFUNCTION(BlueprintCallable)
	void RegisterValue(float Value);
	/**
	 * To use if the m_meditationValues array has only 1 or 2 values.
	 * Assigns the one or two only values present in m_meditationValues to m_prevAvg and m_currAvg.
	 * Basically pointless, this function exists just in case we want a flat value (?).
	 */
	UFUNCTION(BlueprintCallable)
	void AssignValue();
	/**
	* Updates m_prevAvg and m_currAvg based on the just retrieved new value, and the past values together.
	*/
	UFUNCTION(BlueprintCallable)
	void ComputeAvg();
	/**
	* Bind intro tick implementation to tick function (slow rise at start and rise speed increasing).
	*/
	UFUNCTION(BlueprintCallable)
	void BindIntroTick();
	/**
	* Bind default rise tick implementation to tick function (default rise speed).
	*/
	UFUNCTION(BlueprintCallable)
	void BindDefaultRiseTick();
	/**
	* Bind flying in air tick implementation to tick function.
	*/
	UFUNCTION(BlueprintCallable)
	void BindFlyingTick();
};