#ifndef LW_COLLISION_C_API_H
#define LW_COLLISION_C_API_H

#define PL_DECLARE_HANDLE(name) \
	typedef struct name##__     \
	{                           \
		int unused;             \
	} * name

#ifdef BT_USE_DOUBLE_PRECISION
typedef double plReal;
#else
typedef float plReal;
#endif

typedef plReal plVector3[3];
typedef plReal plQuaternion[4];

#ifdef __cplusplus
extern "C"
{
#endif

	/**     Particular collision SDK (C-API) */
	PL_DECLARE_HANDLE(plCollisionSdkHandle);

	/**     Collision world, belonging to some collision SDK (C-API)*/
	PL_DECLARE_HANDLE(plCollisionWorldHandle);

	/** Collision object that can be part of a collision World (C-API)*/
	PL_DECLARE_HANDLE(plCollisionObjectHandle);

	/**     Collision Shape/Geometry, property of a collision object (C-API)*/
	PL_DECLARE_HANDLE(plCollisionShapeHandle);

	/* Collision SDK */

	extern plCollisionSdkHandle plCreateBullet2CollisionSdk();

#ifndef DISABLE_REAL_TIME_BULLET3_COLLISION_SDK
	extern plCollisionSdkHandle plCreateRealTimeBullet3CollisionSdk();
#endif  //DISABLE_REAL_TIME_BULLET3_COLLISION_SDK

	//	extern plCollisionSdkHandle plCreateCustomCollisionSdk();

	extern void plDeleteCollisionSdk(plCollisionSdkHandle collisionSdkHandle);

	//extern int plGetSdkWorldCreationIntParameter();
	//extern int plSetSdkWorldCreationIntParameter(int newValue);

	/* Collision World */

	extern plCollisionWorldHandle plCreateCollisionWorld(plCollisionSdkHandle collisionSdkHandle, int maxNumObjsCapacity, int maxNumShapesCapacity, int maxNumPairsCapacity);
	extern void plDeleteCollisionWorld(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle world);

	extern void plAddCollisionObject(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle world, plCollisionObjectHandle object);
	extern void plRemoveCollisionObject(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle world, plCollisionObjectHandle object);

	/* Collision Object  */

	extern plCollisionObjectHandle plCreateCollisionObject(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, void* userPointer, int userIndex, plCollisionShapeHandle cshape, plVector3 startPosition, plQuaternion startOrientation);
	extern void plDeleteCollisionObject(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, plCollisionObjectHandle body);

	extern void plSetCollisionObjectTransform( plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, plCollisionObjectHandle objHandle, plVector3 startPosition, plQuaternion startOrientation);
	// addition protocol
	extern void plSetCollisionObjectPosition( plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, plCollisionObjectHandle bodyHandle,
												 plVector3 position );
	extern void plSetCollisionObjectRotation( plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, plCollisionObjectHandle bodyHandle,
												 plQuaternion orientation );


	/* Collision Shape definition */
	// addition staple shape: box 
	extern plCollisionShapeHandle plCreateCubeShape(plCollisionSdkHandle sdk,plCollisionWorldHandle world,plVector3 size);

	extern plCollisionShapeHandle plCreateSphereShape( plCollisionSdkHandle sdk, plCollisionWorldHandle worldHandle, plReal radius);
	extern plCollisionShapeHandle plCreateCapsuleShape(plCollisionSdkHandle sdk, plCollisionWorldHandle worldHandle, plReal radius, plReal height, int capsuleAxis);
	extern plCollisionShapeHandle plCreatePlaneShape(plCollisionSdkHandle sdk, plCollisionWorldHandle worldHandle,
													 plReal planeNormalX,
													 plReal planeNormalY,
													 plReal planeNormalZ,
													 plReal planeConstant);
	extern plCollisionShapeHandle plCreateCompoundShape(plCollisionSdkHandle sdk, plCollisionWorldHandle worldHandle);
	extern void plAddChildShape(plCollisionSdkHandle collisionSdkHandle, plCollisionWorldHandle worldHandle, plCollisionShapeHandle compoundShape, plCollisionShapeHandle childShape, plVector3 childPos, plQuaternion childOrn);

	extern void plDeleteShape(plCollisionSdkHandle collisionSdkHandle, plCollisionWorldHandle worldHandle, plCollisionShapeHandle shape);

	/* Contact Results */

	struct lwContactPoint
	{
		plVector3 m_ptOnAWorld;
		plVector3 m_ptOnBWorld;
		plVector3 m_normalOnB;
		plReal m_distance;
	};

	struct ClosestRayResult {
		// todo: hitInfo for lua user 
		// plCollisionObjectHandle
		//		  m_hitObjHandle;
		int       m_hitObjId;
		plReal 	  m_hitFraction;
		plVector3 m_hitPointWorld;
		plVector3 m_hitNormalWorld;
		int 	  m_filterGroup;
		int 	  m_filterMask;
		int 	  m_flags;
	};

	
	//typedef btCollisionWorld::ClosestRayResultCallback ClosestRayResult;

	/* Collision Filtering */
	typedef void (*plNearCallback)(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, void* userData,
								   plCollisionObjectHandle objA, plCollisionObjectHandle objB);

    // add new Collision Filtering,to avoid global variable for user 
	typedef void (*plNearCallback_L)(plCollisionSdkHandle sdkHandle,plCollisionWorldHandle worldHandle, int userId,void* userData,
								   plCollisionObjectHandle objA, plCollisionObjectHandle objB, int &totalPoints,int &numCallbacks);

	/* Collision Queries */
	extern int plCollide(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle worldHandle, plCollisionObjectHandle colA, plCollisionObjectHandle colB,
						 lwContactPoint* pointsOut, int pointCapacity);

	extern void plWorldCollide(plCollisionSdkHandle sdkHandle, plCollisionWorldHandle world,
							   plNearCallback filter, void* userData);
	
	// add new collideWorld, avoid global variables for user 10.11
	extern void plWorldCollide_L( plCollisionWorldHandle world,
							    plNearCallback_L filter,void *userData);

	extern bool plRaycast( plCollisionSdkHandle SdkHandle, plCollisionWorldHandle world, 
							  plVector3 rayFrom, plVector3 rayTo, ClosestRayResult &result);


#ifdef __cplusplus
}
#endif

#endif  //LW_COLLISION_C_API_H
