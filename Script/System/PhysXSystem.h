#ifndef _PHYSX_SYSTEM_H_
#define _PHYSX_SYSTEM_H_

using namespace physx;

#include "PhysXEventCallback.h"

class PhysXSystem final
{

private:

	PhysXSystem() = default;
	~PhysXSystem() = default;

	PhysXSystem(const PhysXSystem& _rhs) = delete;
	PhysXSystem& operator = (const PhysXSystem& _rhs) = delete;
	PhysXSystem(PhysXSystem&& _rhs) = delete;
	PhysXSystem& operator = (PhysXSystem&& _rhs) = delete;

public:
	
	static PhysXSystem* Instance()
	{
		static PhysXSystem instance;
		return &instance;
	}

	void Initialize();
	void Update(float _fixedDeltaTime);
	void Release();

public:

	inline PxPhysics* GetPhysics()
	{
		return mPhysics;
	}

	inline PxScene* GetScene()
	{
		return mScene;
	}

private:

	PxDefaultAllocator mDefaultAllocator;
	PxDefaultErrorCallback mDefaultErrorCallback;
	
	PxScene* mScene;
	PxFoundation*  mFoundation = nullptr;
	PxPhysics* mPhysics = nullptr;
	PxCooking* mCooking = nullptr;
	PxPvdTransport* mPvdTransport = nullptr;
	PxTolerancesScale mTolerancesScale;
	PxControllerManager* mControllerManager = nullptr;
	PxDefaultCpuDispatcher* mCpuDispatcher = nullptr;
	PxCudaContextManager* mCudaContextManager= nullptr;
	PxCudaContextManagerDesc mCudaContextManagerDesc;

	PhysXEventCallback mEventCallback;
	PhysXFillterCallback mFilterCallback;

};
#endif _PHYSX_SYSTEM_H_