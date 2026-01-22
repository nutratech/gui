#ifndef RECIPEWIDGET_H
#define RECIPEWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

#include "db/foodrepository.h"
#include "db/reciperepository.h"

class RecipeWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecipeWidget(QWidget* parent = nullptr);

signals:
    void recipeSelected(int recipeId);

private slots:
    void onNewRecipe();
    void onSaveRecipe();
    void onDeleteRecipe();
    void onRecipeListSelectionChanged();
    void onAddIngredient();
    void onRemoveIngredient();

private:
    void setupUi();
    void loadRecipes();
    void loadRecipeDetails(int recipeId);
    void clearDetails();

    RecipeRepository repository;
    FoodRepository foodRepo;  // For ingredient search/lookup

    QListWidget* recipeList;
    QLineEdit* nameEdit;
    QTableWidget* ingredientsTable;
    QTextEdit* instructionsEdit;

    QPushButton* saveButton;
    QPushButton* deleteButton;
    QPushButton* newButton;
    QPushButton* addIngredientButton;
    QPushButton* removeIngredientButton;

    int currentRecipeId = -1;
};

#endif  // RECIPEWIDGET_H
