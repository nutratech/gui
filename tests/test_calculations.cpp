#include "test_calculations.h"

void TestCalculations::testBMR() {
    // TDD: Fail mainly because not implemented
    QEXPECT_FAIL("", "BMR calculation not yet implemented", Continue);
    QVERIFY(false);
}

void TestCalculations::testBodyFat() {
    QEXPECT_FAIL("", "Body Fat calculation not yet implemented", Continue);
    QVERIFY(false);
}

QTEST_MAIN(TestCalculations)
