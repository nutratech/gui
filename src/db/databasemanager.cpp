#include "db/databasemanager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QVariant>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
    m_userDb = QSqlDatabase::addDatabase("QSQLITE", "user_db");
    initUserDatabase();
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (m_userDb.isOpen()) {
        m_userDb.close();
    }
}

bool DatabaseManager::isValidNutraDatabase(const QSqlDatabase& db) {
    if (!db.isOpen()) return false;
    QSqlQuery query(db);
    // Check for a critical table, e.g., food_des
    return query.exec("SELECT 1 FROM food_des LIMIT 1");
}

bool DatabaseManager::connect(const QString& path) {
    if (m_db.isOpen()) {
        if (m_db.databaseName() == path) {
            return true;
        }
        m_db.close();
    }

    if (!QFileInfo::exists(path)) {
        qCritical() << "Database file not found:" << path;
        return false;
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);
    m_db.setConnectOptions("QSQLITE_OPEN_READONLY");

    if (!m_db.open()) {
        qCritical() << "Error opening database:" << m_db.lastError().text();
        return false;
    }

    if (!isValidNutraDatabase(m_db)) {
        qCritical() << "Invalid database: missing essential tables.";
        m_db.close();
        return false;
    }

    return true;
}

bool DatabaseManager::isOpen() const {
    return m_db.isOpen();
}

QSqlDatabase DatabaseManager::database() const {
    return m_db;
}

QSqlDatabase DatabaseManager::userDatabase() const {
    return m_userDb;
}

void DatabaseManager::initUserDatabase() {
    QString dirPath = QDir::homePath() + "/.nutra";
    QDir().mkpath(dirPath);
    QString path = dirPath + "/nt.sqlite3";
    m_userDb.setDatabaseName(path);

    if (!m_userDb.open()) {
        qCritical() << "Failed to open user database:" << m_userDb.lastError().text();
        return;
    }

    QSqlQuery query(m_userDb);

    // Helper to execute schema creation
    auto createTable = [&](const QString& sql) {
        if (!query.exec(sql)) {
            qCritical() << "Failed to create table:" << query.lastError().text() << "\nSQL:" << sql;
        }
    };

    createTable(
        "CREATE TABLE IF NOT EXISTS version ("
        "id integer PRIMARY KEY AUTOINCREMENT, "
        "version text NOT NULL UNIQUE, "
        "created date NOT NULL, "
        "notes text)");

    createTable(
        "CREATE TABLE IF NOT EXISTS bmr_eq ("
        "id integer PRIMARY KEY, "
        "name text NOT NULL UNIQUE)");

    createTable(
        "CREATE TABLE IF NOT EXISTS bf_eq ("
        "id integer PRIMARY KEY, "
        "name text NOT NULL UNIQUE)");

    createTable(
        "CREATE TABLE IF NOT EXISTS profile ("
        "id integer PRIMARY KEY AUTOINCREMENT, "
        "uuid int NOT NULL DEFAULT (RANDOM()), "
        "name text NOT NULL UNIQUE, "
        "gender text, "
        "dob date, "
        "act_lvl int DEFAULT 2, "
        "goal_wt real, "
        "goal_bf real DEFAULT 18, "
        "bmr_eq_id int DEFAULT 1, "
        "bf_eq_id int DEFAULT 1, "
        "created int DEFAULT (strftime ('%s', 'now')), "
        "FOREIGN KEY (bmr_eq_id) REFERENCES bmr_eq (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE, "
        "FOREIGN KEY (bf_eq_id) REFERENCES bf_eq (id) ON UPDATE CASCADE "
        "ON DELETE CASCADE)");

    createTable(
        "CREATE TABLE IF NOT EXISTS rda ("
        "profile_id int NOT NULL, "
        "nutr_id int NOT NULL, "
        "rda real NOT NULL, "
        "PRIMARY KEY (profile_id, nutr_id), "
        "FOREIGN KEY (profile_id) REFERENCES profile (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE)");

    createTable(
        "CREATE TABLE IF NOT EXISTS custom_food ("
        "id integer PRIMARY KEY AUTOINCREMENT, "
        "tagname text NOT NULL UNIQUE, "
        "name text NOT NULL UNIQUE, "
        "created int DEFAULT (strftime ('%s', 'now')))");

    createTable(
        "CREATE TABLE IF NOT EXISTS cf_dat ("
        "cf_id int NOT NULL, "
        "nutr_id int NOT NULL, "
        "nutr_val real NOT NULL, "
        "notes text, "
        "created int DEFAULT (strftime ('%s', 'now')), "
        "PRIMARY KEY (cf_id, nutr_id), "
        "FOREIGN KEY (cf_id) REFERENCES custom_food (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE)");

    createTable(
        "CREATE TABLE IF NOT EXISTS meal_name ("
        "id integer PRIMARY KEY AUTOINCREMENT, "
        "name text NOT NULL)");

    createTable(
        "CREATE TABLE IF NOT EXISTS log_food ("
        "id integer PRIMARY KEY AUTOINCREMENT, "
        "profile_id int NOT NULL, "
        "date int DEFAULT (strftime ('%s', 'now')), "
        "meal_id int NOT NULL, "
        "food_id int NOT NULL, "
        "msre_id int NOT NULL, "
        "amt real NOT NULL, "
        "created int DEFAULT (strftime ('%s', 'now')), "
        "FOREIGN KEY (profile_id) REFERENCES profile (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE, "
        "FOREIGN KEY (meal_id) REFERENCES meal_name (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE)");

    createTable(
        "CREATE TABLE IF NOT EXISTS log_cf ("
        "id integer PRIMARY KEY AUTOINCREMENT, "
        "profile_id int NOT NULL, "
        "date int DEFAULT (strftime ('%s', 'now')), "
        "meal_id int NOT NULL, "
        "food_id int NOT NULL, "
        "custom_food_id int, "
        "msre_id int NOT NULL, "
        "amt real NOT NULL, "
        "created int DEFAULT (strftime ('%s', 'now')), "
        "FOREIGN KEY (profile_id) REFERENCES profile (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE, "
        "FOREIGN KEY (meal_id) REFERENCES meal_name (id) ON UPDATE "
        "CASCADE ON DELETE CASCADE, "
        "FOREIGN KEY (custom_food_id) REFERENCES custom_food (id) ON "
        "UPDATE CASCADE ON DELETE CASCADE)");

    // Default Data Seeding

    // Ensure default profile exists
    query.exec("INSERT OR IGNORE INTO profile (id, name) VALUES (1, 'default')");

    // Seed standard meal names if table is empty
    query.exec("SELECT count(*) FROM meal_name");
    if (query.next() && query.value(0).toInt() == 0) {
        QStringList meals = {"Breakfast", "Lunch", "Dinner", "Snack", "Brunch"};
        for (const auto& meal : meals) {
            query.prepare("INSERT INTO meal_name (name) VALUES (?)");
            query.addBindValue(meal);
            query.exec();
        }
    }
}
