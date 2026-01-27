#include "db/foodrepository.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <map>

#include "db/databasemanager.h"

FoodRepository::FoodRepository() {}

#include <algorithm>

#include "utils/string_utils.h"

// ...

void FoodRepository::ensureCacheLoaded() {
    if (m_cacheLoaded) return;

    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    // 1. Load Food Items with Group Names
    QSqlQuery query(
        "SELECT f.id, f.long_desc, g.fdgrp_desc, f.fdgrp_id "
        "FROM food_des f "
        "JOIN fdgrp g ON f.fdgrp_id = g.id",
        db);
    std::map<int, int> nutrientCounts;

    // 2. Load Nutrient Counts (Bulk)
    QSqlQuery countQuery("SELECT food_id, count(*) FROM nut_data GROUP BY food_id", db);
    while (countQuery.next()) {
        nutrientCounts[countQuery.value(0).toInt()] = countQuery.value(1).toInt();
    }

    // 3. Load Nutrient Definition Metadata
    m_nutrientNames.clear();
    m_nutrientUnits.clear();
    QSqlQuery defQuery("SELECT id, nutr_desc, unit FROM nutr_def", db);
    while (defQuery.next()) {
        int id = defQuery.value(0).toInt();
        m_nutrientNames[id] = defQuery.value(1).toString();
        m_nutrientUnits[id] = defQuery.value(2).toString();
    }

    while (query.next()) {
        FoodItem item;
        item.id = query.value(0).toInt();
        item.description = query.value(1).toString();
        item.foodGroupName = query.value(2).toString();
        item.foodGroupId = query.value(3).toInt();

        // Set counts from map (default 0 if not found)
        auto it = nutrientCounts.find(item.id);
        item.nutrientCount = (it != nutrientCounts.end()) ? it->second : 0;

        item.aminoCount = 0;  // TODO: Implement specific counts if needed
        item.flavCount = 0;
        item.score = 0;
        m_cache.push_back(item);
    }
    loadRdas();
    m_cacheLoaded = true;
}

void FoodRepository::loadRdas() {
    m_rdas.clear();
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlDatabase userDb = DatabaseManager::instance().userDatabase();

    // 1. Load Defaults from USDA
    if (db.isOpen()) {
        QSqlQuery query("SELECT id, rda FROM nutrients_overview", db);
        while (query.next()) {
            m_rdas[query.value(0).toInt()] = query.value(1).toDouble();
        }
    }

    // 2. Load Overrides from User DB
    if (userDb.isOpen()) {
        QSqlQuery query("SELECT nutr_id, rda FROM rda WHERE profile_id = 1", userDb);
        while (query.next()) {
            m_rdas[query.value(0).toInt()] = query.value(1).toDouble();
        }
    }
}

std::vector<FoodItem> FoodRepository::searchFoods(const QString& query) {
    ensureCacheLoaded();
    std::vector<FoodItem> results;

    if (query.trimmed().isEmpty()) return results;

    // Calculate scores
    // create a temporary list of pointers or indices to sort?
    // Copying might be expensive if cache is huge (8k items is fine though)

    // Let's iterate and keep top matches.
    struct ScoredItem {
        const FoodItem* item;
        int score;
    };
    std::vector<ScoredItem> scoredItems;
    scoredItems.reserve(m_cache.size());

    for (const auto& item : m_cache) {
        int score = Utils::calculateFuzzyScore(query, item.description);
        if (score > 40) {  // Threshold
            scoredItems.push_back({&item, score});
        }
    }

    // Sort by score desc
    std::sort(scoredItems.begin(), scoredItems.end(),
              [](const ScoredItem& a, const ScoredItem& b) { return a.score > b.score; });

    // Take top 100
    int count = 0;
    std::vector<int> resultIds;
    std::map<int, int> idToIndex;

    for (const auto& si : scoredItems) {
        if (count >= 100) break;
        FoodItem res = *si.item;
        res.score = si.score;
        // We will populate nutrients shortly
        results.push_back(res);
        resultIds.push_back(res.id);
        idToIndex[res.id] = count;
        count++;
    }

    // Batch fetch nutrient counts
    if (!resultIds.empty()) {
        QSqlDatabase db = DatabaseManager::instance().database();
        QStringList idStrings;
        for (int id : resultIds) idStrings << QString::number(id);

        QString sql = QString(
                          "SELECT n.food_id, "
                          "COUNT(n.nutr_id) as total_count, "
                          "SUM(CASE WHEN n.nutr_id BETWEEN 501 AND 521 THEN 1 ELSE 0 END) as "
                          "amino_count, "
                          "SUM(CASE WHEN d.flav_class IS NOT NULL AND d.flav_class != '' THEN 1 "
                          "ELSE 0 END) as flav_count "
                          "FROM nut_data n "
                          "JOIN nutr_def d ON n.nutr_id = d.id "
                          "WHERE n.food_id IN (%1) "
                          "GROUP BY n.food_id")
                          .arg(idStrings.join(","));

        QSqlQuery nutQuery(sql, db);
        while (nutQuery.next()) {
            int fid = nutQuery.value(0).toInt();
            int total = nutQuery.value(1).toInt();
            int amino = nutQuery.value(2).toInt();
            int flav = nutQuery.value(3).toInt();

            if (idToIndex.count(fid) != 0U) {
                auto& item = results[idToIndex[fid]];
                item.nutrientCount = total;
                item.aminoCount = amino;
                item.flavCount = flav;
            }
        }
    }

    return results;
}

std::vector<Nutrient> FoodRepository::getFoodNutrients(int foodId) {
    ensureCacheLoaded();
    std::vector<Nutrient> results;
    QSqlDatabase db = DatabaseManager::instance().database();

    if (!db.isOpen()) return results;

    QSqlQuery query(db);
    if (!query.prepare("SELECT n.nutr_id, n.nutr_val, d.nutr_desc, d.unit "
                       "FROM nut_data n "
                       "JOIN nutr_def d ON n.nutr_id = d.id "
                       "WHERE n.food_id = ?")) {
        qCritical() << "Prepare failed:" << query.lastError().text();
        return results;
    }

    query.bindValue(0, foodId);

    if (query.exec()) {
        while (query.next()) {
            Nutrient nut;
            nut.id = query.value(0).toInt();
            nut.amount = query.value(1).toDouble();
            nut.description = query.value(2).toString();
            nut.unit = query.value(3).toString();

            if (m_rdas.count(nut.id) != 0U && m_rdas[nut.id] > 0) {
                nut.rdaPercentage = (nut.amount / m_rdas[nut.id]) * 100.0;
            } else {
                nut.rdaPercentage = 0.0;
            }

            results.push_back(nut);
        }

    } else {
        qCritical() << "Nutrient query failed:" << query.lastError().text();
    }

    return results;
}

std::vector<ServingWeight> FoodRepository::getFoodServings(int foodId) {
    std::vector<ServingWeight> results;
    QSqlDatabase db = DatabaseManager::instance().database();

    if (!db.isOpen()) return results;

    QSqlQuery query(db);
    if (!query.prepare("SELECT d.msre_desc, s.grams "
                       "FROM serving s "
                       "JOIN serv_desc d ON s.msre_id = d.id "
                       "WHERE s.food_id = ?")) {
        qCritical() << "Prepare servings failed:" << query.lastError().text();
        return results;
    }

    query.bindValue(0, foodId);

    if (query.exec()) {
        while (query.next()) {
            ServingWeight sw;
            sw.description = query.value(0).toString();
            sw.grams = query.value(1).toDouble();
            results.push_back(sw);
        }
    } else {
        qCritical() << "Servings query failed:" << query.lastError().text();
    }

    return results;
}

std::map<int, double> FoodRepository::getNutrientRdas() {
    ensureCacheLoaded();
    return m_rdas;
}

void FoodRepository::updateRda(int nutrId, double value) {
    QSqlDatabase userDb = DatabaseManager::instance().userDatabase();
    if (!userDb.isOpen()) return;

    QSqlQuery query(userDb);
    if (!query.prepare("INSERT OR REPLACE INTO rda (profile_id, nutr_id, rda) "
                       "VALUES (1, ?, ?)")) {
        qCritical() << "Failed to prepare RDA update:" << query.lastError().text();
        return;
    }
    query.bindValue(0, nutrId);
    query.bindValue(1, value);

    if (query.exec()) {
        m_rdas[nutrId] = value;
    } else {
        qCritical() << "Failed to update RDA:" << query.lastError().text();
    }
}

QString FoodRepository::getNutrientName(int nutrientId) {
    ensureCacheLoaded();
    if (m_nutrientNames.count(nutrientId) != 0U) {
        return m_nutrientNames[nutrientId];
    }
    return QString("Unknown Nutrient (%1)").arg(nutrientId);
}

QString FoodRepository::getNutrientUnit(int nutrientId) {
    ensureCacheLoaded();
    if (m_nutrientUnits.count(nutrientId) != 0U) {
        return m_nutrientUnits[nutrientId];
    }
    return "?";
}
