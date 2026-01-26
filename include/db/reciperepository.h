#ifndef RECIPEREPOSITORY_H
#define RECIPEREPOSITORY_H

#include <QDateTime>
#include <QString>
#include <map>
#include <vector>

struct RecipeItem {
    int id;
    QString uuid;
    QString name;
    QString instructions;
    QDateTime created;
    double totalCalories = 0.0;  // Calculated on the fly ideally
};

struct RecipeIngredient {
    int foodId;
    QString foodName;
    double amount;  // grams
    // Potential for more info (calories contribution etc)
};

class RecipeRepository {
public:
    RecipeRepository();

    // CRUD
    int createRecipe(const QString& name, const QString& instructions = "");
    bool updateRecipe(int id, const QString& name, const QString& instructions);
    bool deleteRecipe(int id);

    std::vector<RecipeItem> getAllRecipes();
    RecipeItem getRecipe(int id);

    void loadCsvRecipes(const QString& directory);

    // Ingredients
    bool addIngredient(int recipeId, int foodId, double amount);
    bool removeIngredient(int recipeId, int foodId);
    bool updateIngredient(int recipeId, int foodId, double amount);
    std::vector<RecipeIngredient> getIngredients(int recipeId);

private:
    void processCsvFile(const QString& filePath, std::map<QString, int>& recipeMap);
    int getOrCreateRecipe(const QString& name, const QString& instructions,
                          std::map<QString, int>& recipeMap);
};

#endif  // RECIPEREPOSITORY_H
