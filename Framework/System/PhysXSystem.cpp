#include "PhysXSystem.h"

void PhysXSystem::Initialize()
{
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocator, mDefaultErrorCallback);
	assert(mFoundation);

	mTolerancesScale.length = 1.0f;
	mTolerancesScale.speed = 10.0f;

	mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(mTolerancesScale));
	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mTolerancesScale, true, nullptr);
	assert(mCooking);
	assert(mPhysics);

	PxInitVehicleSDK(*mPhysics);
	physx::PxVehicleSetBasisVectors(PxVec3(0.0f, 1.0f, 0.0f), PxVec3(0.0f, 0.0f, 1.0f));
	physx::PxVehicleSetUpdateMode(physx::PxVehicleUpdateMode::eVELOCITY_CHANGE);

	mCpuDispatcher = PxDefaultCpuDispatcherCreate(2);
	mCudaContextManager = PxCreateCudaContextManager(*mFoundation, mCudaContextManagerDesc);

	PxSceneDesc sceneDesc(mTolerancesScale);

	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = mCpuDispatcher;
	sceneDesc.cudaContextManager = mCudaContextManager;
	sceneDesc.filterShader = PhysXFilterShader;
	sceneDesc.simulationEventCallback = &mEventCallback;
	sceneDesc.filterCallback = &mFilterCallback;
	mScene = mPhysics->createScene(sceneDesc);
	
	mControllerManager = PxCreateControllerManager(*mScene);
}

void PhysXSystem::Update(float _fixedDeltaTime)
{
	mScene->simulate(_fixedDeltaTime);
	mScene->fetchResults(true);
}

void PhysXSystem::Release()
{
	mScene->release();
	mScene = nullptr;

	mControllerManager->release();
	mControllerManager = nullptr;

	mPhysics->release();
	mCooking->release();
	mFoundation->release();
}
