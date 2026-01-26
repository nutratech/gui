#include "db/reciperepository.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "db/databasemanager.h"

RecipeRepository::RecipeRepository() = default;

int RecipeRepository::createRecipe(const QString& name, const QString& instructions) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return -1;

    QSqlQuery query(db);
    query.prepare("INSERT INTO recipe (name, instructions) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(instructions);

    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    qCritical() << "Failed to create recipe:" << query.lastError().text();
    return -1;
}

bool RecipeRepository::updateRecipe(int id, const QString& name, const QString& instructions) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE recipe SET name = ?, instructions = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(instructions);
    query.addBindValue(id);

    if (query.exec()) {
        return true;
    }
    qCritical() << "Failed to update recipe:" << query.lastError().text();
    return false;
}

bool RecipeRepository::deleteRecipe(int id) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM recipe WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

std::vector<RecipeItem> RecipeRepository::getAllRecipes() {
    std::vector<RecipeItem> recipes;
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return recipes;

    QSqlQuery query(db);
    // TODO: Join with ingredient amounts * food nutrient values to get calories?
    // For now, simple list.
    if (query.exec("SELECT id, uuid, name, instructions, created FROM recipe ORDER BY name ASC")) {
        while (query.next()) {
            RecipeItem item;
            item.id = query.value(0).toInt();
            item.uuid = query.value(1).toString();
            item.name = query.value(2).toString();
            item.instructions = query.value(3).toString();
            item.created = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
            recipes.push_back(item);
        }
    } else {
        qCritical() << "Failed to fetch recipes:" << query.lastError().text();
    }
    return recipes;
}

RecipeItem RecipeRepository::getRecipe(int id) {
    RecipeItem item;
    item.id = -1;
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return item;

    QSqlQuery query(db);
    query.prepare("SELECT id, uuid, name, instructions, created FROM recipe WHERE id = ?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        item.id = query.value(0).toInt();
        item.uuid = query.value(1).toString();
        item.name = query.value(2).toString();
        item.instructions = query.value(3).toString();
        item.created = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
    }
    return item;
}

bool RecipeRepository::addIngredient(int recipeId, int foodId, double amount) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO recipe_ingredient (recipe_id, food_id, amount) VALUES (?, ?, ?)");
    query.addBindValue(recipeId);
    query.addBindValue(foodId);
    query.addBindValue(amount);

    if (!query.exec()) {
        qCritical() << "Failed to add ingredient:" << query.lastError().text();
        return false;
    }
    return true;
}

bool RecipeRepository::removeIngredient(int recipeId, int foodId) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM recipe_ingredient WHERE recipe_id = ? AND food_id = ?");
    query.addBindValue(recipeId);
    query.addBindValue(foodId);
    return query.exec();
}

bool RecipeRepository::updateIngredient(int recipeId, int foodId, double amount) {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE recipe_ingredient SET amount = ? WHERE recipe_id = ? AND food_id = ?");
    query.addBindValue(amount);
    query.addBindValue(recipeId);
    query.addBindValue(foodId);
    return query.exec();
}

std::vector<RecipeIngredient> RecipeRepository::getIngredients(int recipeId) {
    std::vector<RecipeIngredient> ingredients;
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return ingredients;

    // We need to join with USDA db 'food_des' to get names?
    // USDA db is attached as what? 'main' is User db usually?
    // Wait, DatabaseManager opens main USDA db as 'db' (default connection) and User DB as
    // 'user_db' (named connection). They are SEPARATE connections. Cross-database joins require
    // attaching. DatabaseManager doesn't seem to attach them by default. workaround: Get IDs then
    // query USDA db for names.

    QSqlQuery query(db);
    query.prepare("SELECT food_id, amount FROM recipe_ingredient WHERE recipe_id = ?");
    query.addBindValue(recipeId);

    if (query.exec()) {
        while (query.next()) {
            RecipeIngredient ing;
            ing.foodId = query.value(0).toInt();
            ing.amount = query.value(1).toDouble();

            // Fetch name from main DB
            // This is inefficient (N+1 queries), but simple for now without ATTACH logic.
            // Or we could pass a list of IDs to FoodRepository.

            QSqlDatabase usdaDb = DatabaseManager::instance().database();
            if (usdaDb.isOpen()) {
                QSqlQuery nameQuery(usdaDb);
                nameQuery.prepare("SELECT long_desc FROM food_des WHERE ndb_no = ?");
                nameQuery.addBindValue(ing.foodId);
                if (nameQuery.exec() && nameQuery.next()) {
                    ing.foodName = nameQuery.value(0).toString();
                } else {
                    ing.foodName = "Unknown Food (" + QString::number(ing.foodId) + ")";
                }
            }
            ingredients.push_back(ing);
        }
    }
    return ingredients;
}

#include <QDir>
#include <QFile>

void RecipeRepository::loadCsvRecipes(const QString& directory) {
    QDir dir(directory);
    if (!dir.exists()) return;

    // Pre-load all recipes into a map for O(1) lookup
    std::vector<RecipeItem> existingRecipes = getAllRecipes();
    std::map<QString, int> recipeMap;
    for (const auto& r : existingRecipes) {
        recipeMap[r.name] = r.id;
    }

    QStringList filters;
    filters << "*.csv";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    for (const auto& fileInfo : fileList) {
        processCsvFile(fileInfo.absoluteFilePath(), recipeMap);
    }
}

void RecipeRepository::processCsvFile(const QString& filePath, std::map<QString, int>& recipeMap) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    while (!file.atEnd()) {
        QString line = file.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        QStringList parts = line.split(',');
        if (parts.size() < 4) continue;

        QString recipeName = parts[0].trimmed();
        QString instructions = parts[1].trimmed();
        int foodId = parts[2].toInt();
        double amount = parts[3].toDouble();

        if (foodId <= 0 || amount <= 0) continue;

        int recipeId = getOrCreateRecipe(recipeName, instructions, recipeMap);
        if (recipeId != -1) {
            addIngredient(recipeId, foodId, amount);
        }
    }
    file.close();
}

int RecipeRepository::getOrCreateRecipe(const QString& name, const QString& instructions,
                                        std::map<QString, int>& recipeMap) {
    if (recipeMap.count(name) != 0U) {
        return recipeMap[name];
    }

    int newId = createRecipe(name, instructions);
    if (newId != -1) {
        recipeMap[name] = newId;
    }
    return newId;
}
