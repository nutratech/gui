#include "widgets/searchwidget.h"

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

#include "widgets/weightinputdialog.h"

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    // Search bar
    auto* searchLayout = new QHBoxLayout();
    searchInput = new QLineEdit(this);
    searchInput->setPlaceholderText("Search for food...");

    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(600);  // 600ms debounce

    connect(searchInput, &QLineEdit::textChanged, this, [=]() { searchTimer->start(); });
    connect(searchTimer, &QTimer::timeout, this, &SearchWidget::performSearch);
    connect(searchInput, &QLineEdit::returnPressed, this, &SearchWidget::performSearch);

    searchButton = new QPushButton("Search", this);
    connect(searchButton, &QPushButton::clicked, this, &SearchWidget::performSearch);

    searchLayout->addWidget(searchInput);
    searchLayout->addWidget(searchButton);
    layout->addLayout(searchLayout);

    // Results table
    resultsTable = new QTableWidget(this);
    resultsTable->setColumnCount(7);
    resultsTable->setHorizontalHeaderLabels(
        {"ID", "Description", "Group", "Nutr", "Amino", "Flav", "Score"});

    resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(resultsTable, &QTableWidget::cellDoubleClicked, this,
            &SearchWidget::onRowDoubleClicked);
    connect(resultsTable, &QTableWidget::customContextMenuRequested, this,
            &SearchWidget::onCustomContextMenu);

    layout->addWidget(resultsTable);
}

void SearchWidget::performSearch() {
    QString query = searchInput->text().trimmed();
    if (query.length() < 2) return;

    resultsTable->setRowCount(0);

    std::vector<FoodItem> results = repository.searchFoods(query);

    resultsTable->setRowCount(static_cast<int>(results.size()));
    for (int i = 0; i < static_cast<int>(results.size()); ++i) {
        const auto& item = results[i];
        resultsTable->setItem(i, 0, new QTableWidgetItem(QString::number(item.id)));
        static const std::map<int, QString> groupAbbreviations = {
            {1100, "Vegetables"},      // Vegetables and Vegetable Products
            {600, "Soups/Sauces"},     // Soups, Sauces, and Gravies
            {1700, "Lamb/Veal/Game"},  // Lamb, Veal, and Game Products
            {500, "Poultry"},          // Poultry Products
            {700, "Sausages/Meats"},   // Sausages and Luncheon Meats
            {800, "Cereals"},          // Breakfast Cereals
            {900, "Fruits"},           // Fruits and Fruit Juices
            {1200, "Nuts/Seeds"},      // Nut and Seed Products
            {1400, "Beverages"},       // Beverages
            {400, "Fats/Oils"},        // Fats and Oils
            {1900, "Sweets"},          // Sweets
            {1800, "Baked Prod."},     // Baked Products
            {2100, "Fast Food"},       // Fast Foods
            {2200, "Meals/Entrees"},   // Meals, Entrees, and Side Dishes
            {2500, "Snacks"},          // Snacks
            {3600, "Restaurant"},      // Restaurant Foods
            {100, "Dairy/Egg"},        // Dairy and Egg Products
            {1300, "Beef"},            // Beef Products
            {1000, "Pork"},            // Pork Products
            {2000, "Grains/Pasta"},    // Cereal Grains and Pasta
            {1600, "Legumes"},         // Legumes and Legume Products
            {1500, "Fish/Shellfish"},  // Finfish and Shellfish Products
            {300, "Baby Food"},        // Baby Foods
            {200, "Spices"},           // Spices and Herbs
            {3500, "Native Foods"}     // American Indian/Alaska Native Foods
        };

        QString group = item.foodGroupName;
        auto it = groupAbbreviations.find(item.foodGroupId);
        if (it != groupAbbreviations.end()) {
            group = it->second;
        } else if (group.length() > 20) {
            group = group.left(17) + "...";
        }
        resultsTable->setItem(i, 2, new QTableWidgetItem(group));
        resultsTable->setItem(i, 3, new QTableWidgetItem(QString::number(item.nutrientCount)));
        resultsTable->setItem(i, 4, new QTableWidgetItem(QString::number(item.aminoCount)));
        resultsTable->setItem(i, 5, new QTableWidgetItem(QString::number(item.flavCount)));
        resultsTable->setItem(i, 6, new QTableWidgetItem(QString::number(item.score)));
    }
}

void SearchWidget::onRowDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem* idItem = resultsTable->item(row, 0);
    QTableWidgetItem* descItem = resultsTable->item(row, 1);

    if (idItem != nullptr && descItem != nullptr) {
        emit foodSelected(idItem->text().toInt(), descItem->text());
    }
}

void SearchWidget::onCustomContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = resultsTable->itemAt(pos);
    if (item == nullptr) return;

    int row = item->row();
    QTableWidgetItem* idItem = resultsTable->item(row, 0);
    QTableWidgetItem* descItem = resultsTable->item(row, 1);

    if (idItem == nullptr || descItem == nullptr) return;

    int foodId = idItem->text().toInt();
    QString foodName = descItem->text();

    QMenu menu(this);
    QAction* analyzeAction = menu.addAction("Analyze");
    QAction* addToMealAction = menu.addAction("Add to Meal");

    QAction* selectedAction = menu.exec(resultsTable->viewport()->mapToGlobal(pos));

    if (selectedAction == analyzeAction) {
        emit foodSelected(foodId, foodName);
    } else if (selectedAction == addToMealAction) {
        std::vector<ServingWeight> servings = repository.getFoodServings(foodId);
        WeightInputDialog dlg(foodName, servings, this);
        if (dlg.exec() == QDialog::Accepted) {
            emit addToMealRequested(foodId, foodName, dlg.getGrams());
        }
    }
}
