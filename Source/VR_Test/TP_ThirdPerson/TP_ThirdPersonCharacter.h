// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Deque.h"
#include "GameFramework/Character.h"
#include "TP_ThirdPersonCharacter.generated.h"

UCLASS(config=Game)
class ATP_ThirdPersonCharacter : public ACharacter
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

	/** Rise velocity when relaxed */
	UPROPERTY(EditAnywhere, meta = (ClampMin="0"))
	double riseVelocity;
	/** Fall velocity when not relaxed */
	UPROPERTY(EditAnywhere, meta = (ClampMax="0"))
	double fallVelocity;
	/** Required rate of the values corresponding to the opposite state to change state (relaxed state > 50 not relaxed state < 50) */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"))
	float oppositeStateThreshold;
	/** Time/Duration it should take to reach the target velocity (rise or fall velocity) when changing state */
	UPROPERTY(EditAnywhere, meta = (ClampMin="0"))
	float interpDuration;
	/** Current relaxation value, based on the current frame's interpolation between m_prevAvg and m_currAvg */
	UPROPERTY(BlueprintReadOnly)
	float relaxationValue;
	/** Number of stored relaxation value, decides how much values should be used to compute the relaxation average */
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
	/**
	 * Called when Landing to launch the player again if he is in relaxed and rising state. 
	 */
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

