#pragma once

#include <cooperative_groups.h>
#include "cuda_runtime_api.h"
#include "sm_60_atomic_functions.h"

#include "AccessTOs.cuh"
#include "Base.cuh"
#include "Map.cuh"
#include "CellProcessor.cuh"
#include "ParticleProcessor.cuh"
#include "TokenProcessor.cuh"
#include "GarbageCollectorKernels.cuh"
#include "Operation.cuh"
#include "DebugKernels.cuh"
#include "SimulationResult.cuh" 

__global__ extern void processingStep1(SimulationData data);

__global__ extern void processingStep2(SimulationData data);

__global__ extern void processingStep3(SimulationData data);

__global__ extern void processingStep4(SimulationData data);

__global__ extern void processingStep5(SimulationData data);

__global__ extern void processingStep6(SimulationData data, SimulationResult result);

__global__ extern void processingStep7(SimulationData data);

__global__ extern void processingStep8(SimulationData data, SimulationResult result);

__global__ extern void processingStep9(SimulationData data);

__global__ extern void processingStep10(SimulationData data);

__global__ extern void processingStep11(SimulationData data);

__global__ extern void processingStep12(SimulationData data);

__global__ extern void processingStep13(SimulationData data);

__global__ extern void prepareForNextTimestep(SimulationData data, SimulationResult result);

