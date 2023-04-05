// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "AntiAliasedTextWidgetComponent.generated.h"

/**
 * 
 */
UCLASS()
class VR_TEST_API UAntiAliasedTextWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
	virtual void UpdateRenderTarget(FIntPoint DesiredRenderTargetSize) override;
	virtual void DrawWidgetToRenderTarget(float DeltaTime) override;
};
