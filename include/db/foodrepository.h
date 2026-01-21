#ifndef FOODREPOSITORY_H
#define FOODREPOSITORY_H

#include <QString>
#include <QVariantMap>
#include <vector>

struct Nutrient {
  int id;
  QString description;
  double amount;
  QString unit;
  double rdaPercentage; // Calculated
};

struct ServingWeight {
  QString description;
  double grams;
};

struct FoodItem {
  int id;
  QString description;
  QString foodGroupName;
  int nutrientCount;
  int aminoCount;
  int flavCount;
  int score;                       // For search results
  std::vector<Nutrient> nutrients; // Full details for results
};

class FoodRepository {
public:
  explicit FoodRepository();

  // Search foods by keyword
  std::vector<FoodItem> searchFoods(const QString &query);

  // Get detailed nutrients for a generic food (100g)
  // Returns a list of nutrients
  std::vector<Nutrient> getFoodNutrients(int foodId);

  // Get available serving weights (units) for a food
  std::vector<ServingWeight> getFoodServings(int foodId);

  // RDA methods
  std::map<int, double> getNutrientRdas();
  void updateRda(int nutrId, double value);

  // Helper to get nutrient definition basics if needed
  // QString getNutrientName(int nutrientId);

private:
  // Internal helper methods
  void ensureCacheLoaded();
  void loadRdas();

  bool m_cacheLoaded = false;
  // Cache stores basic food info
  std::vector<FoodItem> m_cache;
  std::map<int, double> m_rdas;
};

#endif // FOODREPOSITORY_H
