#include "db/foodrepository.h"
#include "db/databasemanager.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <map>

FoodRepository::FoodRepository() {}

#include "utils/string_utils.h"
#include <algorithm>

// ...

void FoodRepository::ensureCacheLoaded() {
  if (m_cacheLoaded)
    return;

  QSqlDatabase db = DatabaseManager::instance().database();
  if (!db.isOpen())
    return;

  // 1. Load Food Items
  QSqlQuery query("SELECT id, long_desc, fdgrp_id FROM food_des", db);
  std::map<int, int> nutrientCounts;

  // 2. Load Nutrient Counts (Bulk)
  QSqlQuery countQuery(
      "SELECT food_id, count(*) FROM nut_data GROUP BY food_id", db);
  while (countQuery.next()) {
    nutrientCounts[countQuery.value(0).toInt()] = countQuery.value(1).toInt();
  }

  while (query.next()) {
    FoodItem item;
    item.id = query.value(0).toInt();
    item.description = query.value(1).toString();
    item.foodGroupId = query.value(2).toInt();

    // Set counts from map (default 0 if not found)
    auto it = nutrientCounts.find(item.id);
    item.nutrientCount = (it != nutrientCounts.end()) ? it->second : 0;

    item.aminoCount = 0; // TODO: Implement specific counts if needed
    item.flavCount = 0;
    item.score = 0;
    m_cache.push_back(item);
  }
  m_cacheLoaded = true;
}

std::vector<FoodItem> FoodRepository::searchFoods(const QString &query) {
  ensureCacheLoaded();
  std::vector<FoodItem> results;

  if (query.trimmed().isEmpty())
    return results;

  // Calculate scores
  // create a temporary list of pointers or indices to sort?
  // Copying might be expensive if cache is huge (8k items is fine though)

  // Let's iterate and keep top matches.
  struct ScoredItem {
    const FoodItem *item;
    int score;
  };
  std::vector<ScoredItem> scoredItems;
  scoredItems.reserve(m_cache.size());

  for (const auto &item : m_cache) {
    int score = Utils::calculateFuzzyScore(query, item.description);
    if (score > 40) { // Threshold
      scoredItems.push_back({&item, score});
    }
  }

  // Sort by score desc
  std::sort(scoredItems.begin(), scoredItems.end(),
            [](const ScoredItem &a, const ScoredItem &b) {
              return a.score > b.score;
            });

  // Take top 100
  int count = 0;
  std::vector<int> resultIds;
  std::map<int, int> idToIndex;

  for (const auto &si : scoredItems) {
    if (count >= 100)
      break;
    FoodItem res = *si.item;
    res.score = si.score;
    // We will populate nutrients shortly
    results.push_back(res);
    resultIds.push_back(res.id);
    idToIndex[res.id] = count;
    count++;
  }

  // Batch fetch nutrients for these results
  if (!resultIds.empty()) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QStringList idStrings;
    for (int id : resultIds)
      idStrings << QString::number(id);

    QString sql =
        QString("SELECT n.food_id, n.nutr_id, n.nutr_val, d.nutr_desc, d.unit "
                "FROM nut_data n "
                "JOIN nutr_def d ON n.nutr_id = d.id "
                "WHERE n.food_id IN (%1)")
            .arg(idStrings.join(","));

    QSqlQuery nutQuery(sql, db);
    while (nutQuery.next()) {
      int fid = nutQuery.value(0).toInt();
      Nutrient nut;
      nut.id = nutQuery.value(1).toInt();
      nut.amount = nutQuery.value(2).toDouble();
      nut.description = nutQuery.value(3).toString();
      nut.unit = nutQuery.value(4).toString();
      nut.rdaPercentage = 0.0;

      if (idToIndex.count(fid) != 0U) {
        results[idToIndex[fid]].nutrients.push_back(nut);
      }
    }

    // Update counts based on actual data
    for (auto &res : results) {
      res.nutrientCount = static_cast<int>(res.nutrients.size());
      // TODO: Logic for amino/flav counts if we have ranges of IDs
    }
  }

  return results;
}

std::vector<Nutrient> FoodRepository::getFoodNutrients(int foodId) {
  std::vector<Nutrient> results;
  QSqlDatabase db = DatabaseManager::instance().database();

  if (!db.isOpen())
    return results;

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
      nut.rdaPercentage = 0.0;

      results.push_back(nut);
    }

  } else {
    qCritical() << "Nutrient query failed:" << query.lastError().text();
  }

  return results;
}
