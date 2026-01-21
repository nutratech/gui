#include "db/mealrepository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "db/databasemanager.h"

MealRepository::MealRepository() = default;

void MealRepository::ensureMealNamesLoaded() {
    if (!m_mealNamesCache.empty()) return;

    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    QSqlQuery query("SELECT id, name FROM meal_name", db);
    while (query.next()) {
        m_mealNamesCache[query.value(0).toInt()] = query.value(1).toString();
    }
}

std::map<int, QString> MealRepository::getMealNames() {
    ensureMealNamesLoaded();
    return m_mealNamesCache;
}

void MealRepository::addFoodLog(int foodId, double grams, int mealId, QDate date) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    // Use current time if today, otherwise noon of target date
    qint64 timestamp;
    if (date == QDate::currentDate()) {
        timestamp = QDateTime::currentSecsSinceEpoch();
    } else {
        timestamp = QDateTime(date, QTime(0, 0, 0)).toSecsSinceEpoch() + 43200;  // Noon
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO log_food (profile_id, date, meal_id, food_id, msre_id, amt) "
        "VALUES (1, ?, ?, ?, 0, ?)");  // msre_id 0 for default/grams
    query.addBindValue(timestamp);
    query.addBindValue(mealId);
    query.addBindValue(foodId);
    query.addBindValue(grams);

    if (!query.exec()) {
        qCritical() << "Failed to add food log:" << query.lastError().text();
    }
}

std::vector<MealLogItem> MealRepository::getDailyLogs(QDate date) {
    std::vector<MealLogItem> results;
    QSqlDatabase userDb = DatabaseManager::instance().userDatabase();
    QSqlDatabase mainDb = DatabaseManager::instance().database();

    if (!userDb.isOpen()) return results;

    ensureMealNamesLoaded();

    qint64 startOfDay = QDateTime(date, QTime(0, 0, 0)).toSecsSinceEpoch();
    qint64 endOfDay = QDateTime(date, QTime(23, 59, 59)).toSecsSinceEpoch();

    QSqlQuery query(userDb);
    query.prepare(
        "SELECT id, food_id, meal_id, amt FROM log_food "
        "WHERE date >= ? AND date <= ? AND profile_id = 1");
    query.addBindValue(startOfDay);
    query.addBindValue(endOfDay);

    std::vector<int> foodIds;

    if (query.exec()) {
        while (query.next()) {
            MealLogItem item;
            item.id = query.value(0).toInt();
            item.foodId = query.value(1).toInt();
            item.mealId = query.value(2).toInt();
            item.grams = query.value(3).toDouble();

            if (m_mealNamesCache.count(item.mealId) != 0U) {
                item.mealName = m_mealNamesCache[item.mealId];
            } else {
                item.mealName = "Unknown";
            }

            results.push_back(item);
            foodIds.push_back(item.foodId);
        }
    } else {
        qCritical() << "Failed to fetch daily logs:" << query.lastError().text();
    }

    // Hydrate food names from Main DB
    if (!foodIds.empty() && mainDb.isOpen()) {
        QStringList idStrings;
        for (int id : foodIds) idStrings << QString::number(id);

        // Simple name fetch
        // Optimization: Could use FoodRepository cache if available, but direct
        // query is safe here
        QString sql =
            QString("SELECT id, long_desc FROM food_des WHERE id IN (%1)").arg(idStrings.join(","));
        QSqlQuery nameQuery(sql, mainDb);

        std::map<int, QString> names;
        while (nameQuery.next()) {
            names[nameQuery.value(0).toInt()] = nameQuery.value(1).toString();
        }

        for (auto& item : results) {
            if (names.count(item.foodId) != 0U) {
                item.foodName = names[item.foodId];
            } else {
                item.foodName = "Unknown Food";  // Should not happen if DBs consistent
            }
        }
    }

    return results;
}

void MealRepository::clearDailyLogs(QDate date) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    qint64 startOfDay = QDateTime(date, QTime(0, 0, 0)).toSecsSinceEpoch();
    qint64 endOfDay = QDateTime(date, QTime(23, 59, 59)).toSecsSinceEpoch();

    QSqlQuery query(db);
    query.prepare("DELETE FROM log_food WHERE date >= ? AND date <= ? AND profile_id = 1");
    query.addBindValue(startOfDay);
    query.addBindValue(endOfDay);

    if (!query.exec()) {
        qCritical() << "Failed to clear daily logs:" << query.lastError().text();
    }
}

void MealRepository::removeLogEntry(int logId) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("DELETE FROM log_food WHERE id = ?");
    query.addBindValue(logId);
    query.addBindValue(logId);
    if (!query.exec()) {
        qCritical() << "Failed to remove log entry:" << query.lastError().text();
    }
}
