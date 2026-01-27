#ifndef MEALREPOSITORY_H
#define MEALREPOSITORY_H

#include <QDate>
#include <QString>
#include <map>
#include <vector>

struct MealLogItem {
    int id;  // log_food.id
    int foodId;
    double grams;
    int mealId;
    QString mealName;
    // Potentially cached description?
    QString foodName;  // Joined from food_des if we query it
};

class MealRepository {
public:
    MealRepository();

    // Meal Names (Breakfast, Lunch, etc.)
    std::map<int, QString> getMealNames();

    // Logging
    void addFoodLog(int foodId, double grams, int mealId, QDate date = QDate::currentDate());
    std::vector<MealLogItem> getDailyLogs(QDate date = QDate::currentDate());
    void clearDailyLogs(QDate date = QDate::currentDate());
    void removeLogEntry(int logId);

private:
    std::map<int, QString> m_mealNamesCache;
    void ensureMealNamesLoaded();
};

#endif  // MEALREPOSITORY_H
