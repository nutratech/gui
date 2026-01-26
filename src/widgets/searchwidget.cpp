#include "widgets/searchwidget.h"

#include <QAbstractItemView>
#include <QAction>
#include <QDateTime>
#include <QElapsedTimer>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "widgets/weightinputdialog.h"

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    // Search bar
    auto* searchLayout = new QHBoxLayout();
    searchInput = new QLineEdit(this);
    searchInput->setPlaceholderText("Search for food (or type to see history)...");

    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);

    reloadSettings();

    // History Completer
    historyModel = new QStringListModel(this);
    historyCompleter = new QCompleter(historyModel, this);
    historyCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    historyCompleter->setCompletionMode(QCompleter::PopupCompletion);
    searchInput->setCompleter(historyCompleter);

    QAbstractItemView* popup = historyCompleter->popup();
    popup->setContextMenuPolicy(Qt::CustomContextMenu);
    popup->installEventFilter(this);

    connect(popup, &QAbstractItemView::customContextMenuRequested, this,
            &SearchWidget::onHistoryContextMenu);

    connect(historyCompleter, QOverload<const QString&>::of(&QCompleter::activated), this,
            &SearchWidget::onCompleterActivated);

    connect(searchInput, &QLineEdit::textChanged, this, [=]() { searchTimer->start(); });
    connect(searchTimer, &QTimer::timeout, this, &SearchWidget::performSearch);
    connect(searchInput, &QLineEdit::returnPressed, this, &SearchWidget::performSearch);

    searchLayout->addWidget(searchInput);

    auto* searchButton = new QPushButton("Search", this);
    connect(searchButton, &QPushButton::clicked, this, &SearchWidget::performSearch);
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

    loadHistory();
}

void SearchWidget::performSearch() {
    QString query = searchInput->text().trimmed();
    if (query.length() < 2) return;

    // Save query to history
    addToHistory(0, query);

    // Organization and application name - saves to ~/.config/nutra/nutra.conf
    QSettings settings("nutra", "nutra");

    // Save persistence
    settings.setValue("lastSearchQuery", query);

    QElapsedTimer timer;
    timer.start();

    resultsTable->setRowCount(0);

    std::vector<FoodItem> results = repository.searchFoods(query);
    int elapsed = static_cast<int>(timer.elapsed());

    resultsTable->setRowCount(static_cast<int>(results.size()));
    for (int i = 0; i < static_cast<int>(results.size()); ++i) {
        const auto& item = results[i];
        resultsTable->setItem(i, 0, new QTableWidgetItem(QString::number(item.id)));
        resultsTable->setItem(i, 1,
                              new QTableWidgetItem(item.description));  // Fixed: Description Column

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

    emit searchStatus(
        QString("Search: matched %1 foods in %2 ms").arg(results.size()).arg(elapsed));
}

void SearchWidget::onRowDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem* idItem = resultsTable->item(row, 0);
    QTableWidgetItem* descItem = resultsTable->item(row, 1);

    if (idItem != nullptr && descItem != nullptr) {
        int id = idItem->text().toInt();
        QString name = descItem->text();
        addToHistory(id, name);
        emit foodSelected(id, name);
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

    if (selectedAction != nullptr) {
        addToHistory(foodId, foodName);
    }

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

void SearchWidget::addToHistory(int foodId, const QString& foodName) {
    // Remove if exists to move to top
    for (int i = 0; i < recentHistory.size(); ++i) {
        bool sameId = (foodId != 0) && (recentHistory[i].id == foodId);
        bool sameName = (recentHistory[i].name.compare(foodName, Qt::CaseInsensitive) == 0);

        if (sameId || sameName) {
            recentHistory.removeAt(i);
            break;
        }
    }

    HistoryItem item{foodId, foodName, QDateTime::currentDateTime()};
    recentHistory.prepend(item);

    // Limit to 50
    while (recentHistory.size() > 50) {
        recentHistory.removeLast();
    }

    // Save to settings
    QSettings settings("NutraTech", "Nutra");
    QList<QVariant> list;
    for (const auto& h : recentHistory) {
        QVariantMap m;
        m["id"] = h.id;
        m["name"] = h.name;
        m["timestamp"] = h.timestamp;
        list.append(m);
    }
    settings.setValue("recentFoods", list);

    updateCompleterModel();
}

void SearchWidget::loadHistory() {
    QSettings settings("nutra", "nutra");
    QList<QVariant> list = settings.value("recentFoods").toList();
    recentHistory.clear();
    for (const auto& v : list) {
        QVariantMap m = v.toMap();
        HistoryItem item;
        item.id = m["id"].toInt();
        item.name = m["name"].toString();
        item.timestamp = m["timestamp"].toDateTime();
        recentHistory.append(item);
    }
    updateCompleterModel();
}

void SearchWidget::updateCompleterModel() {
    QStringList suggestions;
    for (const auto& item : recentHistory) {
        suggestions << item.name;
    }
    historyModel->setStringList(suggestions);
}

void SearchWidget::onCompleterActivated(const QString& text) {
    searchInput->blockSignals(true);
    searchInput->setText(text);
    searchInput->blockSignals(false);
    performSearch();
}

void SearchWidget::reloadSettings() {
    QSettings settings("nutra", "nutra");
    int debounce = settings.value("searchDebounce", 600).toInt();
    debounce = std::max(debounce, 250);
    searchTimer->setInterval(debounce);

    // Restore last search if empty
    if (searchInput->text().isEmpty() && settings.contains("lastSearchQuery")) {
        QString lastQuery = settings.value("lastSearchQuery").toString();
        searchInput->setText(lastQuery);
        // Defer search slightly to allow UI unchecked init
        QTimer::singleShot(100, this, &SearchWidget::performSearch);
    }
}

bool SearchWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == historyCompleter->popup() && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            QModelIndex index = historyCompleter->popup()->currentIndex();
            if (index.isValid()) {
                removeFromHistory(index.row());
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SearchWidget::onHistoryContextMenu(const QPoint& pos) {
    QAbstractItemView* popup = historyCompleter->popup();
    QModelIndex index = popup->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    QAction* deleteAction = menu.addAction("Remove from history");
    QAction* selectedAction = menu.exec(popup->viewport()->mapToGlobal(pos));

    if (selectedAction == deleteAction) {
        removeFromHistory(index.row());
    }
}

void SearchWidget::removeFromHistory(int index) {
    if (index < 0 || index >= recentHistory.size()) return;

    recentHistory.removeAt(index);

    // Save to settings
    QSettings settings("NutraTech", "Nutra");
    QList<QVariant> list;
    for (const auto& h : recentHistory) {
        QVariantMap m;
        m["id"] = h.id;
        m["name"] = h.name;
        m["timestamp"] = h.timestamp;
        list.append(m);
    }
    settings.setValue("recentFoods", list);

    updateCompleterModel();
}
