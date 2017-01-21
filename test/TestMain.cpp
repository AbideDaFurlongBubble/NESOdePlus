#include "Test.h"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::GTEST_FLAG(filter) = "*EmulationSpeedTest*:*Gen.*";
	//::testing::GTEST_FLAG(filter) = "-*EmulationSpeedTest.*";
	int rtn = RUN_ALL_TESTS();
	system("pause");
	return rtn;
}