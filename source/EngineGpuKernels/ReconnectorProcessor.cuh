#pragma once


#include "EngineInterface/CellFunctionConstants.h"

#include "CellFunctionProcessor.cuh"
#include "ConstantMemory.cuh"
#include "Object.cuh"
#include "ParticleProcessor.cuh"
#include "SimulationData.cuh"
#include "SimulationStatistics.cuh"
#include "CellConnectionProcessor.cuh"

class ReconnectorProcessor
{
public:
    __inline__ __device__ static void process(SimulationData& data, SimulationStatistics& result);

private:
    __inline__ __device__ static void processCell(SimulationData& data, SimulationStatistics& statistics, Cell* cell);

    __inline__ __device__ static void tryEstablishConnection(SimulationData& data, SimulationStatistics& statistics, Cell* cell, Activity& activity);
    __inline__ __device__ static void removeConnections(SimulationData& data, SimulationStatistics& statistics, Cell* cell, Activity& activity);
};

/************************************************************************/
/* Implementation                                                       */
/************************************************************************/

__device__ __inline__ void ReconnectorProcessor::process(SimulationData& data, SimulationStatistics& result)
{
    auto& operations = data.cellFunctionOperations[CellFunction_Reconnector];
    auto partition = calcAllThreadsPartition(operations.getNumEntries());
    for (int i = partition.startIndex; i <= partition.endIndex; ++i) {
        processCell(data, result, operations.at(i).cell);
    }
}

__device__ __inline__ void ReconnectorProcessor::processCell(SimulationData& data, SimulationStatistics& statistics, Cell* cell)
{
    auto activity = CellFunctionProcessor::calcInputActivity(cell);
    if (activity.channels[0] >= 0.1f) {
        tryEstablishConnection(data, statistics, cell, activity);
    } else if (activity.channels[0] <= -0.1f) {
        removeConnections(data, statistics, cell, activity);
    }
    CellFunctionProcessor::setActivity(cell, activity);
}

__inline__ __device__ void
ReconnectorProcessor::tryEstablishConnection(SimulationData& data, SimulationStatistics& statistics, Cell* cell, Activity& activity)
{
    Cell* closestCell = nullptr;
    float closestDistance = 0;
    data.cellMap.executeForEach(cell->pos, 2.0f, cell->detached, [&](auto const& otherCell) {
        if (cell->creatureId != 0 && otherCell->creatureId == cell->creatureId) {
            return;
        }
        if (CellConnectionProcessor::isConnectedConnected(cell, otherCell)) {
            return;
        }
        if (otherCell->barrier) {
            return;
        }
        auto distance = data.cellMap.getDistance(cell->pos, otherCell->pos);
        if (!closestCell || distance < closestDistance) {
            closestCell = otherCell;
            closestDistance = distance;
        }
    });

    activity.channels[1] = 0;
    if (closestCell) {
        SystemDoubleLock lock;
        lock.init(&cell->locked, &closestCell->locked);
        if (lock.tryLock()) {
            closestCell->maxConnections = min(max(closestCell->maxConnections, closestCell->numConnections + 1), MAX_CELL_BONDS);
            cell->maxConnections = min(max(cell->maxConnections, cell->numConnections + 1), MAX_CELL_BONDS);
            CellConnectionProcessor::scheduleAddConnectionPair(data, cell, closestCell);
            lock.releaseLock();
            activity.channels[1] = 1;
        }
    }
}

__inline__ __device__ void ReconnectorProcessor::removeConnections(SimulationData& data, SimulationStatistics& statistics, Cell* cell, Activity& activity)
{
    if (cell->tryLock()) {
        for (int i = 0; i < cell->numConnections; ++i) {
            auto connectedCell = cell->connections[i].cell;
            if (connectedCell->creatureId != cell->creatureId) {
                CellConnectionProcessor::scheduleDeleteConnectionPair(data, cell, connectedCell);
            }
        }
        cell->releaseLock();
    }
}
