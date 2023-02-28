// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Deque.h"
#include "GameFramework/Character.h"
#include "TP_ThirdPersonCharacter.generated.h"

struct FFrameStep
{
	float dt;
	float timestamp;
};

UCLASS(config=Game)
class ATP_ThirdPersonCharacter : public ACharacter
{
	GENERATED_BODY()

	TDeque<float> m_meditationValues;
	TQueue<float> kk;
	
	TQueue<FFrameStep> m_unrelaxedFrames;
	float m_unrelaxedDuration = 0.f;

	float m_interpTime = 0;
	float m_interpSpeed;
	float m_currAvg = 0;
	float m_prevAvg = 0;
	double m_targetZVelocity;
	double m_curZVelocity;
	int m_sumSize;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	ATP_ThirdPersonCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;
	
	UPROPERTY(EditAnywhere, meta = (ClampMin="0"))
	double riseVelocity;
	UPROPERTY(EditAnywhere, meta = (ClampMax="0"))
	double fallVelocity;
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"))
	float oppositeStateThreshold; // Pourcentage de valeurs correspondant à l'état opposé nécessaires pour changer d'état (état relaxed: valeurs > 50 état non relaxed: valeurs < 50)
	UPROPERTY(EditAnywhere, meta = (ClampMin="0"))
	float interpDuration;
	UPROPERTY(BlueprintReadOnly)
	float relaxationValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin="1"))
	int meditationQueueSize;
	UPROPERTY(BlueprintReadOnly)
	bool bRelaxed;

protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	/**
	 * Calculates the new up velocity and sets the characters z-velocity with it.
	 * @param DeltaSeconds	DeltaTime
	 */
	void UpdateUpVelocity(float DeltaSeconds);
	UFUNCTION()
	void Landed(const FHitResult& Hit);
	
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

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

