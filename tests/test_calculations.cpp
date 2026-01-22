#include <QtTest>

class TestCalculations : public QObject {
    Q_OBJECT

private slots:
    void testBMR() {
        // TDD: Fail mainly because not implemented
        QEXPECT_FAIL("", "BMR calculation not yet implemented", Continue);
        QVERIFY(false);
    }

    void testBodyFat() {
        QEXPECT_FAIL("", "Body Fat calculation not yet implemented", Continue);
        QVERIFY(false);
    }
};

QTEST_MAIN(TestCalculations)
#include "test_calculations.moc"
