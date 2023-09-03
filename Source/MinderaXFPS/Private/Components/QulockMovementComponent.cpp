// Ricardo Santos, 2023

#include "Components/QulockMovementComponent.h"

#include "BaseGizmos/GizmoMath.h"
#include "GameFramework/MXFPSGameStateBase.h"

namespace
{
	FVector ProjectPointOntoPlane(FVector const& Point, FVector const& PlaneOrigin, FVector const& PlaneNormal)
	{
		return PlaneNormal * FVector::DotProduct(PlaneOrigin - Point, PlaneNormal) + Point;
	}
}

UQulockMovementComponent::UQulockMovementComponent(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

#define MXFPS_SHOULD_USE_PROJECTED_VERTEX_TRACES 1
#define MXFPS_SHOULD_SHOW_DEBUG_TRACES 0

void UQulockMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bCanOwnerMove = true;

	auto World = GetWorld();
	auto GameState = World->GetGameState<AMXFPSGameStateBase>();

	FVector ViewOrigin = GameState->GetPlayerViewOrigin();
#if MXFPS_SHOULD_USE_PROJECTED_VERTEX_TRACES
	FMatrix ViewMatrix = GameState->GetPlayerViewProjMatrix();

	TArray<FPlane, TFixedAllocator<4>> FrustumPlaneArray;
	FrustumPlaneArray.SetNum(4);
	
	ViewMatrix.GetFrustumLeftPlane(FrustumPlaneArray[0]);
	ViewMatrix.GetFrustumRightPlane(FrustumPlaneArray[1]);
	ViewMatrix.GetFrustumTopPlane(FrustumPlaneArray[2]);
	ViewMatrix.GetFrustumBottomPlane(FrustumPlaneArray[3]);
#endif

	TArray<FVector> SupportVertexArray;
	if (GameState->IsActorWithinPlayerFrustum(GetOwner(), SupportVertexArray))
	{
		bool bNewCanOwnerMove = true;
		for (FVector Vertex : SupportVertexArray)
		{
			constexpr float TraceThreshold = 10.f;
			
			// Extend trace a bit to ensure it hits the geometry
			FVector TraceDirection = (Vertex - ViewOrigin).GetUnsafeNormal();
			Vertex += TraceDirection * TraceThreshold;

#		if MXFPS_SHOULD_SHOW_DEBUG_TRACES
			DrawDebugLine(World, ViewOrigin, Vertex, FColor::Red, false, 1.f, 255);
#		endif
			FHitResult HitResult;
			if (World->LineTraceSingleByChannel(HitResult, ViewOrigin, Vertex, ECC_Visibility))
			{
				// NOTE: this currently has a limitation: if the point is actually outside the view frustum,
				// it will still report as a hit, but the actor might not be visible anymore
				// (this will only happen when there is some object occluding the actor).
				// MXFPS_SHOULD_USE_PROJECTED_VERTEX_TRACES tries to mitigate this, but is currently imperfect.
				bNewCanOwnerMove = HitResult.GetActor() != GetOwner();
#			if MXFPS_SHOULD_SHOW_DEBUG_TRACES
				DrawDebugPoint(World, HitResult.Location, 10.f,
							   bNewCanOwnerMove ? FColor::Yellow : FColor::Green, false, 1.f, 255);
#			endif
				if (!bNewCanOwnerMove)
				{
#				if MXFPS_SHOULD_USE_PROJECTED_VERTEX_TRACES
					FVector ProjectedVertex = Vertex;
					bool bShouldProject = false;
					
					for (FPlane const& Plane : FrustumPlaneArray)
					{
						if (Plane.PlaneDot(ProjectedVertex) > 0)
						{
							auto PlaneNormal = Plane.GetNormal();
							ProjectedVertex = ProjectPointOntoPlane(ProjectedVertex, ViewOrigin, PlaneNormal);
							bShouldProject = true;
						}
					}
					
					if (bShouldProject)
					{
#					if MXFPS_SHOULD_SHOW_DEBUG_TRACES
						DrawDebugLine(World, ViewOrigin, ProjectedVertex, FColor::Red, false, 1.f, 255);
#					endif
						HitResult.Reset();
						if (World->LineTraceSingleByChannel(HitResult, ViewOrigin, ProjectedVertex, ECC_Visibility))
						{
							bNewCanOwnerMove = HitResult.GetActor() != GetOwner();
#						if MXFPS_SHOULD_SHOW_DEBUG_TRACES
							DrawDebugPoint(World, HitResult.Location, 10.f,
										   bNewCanOwnerMove ? FColor::Yellow : FColor::Green, false, 1.f, 255);
#						endif
						}
					}
					
					if (!bNewCanOwnerMove)
					{
						bCanOwnerMove = false;
#					if !MXFPS_SHOULD_SHOW_DEBUG_TRACES
						break;
#					endif
					}
#				else
					bCanOwnerMove = false;
#				if !MXFPS_SHOULD_SHOW_DEBUG_TRACES
					break;
#				endif
#				endif
				}
			}
		}
	}
}

