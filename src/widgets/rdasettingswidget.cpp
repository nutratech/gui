#include "widgets/rdasettingswidget.h"
#include "db/databasemanager.h"
#include <QHeaderView>
#include <QLabel>
#include <QSqlQuery>
#include <QVBoxLayout>

RDASettingsWidget::RDASettingsWidget(FoodRepository &repository,
                                     QWidget *parent)
    : QDialog(parent), m_repository(repository) {
  setWindowTitle("RDA Settings");
  resize(600, 400);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(new QLabel("Customize your Recommended Daily Allowances "
                               "(RDA). Changes are saved automatically."));

  m_table = new QTableWidget(this);
  m_table->setColumnCount(4);
  m_table->setHorizontalHeaderLabels({"ID", "Nutrient", "RDA", "Unit"});
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

  loadData();

  connect(m_table, &QTableWidget::cellChanged, this,
          &RDASettingsWidget::onCellChanged);

  layout->addWidget(m_table);
}

void RDASettingsWidget::loadData() {
  m_loading = true;
  m_table->setRowCount(0);

  QSqlDatabase db = DatabaseManager::instance().database();
  if (!db.isOpen())
    return;

  // Get metadata from USDA
  QSqlQuery query(
      "SELECT id, nutr_desc, unit FROM nutrients_overview ORDER BY nutr_desc",
      db);
  auto currentRdas = m_repository.getNutrientRdas();

  int row = 0;
  while (query.next()) {
    int id = query.value(0).toInt();
    QString name = query.value(1).toString();
    QString unit = query.value(2).toString();
    double rda = currentRdas.count(id) != 0U ? currentRdas[id] : 0.0;

    m_table->insertRow(row);

    auto *idItem = new QTableWidgetItem(QString::number(id));
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, 0, idItem);

    auto *nameItem = new QTableWidgetItem(name);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, 1, nameItem);

    auto *rdaItem = new QTableWidgetItem(QString::number(rda));
    m_table->setItem(row, 2, rdaItem);

    auto *unitItem = new QTableWidgetItem(unit);
    unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, 3, unitItem);

    row++;
  }

  m_loading = false;
}

void RDASettingsWidget::onCellChanged(int row, int column) {
  if (m_loading || column != 2)
    return;

  int id = m_table->item(row, 0)->text().toInt();
  double value = m_table->item(row, 2)->text().toDouble();

  m_repository.updateRda(id, value);
}
