// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Containers/Deque.h"
#include "GameFramework/Pawn.h"
#include "VRPawn.generated.h"

UCLASS()
class VR_TEST_API AVRPawn : public APawn
{
	GENERATED_BODY()

	/** Queue of the meditationQueueSize last meditation values stored. End up using a Deque to read the values to compute averages */
	TDeque<float> m_meditationValues;
	
	/** Current timer used for the lerping of the relaxation value */
	float m_interpTime = 0;
	/** Interpolation speed based on the interpolation duration chosed by the user */
	float m_interpSpeed;
	/** Relaxation average of the last "meditationQueueSize" - 1 frames */
	float m_currAvg = 0;
	/** Relaxation average of the last "meditationQueueSize" frames, except the most recent one */
	float m_prevAvg = 0;
	/** Up velocity the Character is currently aiming for */
	double m_targetZVelocity;
	/** Current up velocity of the Character */
	double m_curZVelocity;
	/** Number of values m_prevAvg and m_currAvg bases their average on. = meditationQueueSize - 1 */
	int m_sumSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* SphereCollider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* TextWidget;

public:
	// Sets default values for this pawn's properties
	AVRPawn();

	UPROPERTY(BlueprintReadOnly)
	FVector velocity;
	/** Rise velocity when relaxed */
	UPROPERTY(EditAnywhere, meta = (ClampMin="0"), Category = "Meditation")
	double riseVelocity;
	/** Fall velocity when not relaxed */
	UPROPERTY(EditAnywhere, meta = (ClampMax="0"), Category = "Meditation")
	double fallVelocity;
	/** Required rate of the values corresponding to the opposite state to change state (relaxed state > 50 not relaxed state < 50) */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"), Category = "Meditation")
	float oppositeStateThreshold;
	/** Time/Duration it should take to reach the target velocity (rise or fall velocity) when changing state */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin="0"), Category = "Meditation")
	float interpDuration;
	/** Current relaxation value, based on the current frame's interpolation between m_prevAvg and m_currAvg */
	UPROPERTY(BlueprintReadOnly)
	float relaxationValue;
	/** Number of stored relaxation value, decides how much values should be used to compute the relaxation average */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin="1"), Category = "Meditation")
	int meditationQueueSize;
	UPROPERTY(BlueprintReadOnly)
	bool bRelaxed = false;
	UPROPERTY(BlueprintReadOnly)
	bool bGrounded = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/**
	 * Calculates the new up velocity and sets the characters z-velocity with it.
	 * @param DeltaSeconds	DeltaTime
	 */
	void UpdateUpVelocity(float DeltaSeconds);
	/**
	 * Called when Landing to launch the player again if he is in relaxed and rising state. 
	 */
	UFUNCTION()
	void Landed(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

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
	UFUNCTION(BlueprintCallable)
	void SetInterpDuration(float value);
	/**
	 * Evaluates whether or not bRelaxed should change.
	 * @return True if bRelaxed should get inverted. False otherwise.
	 */
	UFUNCTION(BlueprintCallable)
	bool ShouldChangeState();
	/**
	 * Registers a new value into m_meditationValues, and pops/deletes the last one.
	 * @param value			New value to be registered
	 */
	UFUNCTION(BlueprintCallable)
	void RegisterValue(float value);
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
};
