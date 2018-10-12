#include <gtest/gtest.h>

#include <QEventLoop>

#include "Base/ServiceLocator.h"
#include "Base/GlobalFactory.h"
#include "Base/NumberGenerator.h"
#include "ModelBasic/Settings.h"
#include "ModelBasic/ModelBasicBuilderFacade.h"
#include "ModelBasic/SimulationContext.h"
#include "ModelBasic/SimulationController.h"
#include "ModelBasic/DescriptionHelper.h"
#include "ModelBasic/SimulationParameters.h"
#include "ModelBasic/SpaceProperties.h"
#include "ModelBasic/SimulationAccess.h"
#include "ModelBasic/Serializer.h"

#include "Tests/Predicates.h"

#include "IntegrationTestHelper.h"
#include "IntegrationTestFramework.h"

class ClusterSizeTest
	: public IntegrationTestFramework
{
public:
	ClusterSizeTest();
	~ClusterSizeTest();

protected:
	SimulationController* _controller = nullptr;
	SimulationContext* _context = nullptr;
	SpaceProperties* _space = nullptr;
	SimulationAccess* _access = nullptr;
	IntVector2D _gridSize{ 12, 6 };
};

ClusterSizeTest::ClusterSizeTest()
	: IntegrationTestFramework({ 600, 300 })
{
	GlobalFactory* factory = ServiceLocator::getInstance().getService<GlobalFactory>();
	_controller = _facade->buildSimulationController(1, _gridSize, _universeSize, _symbols, _parameters);
	_context = _controller->getContext();
	_space = _context->getSpaceProperties();
	_access = _facade->buildSimulationAccess();
	_access->init(_context);
}

ClusterSizeTest::~ClusterSizeTest()
{
	delete _access;
	delete _controller;
}

TEST_F(ClusterSizeTest, testDistanceToNeighbors)
{
	DataDescription data;
	for (int i = 1; i <= 10000; ++i) {
		data.addParticle(createParticleDescription());
	}
	_access->updateData(data);

	runSimulation(50, _controller);

	//check result
	DataDescription extract = IntegrationTestHelper::getContent(_access, { { 0, 0 }, _universeSize });
	DescriptionNavigator navi;
	navi.update(extract);
	if (extract.clusters) {
		for (auto const& cluster : *extract.clusters) {
			if (cluster.cells) {
				for (auto const& cell : *cluster.cells) {
					if (cell.connectingCells) {
						for (uint64_t connectingCellId : *cell.connectingCells) {
							auto const& connectingCell = cluster.cells->at(navi.cellIndicesByCellIds.at(connectingCellId));
							auto distance = *cell.pos - *connectingCell.pos;
							_space->correctDisplacement(distance);
							ASSERT_TRUE(predLessThanMediumPrecision(distance.length(), _parameters->cellMaxDistance));
						}
					}
				}
			}
		}
	}
}
