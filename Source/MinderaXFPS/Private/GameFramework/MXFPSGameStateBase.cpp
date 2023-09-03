// Ricardo Santos, 2023

#include "GameFramework/MXFPSGameStateBase.h"

namespace
{
	// Theory: https://en.wikipedia.org/wiki/Supporting_hyperplane
	// In summary, for each plane of the player's view frustum,
	// we want to compare the first vertex of an actor (or its bounds)
	// that would appear in the player's view if the actor were moving in the direction of the frustum,
	// i.e., for the left plane, we want to find the vertex that, projected on the screen,
	// would be the rightmost vertex of the actor.
	// We'll do this for each plane, and if all points are beyond the respective planes,
	// we'll test each point later through a raycast (from the view's origin).
	// We need to find only a single raycast which hits to stop the actor from moving.
	
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
}

void AMXFPSGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ViewMatrices with something, just to avoid NAN warnings.
	//UpdateViewMatrices(GetWorld(), LEVELTICK_All, 0.f);
	
	PreActorTickDelegateHandle = FWorldDelegates::OnWorldPreActorTick
												 .AddUObject(this, &AMXFPSGameStateBase::UpdateViewMatrices);
}

void AMXFPSGameStateBase::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	FWorldDelegates::OnWorldPreActorTick.Remove(PreActorTickDelegateHandle);
	PreActorTickDelegateHandle.Reset();
}

FMatrix AMXFPSGameStateBase::GetPlayerViewProjMatrix() const
{
	return CachedViewMatrices.GetViewProjectionMatrix();
}

FVector AMXFPSGameStateBase::GetPlayerViewOrigin() const
{
	return CachedViewMatrices.GetViewOrigin();
}

bool AMXFPSGameStateBase::IsActorWithinPlayerFrustum(AActor* Actor, TArray<FVector>& OutSupportVertexes) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_GameState_IsActorWithinPlayerFrustum);
	
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

	FMatrix ViewProjMat = CachedViewMatrices.GetViewProjectionMatrix();
	FIntRect ConstrainedViewRect = CachedViewInitOptions.GetConstrainedViewRect();

	static FVector2D ScreenLeftDir(-1.f, 0.f);
	static FVector2D ScreenRightDir(+1.f, 0.f);
	static FVector2D ScreenUpDir(0.f, -1.f);
	static FVector2D ScreenDownDir(0.f, +1.f);
	
	FPlane RightPlane;
	ViewProjMat.GetFrustumRightPlane(RightPlane);
	
	FVector LeftSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															  ConstrainedViewRect, ScreenLeftDir);

	float LeftSignedDistance = RightPlane.PlaneDot(LeftSupportVertex);
	if (LeftSignedDistance > 0)
	{
		return false;
	}

	FPlane LeftPlane;
	ViewProjMat.GetFrustumLeftPlane(LeftPlane);

	FVector RightSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															   ConstrainedViewRect, ScreenRightDir);

	float RightSignedDistance = LeftPlane.PlaneDot(RightSupportVertex);
	if (RightSignedDistance > 0)
	{
		return false;
	}
	
	FPlane BottomPlane;
	ViewProjMat.GetFrustumBottomPlane(BottomPlane);
	
	FVector TopSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															 ConstrainedViewRect, ScreenUpDir);

	float TopSignedDistance = BottomPlane.PlaneDot(TopSupportVertex);
	if (TopSignedDistance > 0)
	{
		return false;
	}
	
	FPlane TopPlane;
	ViewProjMat.GetFrustumTopPlane(TopPlane);
	
	FVector BottomSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
																ConstrainedViewRect, ScreenDownDir);

	float BottomSignedDistance = TopPlane.PlaneDot(BottomSupportVertex);
	if (BottomSignedDistance > 0)
	{
		return false;
	}

	// Inverses are expensive, do it only if we really need it, and cache it for this frame
	if (!CachedInvViewRotMat.IsSet())
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_GameState_ComputeInvViewRotMat);
		
		CachedInvViewRotMat = CachedViewInitOptions.ViewRotationMatrix.InverseFast(); 
	}
	
	FMatrix InvViewRotMat = CachedInvViewRotMat.GetValue();

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

void AMXFPSGameStateBase::UpdateViewMatrices(UWorld* World, ELevelTick, float)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_GameState_UpdateViewMatrices);
	
	APlayerController* PlayerController = World->GetFirstPlayerController();
	ULocalPlayer* Player = PlayerController->GetLocalPlayer();

	Player->CalcSceneViewInitOptions(CachedViewInitOptions, Player->ViewportClient->Viewport, nullptr);

	CachedViewMatrices = FViewMatrices{ CachedViewInitOptions };
}

