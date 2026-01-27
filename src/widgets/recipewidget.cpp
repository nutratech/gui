#include "widgets/recipewidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

// Simple dialog to pick a food (MVP specific for ingredients)
// Ideally we reuse SearchWidget, but wrapping it might be cleaner later.
// For now, let's use a simple input dialog for ID, or we can make a tiny search dialog.
#include <QDialog>

#include "widgets/searchwidget.h"

class IngredientSearchDialog : public QDialog {
public:
    IngredientSearchDialog(QWidget* parent) : QDialog(parent) {
        setWindowTitle("Add Ingredient");
        resize(600, 400);
        auto* layout = new QVBoxLayout(this);
        searchWidget = new SearchWidget(this);
        layout->addWidget(searchWidget);

        connect(searchWidget, &SearchWidget::foodSelected, this,
                [this](int id, const QString& name) {
                    selectedFoodId = id;
                    selectedFoodName = name;
                    accept();
                });
    }

    int selectedFoodId = -1;
    QString selectedFoodName;
    SearchWidget* searchWidget;
};

RecipeWidget::RecipeWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    loadRecipes();
}

void RecipeWidget::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(splitter);

    // Left Pane: Recipe List
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    leftLayout->addWidget(new QLabel("Recipes", this));
    recipeList = new QListWidget(this);
    leftLayout->addWidget(recipeList);

    newButton = new QPushButton("New Recipe", this);
    leftLayout->addWidget(newButton);

    leftWidget->setLayout(leftLayout);
    splitter->addWidget(leftWidget);

    // Right Pane: Details
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Name
    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:", this));
    nameEdit = new QLineEdit(this);
    nameLayout->addWidget(nameEdit);
    rightLayout->addLayout(nameLayout);

    // Ingredients
    rightLayout->addWidget(new QLabel("Ingredients:", this));
    ingredientsTable = new QTableWidget(this);
    ingredientsTable->setColumnCount(3);
    ingredientsTable->setHorizontalHeaderLabels({"ID", "Food", "Amount (g)"});
    ingredientsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ingredientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rightLayout->addWidget(ingredientsTable);

    auto* ingButtonsLayout = new QHBoxLayout();
    addIngredientButton = new QPushButton("Add Ingredient", this);
    removeIngredientButton = new QPushButton("Remove Selected", this);
    ingButtonsLayout->addWidget(addIngredientButton);
    ingButtonsLayout->addWidget(removeIngredientButton);
    ingButtonsLayout->addStretch();
    rightLayout->addLayout(ingButtonsLayout);

    // Instructions
    rightLayout->addWidget(new QLabel("Instructions:", this));
    instructionsEdit = new QTextEdit(this);
    rightLayout->addWidget(instructionsEdit);

    // Action Buttons
    auto* actionLayout = new QHBoxLayout();
    saveButton = new QPushButton("Save", this);
    deleteButton = new QPushButton("Delete", this);
    // deleteButton->setStyleSheet("background-color: #e74c3c; color: white;"); // User prefers
    // styled?

    actionLayout->addStretch();
    actionLayout->addWidget(deleteButton);
    actionLayout->addWidget(saveButton);
    rightLayout->addLayout(actionLayout);

    rightWidget->setLayout(rightLayout);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(1, 2);  // Give details more space

    // Connections
    connect(newButton, &QPushButton::clicked, this, &RecipeWidget::onNewRecipe);
    connect(recipeList, &QListWidget::itemSelectionChanged, this,
            &RecipeWidget::onRecipeListSelectionChanged);
    connect(saveButton, &QPushButton::clicked, this, &RecipeWidget::onSaveRecipe);
    connect(deleteButton, &QPushButton::clicked, this, &RecipeWidget::onDeleteRecipe);
    connect(addIngredientButton, &QPushButton::clicked, this, &RecipeWidget::onAddIngredient);
    connect(removeIngredientButton, &QPushButton::clicked, this, &RecipeWidget::onRemoveIngredient);

    clearDetails();
}

void RecipeWidget::loadRecipes() {
    recipeList->clear();
    auto recipes = repository.getAllRecipes();
    for (const auto& r : recipes) {
        auto* item = new QListWidgetItem(r.name, recipeList);
        item->setData(Qt::UserRole, r.id);
    }
}

void RecipeWidget::onRecipeListSelectionChanged() {
    auto items = recipeList->selectedItems();
    if (items.isEmpty()) {
        clearDetails();
        return;
    }
    int id = items.first()->data(Qt::UserRole).toInt();
    loadRecipeDetails(id);
}

void RecipeWidget::loadRecipeDetails(int recipeId) {
    currentRecipeId = recipeId;
    RecipeItem item = repository.getRecipe(recipeId);
    if (item.id == -1) return;  // Error

    nameEdit->setText(item.name);
    instructionsEdit->setText(item.instructions);

    // Load ingredients
    ingredientsTable->setRowCount(0);
    auto ingredients = repository.getIngredients(recipeId);
    ingredientsTable->setRowCount(static_cast<int>(ingredients.size()));
    for (int i = 0; i < ingredients.size(); ++i) {
        const auto& ing = ingredients[i];
        ingredientsTable->setItem(i, 0, new QTableWidgetItem(QString::number(ing.foodId)));
        ingredientsTable->setItem(i, 1, new QTableWidgetItem(ing.foodName));
        ingredientsTable->setItem(i, 2, new QTableWidgetItem(QString::number(ing.amount)));
    }

    saveButton->setEnabled(true);
    deleteButton->setEnabled(true);
}

void RecipeWidget::clearDetails() {
    currentRecipeId = -1;
    nameEdit->clear();
    instructionsEdit->clear();
    ingredientsTable->setRowCount(0);
    saveButton->setEnabled(false);
    deleteButton->setEnabled(false);
}

void RecipeWidget::onNewRecipe() {
    recipeList->clearSelection();
    clearDetails();
    nameEdit->setFocus();
    saveButton->setEnabled(true);  // Allow saving a new one
}

void RecipeWidget::onSaveRecipe() {
    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Recipe name cannot be empty.");
        return;
    }
    QString instructions = instructionsEdit->toPlainText();

    if (currentRecipeId == -1) {
        // Create
        int newId = repository.createRecipe(name, instructions);
        if (newId != -1) {
            currentRecipeId = newId;
            // Add ingredients from table if any (though usually table is empty on new)
            // But if user added ingredients before saving, we should handle that.
            // Currently, adding ingredient requires a recipe ID?
            // If strict: enforce save before adding ingredients.
            // Or: keep ingredients in memory until save.
            // For MVP: Simplest is: Create Recipe -> Then Add Ingredients.
            // So if new, save creates empty recipe, then reloads it, allowing adds.
            loadRecipes();
            // Select the new item
            for (int i = 0; i < recipeList->count(); ++i) {
                if (recipeList->item(i)->data(Qt::UserRole).toInt() == newId) {
                    recipeList->setCurrentRow(i);
                    break;
                }
            }
        }
    } else {
        // Update
        repository.updateRecipe(currentRecipeId, name, instructions);

        // Update ingredients?
        // Ingredients are updated immediately in onAdd/RemoveIngredient for now?
        // Or we should do batch save.
        // Current implementation of onAddIngredient writes to DB immediately.
        // So here just update name/instructions.

        // Refresh list name if changed
        auto items = recipeList->findItems(name, Qt::MatchExactly);  // This might find others?
        // Just reload list to be safe or update current item
        if (!recipeList->selectedItems().isEmpty()) {
            recipeList->selectedItems().first()->setText(name);
        }
    }
}

void RecipeWidget::onDeleteRecipe() {
    if (currentRecipeId == -1) return;

    auto reply = QMessageBox::question(
        this, "Confirm Delete",
        "Are you sure you want to delete this recipe?\n\n"
        "Note: The recipe will be marked as deleted and can be recovered if needed.",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (repository.deleteRecipe(currentRecipeId)) {
            loadRecipes();
            clearDetails();
            QMessageBox::information(
                this, "Recipe Deleted",
                "Recipe marked as deleted. It can be recovered from the database if needed.");
        }
    }
}

void RecipeWidget::onAddIngredient() {
    if (currentRecipeId == -1) {
        QMessageBox::information(this, "Save Required",
                                 "Please save the recipe before adding ingredients.");
        return;
    }

    IngredientSearchDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        int foodId = dlg.selectedFoodId;
        QString foodName = dlg.selectedFoodName;

        // Ask for amount
        bool ok;
        double amount = QInputDialog::getDouble(
            this, "Ingredient Amount", QString("Enter amount (grams) for %1:").arg(foodName), 100.0,
            0.1, 10000.0, 1, &ok);

        if (ok) {
            if (repository.addIngredient(currentRecipeId, foodId, amount)) {
                loadRecipeDetails(currentRecipeId);  // Refresh table
            }
        }
    }
}

void RecipeWidget::onRemoveIngredient() {
    if (currentRecipeId == -1) return;

    int row = ingredientsTable->currentRow();
    if (row < 0) return;

    auto* item = ingredientsTable->item(row, 0);
    if (item == nullptr) return;
    int foodId = item->text().toInt();

    if (repository.removeIngredient(currentRecipeId, foodId)) {
        ingredientsTable->removeRow(row);
    }
}
