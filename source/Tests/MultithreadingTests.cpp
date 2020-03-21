#include <gtest/gtest.h>

#include <QEventLoop>

#include "Base/ServiceLocator.h"
#include "Base/GlobalFactory.h"
#include "Base/NumberGenerator.h"
#include "ModelBasic/ModelBasicBuilderFacade.h"
#include "ModelBasic/Settings.h"
#include "ModelBasic/SimulationController.h"
#include "ModelBasic/SimulationParameters.h"
#include "ModelCpu/SimulationContextCpuImpl.h"
#include "ModelCpu/SimulationAccessCpu.h"
#include "ModelCpu/UnitGrid.h"
#include "ModelCpu/Unit.h"
#include "ModelCpu/UnitContext.h"
#include "ModelCpu/MapCompartment.h"
#include "ModelCpu/UnitThreadControllerImpl.h"
#include "ModelCpu/UnitThread.h"
#include "ModelCpu/SimulationControllerCpu.h"
#include "ModelCpu/ModelCpuBuilderFacade.h"
#include "ModelCpu/ModelCpuData.h"

#include "tests/Predicates.h"
#include "IntegrationTestHelper.h"

class MultithreadingTests : public ::testing::Test
{
public:
	MultithreadingTests();
	~MultithreadingTests();

protected:

	SimulationControllerCpu* _controller = nullptr;
	SimulationContextCpuImpl* _context = nullptr;
	SimulationParameters _parameters;
	UnitThreadControllerImpl* _threadController = nullptr;
	NumberGenerator* _numberGen = nullptr;
	IntVector2D _gridSize{ 6, 6 };
	int _threads = 4;
	IntVector2D _universeSize{ 600, 300 };
};

MultithreadingTests::MultithreadingTests()
{
	auto basicFacade = ServiceLocator::getInstance().getService<ModelBasicBuilderFacade>();
	auto cpuFacade = ServiceLocator::getInstance().getService<ModelCpuBuilderFacade>();

	GlobalFactory* factory = ServiceLocator::getInstance().getService<GlobalFactory>();
	auto symbols = basicFacade->buildDefaultSymbolTable();
	_parameters = basicFacade->buildDefaultSimulationParameters();
	_controller = cpuFacade->buildSimulationController({ _universeSize, symbols, _parameters }, ModelCpuData(_threads, _gridSize));
	_context = static_cast<SimulationContextCpuImpl*>(_controller->getContext());
	_threadController = static_cast<UnitThreadControllerImpl*>(_context->getUnitThreadController());
	_numberGen = _context->getNumberGenerator();
}

MultithreadingTests::~MultithreadingTests()
{
	delete _controller;
}

TEST_F(MultithreadingTests, testThreads)
{
	QEventLoop pause;
	_controller->connect(_controller, &SimulationController::nextTimestepCalculated, &pause, &QEventLoop::quit);
	_controller->calculateSingleTimestep();
	pause.exec();

	for (auto const& threadAndCalcSignal : _threadController->_threadsAndCalcSignals) {
		ASSERT_TRUE(threadAndCalcSignal.thr->isFinished()) << "One thread is not finished.";
	}
}


TEST_F(MultithreadingTests, testOneCellMovement)
{
	auto facade = ServiceLocator::getInstance().getService<ModelCpuBuilderFacade>();
	auto access = facade->buildSimulationAccess();
	access->init(_controller);

	_parameters.radiationProb = 0.0;

	DataChangeDescription desc;
	desc.addNewCluster(ClusterChangeDescription().setPos({ 100, 50 }).setVel({ 1.0, 0.5 })
		.addNewCell(CellChangeDescription().setEnergy(_parameters.cellFunctionConstructorOffspringCellEnergy)));
	access->updateData(desc);

	IntegrationTestHelper::runSimulation(300, _controller);

	IntRect rect = { { 0, 0 }, { _universeSize.x - 1, _universeSize.y - 1 } };
	DataDescription data = IntegrationTestHelper::getContent(access, rect);

	ASSERT_TRUE(data.clusters);
	ASSERT_EQ(1, data.clusters->size()) << "Wrong number of clusters.";
	auto const& cluster = data.clusters->at(0);
	ASSERT_PRED_FORMAT2(predEqualVectorMediumPrecision, QVector2D(400, 200), *cluster.pos);

	delete access;
}


TEST_F(MultithreadingTests, testManyCellsMovement)
{
	auto facade = ServiceLocator::getInstance().getService<ModelCpuBuilderFacade>();
	auto access = facade->buildSimulationAccess();
	access->init(_controller);

	DataChangeDescription desc;
	for (int i = 0; i < 10000; ++i) {
		desc.addNewCluster(ClusterChangeDescription().setPos(QVector2D( _numberGen->getRandomInt(_universeSize.x), _numberGen->getRandomInt(_universeSize.y) ))
			.setVel(QVector2D(_numberGen->getRandomReal() - 0.5, _numberGen->getRandomReal() - 0.5 ))
			.addNewCell(CellChangeDescription().setEnergy(_parameters.cellFunctionConstructorOffspringCellEnergy).setMaxConnections(4)));
	}
	access->updateData(desc);

	IntegrationTestHelper::runSimulation(300, _controller);

	delete access;
}
