#ifndef TEST_CALCULATIONS_H
#define TEST_CALCULATIONS_H

#include <QObject>
#include <QtTest>

class TestCalculations : public QObject {
    Q_OBJECT

private slots:
    void testBMR();
    void testBodyFat();
};

#endif  // TEST_CALCULATIONS_H
