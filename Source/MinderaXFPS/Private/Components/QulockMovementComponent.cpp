// Ricardo Santos, 2023

#include "Components/QulockMovementComponent.h"

#include "Subsystem/PlayerViewDataCachingSubsystem.h"

DECLARE_STATS_GROUP(TEXT("Qulock Movement Logic"), STATGROUP_QulockMovement, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Is Actor Within Player View"), STAT_IsActorWithinPlayerView, STATGROUP_QulockMovement);
DECLARE_CYCLE_STAT(TEXT("Is Actor Within Player Frustum"), STAT_IsActorWithinPlayerFrustum, STATGROUP_QulockMovement);

#define QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES 1
#define QULOCK_SHOULD_SHOW_DEBUG_TRACES 0

// Theory: https://en.wikipedia.org/wiki/Supporting_hyperplane
// In summary, for each plane of the player's view frustum,
// we want to compare the first vertex of an actor (or its bounds)
// that would appear in the player's view if the actor were moving in the direction of the frustum,
// i.e., for the left plane, we want to find the vertex that, projected on the screen,
// would be the rightmost vertex of the actor.
// We'll do this for each plane, and if all points are beyond the respective planes,
// we'll test each point later through a raycast (from the view's origin).
// We need to find only a single raycast which hits to stop the actor from moving.

namespace
{
	template <class AllocatorType>
	FVector ComputeProjectedSupportVertex(TArray<FVector, AllocatorType> const& PolyVertexes,
										  FMatrix const& ViewProjMat, FIntRect const& ViewRect,
										  FVector2D const& Direction)
	{
		float MaxProjection = -TNumericLimits<float>::Max();

		FVector SupportVertex;
		for (FVector const& Vertex : PolyVertexes)
		{
			FVector2D ScreenVertex;
			FSceneView::ProjectWorldToScreen(Vertex, ViewRect, ViewProjMat, ScreenVertex);

			float Projection = Direction.Dot(ScreenVertex);
			if (Projection > MaxProjection)
			{
				MaxProjection = Projection;
				SupportVertex = Vertex;
			}
		}

		return SupportVertex;
	}

	FVector ProjectPointOntoPlane(FVector const& Point, FVector const& PlaneOrigin, FVector const& PlaneNormal)
	{
		return PlaneNormal * FVector::DotProduct(PlaneOrigin - Point, PlaneNormal) + Point;
	}
}

UQulockMovementComponent::UQulockMovementComponent(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UQulockMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	for (auto PlayerIndex : PlayersToCheck)
	{
		int32 PlayerControllerIndex = PlayerIndex - EAutoReceiveInput::Player0;
		if (ensureMsgf(PlayerControllerIndex != INDEX_NONE, TEXT("PlayersToCheck was improperly configured")))
		{
			auto PlayerControllerIter = GetWorld()->GetPlayerControllerIterator();
			PlayerControllerIter += PlayerControllerIndex;

			SetupPlayerToCheck(PlayerControllerIter->Get());
		}
	}
}

void UQulockMovementComponent::SetupPlayerToCheck(APlayerController* PlayerController)
{
	bool bAlreadySetup;
	PlayerControllerSet.Add(PlayerController, &bAlreadySetup);
	
	if (!bAlreadySetup)
	{
		GetCachingSubsystem()->SetupPlayerViewDataUpdate(PlayerController);
	}
}

bool UQulockMovementComponent::CanMoveThisFrame() const
{
	if (bCanOwnerMove.IsSet())
	{
		return bCanOwnerMove.GetValue();
	}

	bCanOwnerMove = true;
	for (auto Player : PlayerControllerSet)
	{
		if (IsActorWithinPlayerView(Player, GetOwner()))
		{
			bCanOwnerMove = false;
			break;
		}
	}
	
	return bCanOwnerMove.GetValue();
}

bool UQulockMovementComponent::IsActorWithinPlayerView(APlayerController* Player, AActor* Actor) const
{
	SCOPE_CYCLE_COUNTER(STAT_IsActorWithinPlayerView);

	bool bIsInView = false;

	auto World = GetWorld();
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);

	FVector const& ViewOrigin = PlayerViewData.ViewOrigin;
#if QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES
	FMatrix const& ViewProjMatrix = PlayerViewData.ViewProjMatrix;

	TArray<FPlane, TFixedAllocator<4>> FrustumPlaneArray;
	FrustumPlaneArray.SetNum(4);

	ViewProjMatrix.GetFrustumLeftPlane(FrustumPlaneArray[0]);
	ViewProjMatrix.GetFrustumRightPlane(FrustumPlaneArray[1]);
	ViewProjMatrix.GetFrustumTopPlane(FrustumPlaneArray[2]);
	ViewProjMatrix.GetFrustumBottomPlane(FrustumPlaneArray[3]);
#endif

	TArray<FVector> SupportVertexArray;
	if (IsActorWithinPlayerFrustum(Player, Actor, SupportVertexArray))
	{
		bool bNewIsInView = true;
		for (FVector Vertex : SupportVertexArray)
		{
			constexpr float TraceTolerance = 10.f;
		
			// Extend trace a bit to ensure it hits the geometry
			FVector TraceDirection = (Vertex - ViewOrigin).GetUnsafeNormal();
			Vertex += TraceDirection * TraceTolerance;

#		if QULOCK_SHOULD_SHOW_DEBUG_TRACES
			DrawDebugLine(World, ViewOrigin, Vertex, FColor::Yellow, false, 1.f, 255);
#		endif
			FHitResult HitResult;
			if (World->LineTraceSingleByChannel(HitResult, ViewOrigin, Vertex, ECC_Visibility))
			{
				// NOTE: this currently has a limitation: if the point is actually outside the view frustum,
				// it will still report as a hit, but the actor might not be visible anymore due to occluding actors.
				// QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES tries to mitigate this, and does it pretty decently.
				
				bNewIsInView = HitResult.GetActor() == Actor;
#			if QULOCK_SHOULD_SHOW_DEBUG_TRACES
				DrawDebugPoint(World, HitResult.Location, 10.f,
							   bNewIsInView ? FColor::Green : FColor::Red, false, 1.f, 255);
#			endif
				if (bNewIsInView)
				{
#				if QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES
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
#					if QULOCK_SHOULD_SHOW_DEBUG_TRACES
						DrawDebugLine(World, ViewOrigin, ProjectedVertex, FColor::Yellow, false, 1.f, 255);
#					endif
						HitResult.Reset();
						if (World->LineTraceSingleByChannel(HitResult, ViewOrigin, ProjectedVertex, ECC_Visibility))
						{
							bNewIsInView = HitResult.GetActor() == Actor;
#						if QULOCK_SHOULD_SHOW_DEBUG_TRACES
							DrawDebugPoint(World, HitResult.Location, 10.f,
										   bNewIsInView ? FColor::Green : FColor::Red, false, 1.f, 255);
#						endif
						}
					}
				
					if (bNewIsInView)
					{
						bIsInView = true;
#					if !QULOCK_SHOULD_SHOW_DEBUG_TRACES
						break;
#					endif
					}
#				else
					bIsInView = true;
#				if !QULOCK_SHOULD_SHOW_DEBUG_TRACES
					break;
#				endif
#				endif
				}
			}
		}
	}

	return bIsInView;
}

bool UQulockMovementComponent::IsActorWithinPlayerFrustum(APlayerController* Player, AActor* Actor,
														  TArray<FVector>& OutSupportVertexes) const
{
	SCOPE_CYCLE_COUNTER(STAT_IsActorWithinPlayerFrustum);
	
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);

	FBox ActorBounds = Actor->GetComponentsBoundingBox();

	FVector ActorBoundsPos = ActorBounds.GetCenter();
	FVector ActorBoundsSizeAbs = ActorBounds.GetExtent();
		
	TArray<FVector, TFixedAllocator<8>> ActorBoundsVertexes;
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	
	FMatrix const& ViewProjMat = PlayerViewData.ViewProjMatrix;
	FIntRect const& ViewRect = PlayerViewData.ViewRectangle;

	static FVector2D ScreenLeftDir(-1.f, 0.f);
	static FVector2D ScreenRightDir(+1.f, 0.f);
	static FVector2D ScreenUpDir(0.f, -1.f);
	static FVector2D ScreenDownDir(0.f, +1.f);
	
	FPlane RightPlane;
	ViewProjMat.GetFrustumRightPlane(RightPlane);
	
	FVector LeftSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															  ViewRect, ScreenLeftDir);

	float LeftSignedDistance = RightPlane.PlaneDot(LeftSupportVertex);
	if (LeftSignedDistance > 0)
	{
		return false;
	}

	FPlane LeftPlane;
	ViewProjMat.GetFrustumLeftPlane(LeftPlane);

	FVector RightSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															   ViewRect, ScreenRightDir);

	float RightSignedDistance = LeftPlane.PlaneDot(RightSupportVertex);
	if (RightSignedDistance > 0)
	{
		return false;
	}
	
	FPlane BottomPlane;
	ViewProjMat.GetFrustumBottomPlane(BottomPlane);
	
	FVector TopSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															 ViewRect, ScreenUpDir);

	float TopSignedDistance = BottomPlane.PlaneDot(TopSupportVertex);
	if (TopSignedDistance > 0)
	{
		return false;
	}
	
	FPlane TopPlane;
	ViewProjMat.GetFrustumTopPlane(TopPlane);
	
	FVector BottomSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
																ViewRect, ScreenDownDir);

	float BottomSignedDistance = TopPlane.PlaneDot(BottomSupportVertex);
	if (BottomSignedDistance > 0)
	{
		return false;
	}

	FMatrix const& InvViewRotMat = PlayerViewData.InvViewRotMatrix;

	// These are the axes of the player view's frame of reference
	FVector WorldRightDir = InvViewRotMat.GetScaledAxis(EAxis::X);
	FVector WorldLeftDir = -WorldRightDir;
	FVector WorldUpDir = InvViewRotMat.GetScaledAxis(EAxis::Y);
	FVector WorldDownDir = -WorldUpDir;

	OutSupportVertexes.Reserve(4);
	
	// We'll push the vertexes into the mesh a bit to ensure the raycast always hits when it should
	OutSupportVertexes.Add(LeftSupportVertex + WorldRightDir);
	OutSupportVertexes.Add(RightSupportVertex + WorldLeftDir);
	OutSupportVertexes.Add(TopSupportVertex + WorldDownDir);
	OutSupportVertexes.Add(BottomSupportVertex + WorldUpDir);

	return true;
}

FMatrix UQulockMovementComponent::GetPlayerViewProjMatrix(APlayerController* Player) const
{
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);
	return PlayerViewData.ViewProjMatrix;
}

FVector UQulockMovementComponent::GetPlayerViewOrigin(APlayerController* Player) const
{
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);
	return PlayerViewData.ViewOrigin;
}

UPlayerViewDataCachingSubsystem* UQulockMovementComponent::GetCachingSubsystem() const
{
	return CachingSubsystem
		 ? CachingSubsystem
		 : CachingSubsystem = GetWorld()->GetSubsystem<UPlayerViewDataCachingSubsystem>();
}
