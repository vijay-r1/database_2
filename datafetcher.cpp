// DataFetcher.cpp
#include "datafetcher.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDateTime>
#include <QFile>          //  Required for file handling
#include <QTextStream>    // Required for writing to file
#include <QIODevice>

DataFetcher::DataFetcher(QObject *parent) : QObject(parent), totalRecordsFetched(0) {}

void DataFetcher::startExport(const QString &type, const QString &length, const QDate &fromDate, const QDate &toDate)
{
    qDebug() << "[DataFetcher] Export clicked with type:" << type << ", length:" << length
             << ", From:" << fromDate << ", To:" << toDate;
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        emit logMessage("SQLITE driver not available");
        return;
    }

    fetchData(fromDate, toDate);

    if (type == "CSV")
        exportToCSV();
    else if (type == "PDF")
        exportToPDF();
    else if (type == "PUC")
        exportToPUC();

    emit exportComplete();
}

void DataFetcher::fetchData(const QDate &fromDate, const QDate &toDate) {
    if (fromDate > toDate) {
        emit logMessage("Selected dates are not valid");
        return;
    }

    totalRecordsFetched = 0;
    int fromYear = fromDate.year();
    int toYear = toDate.year();

    if (fromYear == toYear) {
        QString dbPath = "/home/thermo/database_1/TsxUlt42_" + QString::number(fromYear) + ".sqlite";
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "conn_" + QString::number(fromYear));
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            logMessage("Failed to open database: " + dbPath);
            return;
        }
        int dayDiff = fromDate.daysTo(toDate);

        if (dayDiff <= 45) {
            fetchFromDatabase(db, fromDate, toDate);
        } else {
            QDate tempFrom = fromDate;
            QDate tempTo = tempFrom.addDays(30);

            while (tempFrom <= toDate) {
                if (tempTo > toDate) tempTo = toDate;
                fetchFromDatabase(db, tempFrom, tempTo);
                tempFrom = tempTo.addDays(1);
                tempTo = tempFrom.addDays(30);
            }
        }

        db.close();
    } else {
        for (int year = fromYear; year <= toYear; ++year) {
            QString dbPath = "/home/thermo/database_1/TsxUlt42_" + QString::number(year) + ".sqlite";
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "conn_" + QString::number(year));
            db.setDatabaseName(dbPath);

            if (!db.open()) {
                logMessage("Failed to open database: " + dbPath);
                continue;
            }
            qDebug() << "DB Opened: " << db.databaseName();

            int daysDiff = fromDate.daysTo(toDate);
            if (daysDiff <= 45) {
                QDate tempFrom = fromDate;
                QDate tempTo = qMin(QDate(tempFrom.year(), 12, 31), toDate);

                fetchFromDatabase(db, tempFrom, tempTo);

                tempFrom = tempTo.addDays(1);

                if (tempFrom <= toDate) {
                    db.close();
                    QString nextDbPath = QString("/home/thermo/database_1/TsxUlt42_%1.sqlite").arg(tempFrom.year());
                    db.setDatabaseName(nextDbPath);
                    if (!db.open()) {
                        qDebug() << "Failed to open database:" << nextDbPath;
                        return;
                    }
                    qDebug() << "DB Opened:" << nextDbPath;

                    tempTo = toDate;
                    fetchFromDatabase(db, tempFrom, tempTo);
                }
                return;
            } else {
                QDate tempFrom = qMax(fromDate, QDate(year, 1, 1));
                QDate yearEndDate = qMin(QDate(tempFrom.year(), 12, 31), toDate);

                QDate tempTo = qMin(QDate(tempFrom.year(), 12, 31), tempFrom.addDays(30));
                qDebug() << "Temp From, Temp To: " << tempFrom << tempTo;

                while (tempFrom <= yearEndDate) {
                    if (tempTo > yearEndDate)
                        tempTo = yearEndDate;

                    fetchFromDatabase(db, tempFrom, tempTo);
                    tempFrom = tempTo.addDays(1);

                    if (tempFrom.addDays(30) > yearEndDate) {
                        tempTo = yearEndDate;
                    } else {
                        tempTo = qMin(QDate(tempFrom.year(), 12, 31), tempFrom.addDays(30));
                    }
                }
            }
            db.close();
        }
    }
}

void DataFetcher::fetchFromDatabase(QSqlDatabase& db, const QDate& from, const QDate& to) {
    QString chunkStart = from.toString("yyyy/MM/dd") + " 00:00:00";
    QString chunkEnd = to.toString("yyyy/MM/dd") + " 23:59:59";

    QSqlQuery query(db);
    query.prepare(R"(SELECT * FROM ChartDetails WHERE Date_Time BETWEEN :from AND :to)");
    query.bindValue(":from", chunkStart);
    query.bindValue(":to", chunkEnd);

    if (query.exec()) {
        int chunkCount = 0;
        while (query.next()) {
            totalRecordsFetched++;
            chunkCount++;

            QStringList row;
            for (int i = 0; i < query.record().count(); ++i) {
                row << query.value(i).toString();
            }
            fetchedRecords.append(row);
        }

        logMessage("Fetched " + QString::number(chunkCount) + " records from " + chunkStart + " to " + chunkEnd);
    } else {
        logMessage("Query failed for range: " + chunkStart + " to " + chunkEnd);
    }
}

void DataFetcher::exportToCSV() {
    qDebug() << "Exporting to CSV...";

    // Set destination path directly (USB path in this example)
    QString destinationPath = "/home/thermo/";

    if (fetchedRecords.isEmpty()) {
        emit logMessage("Export failed: No records to export.");
        return;
    }

    // Compose filename with timestamp
    QString filePath = destinationPath + "/export_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logMessage("Export failed: Could not open file " + filePath);
        return;
    }

    QTextStream out(&file);

    // Write header (optional)
    QStringList header;
    for (int i = 0; i < fetchedRecords.first().size(); ++i) {
        header << "Column" + QString::number(i + 1);
    }
    out << header.join(",") << "\n";

    // Write data
    for (const QStringList& row : qAsConst(fetchedRecords)) {
        out << row.join(",") << "\n";
    }

    file.close();
    emit logMessage("Export finished: " + filePath);
}


void DataFetcher::exportToPDF() {
    qDebug() << "Exporting to PDF...";

    QString destinationPath = "/home/thermo/";

    if (fetchedRecords.isEmpty()) {
        emit logMessage("Export failed: No records to export.");
        return;
    }

    QString filePath = destinationPath + "/export_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".pdf";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logMessage("Export failed: Could not open file " + filePath);
        return;
    }

    QTextStream out(&file);
    out << "Exported Data - PDF Format (Simulated)\n\n";

    QStringList header;
    for (int i = 0; i < fetchedRecords.first().size(); ++i) {
        header << "Column" + QString::number(i + 1);
    }
    out << header.join(" | ") << "\n";
    out << QString("-").repeated(80) << "\n";

    for (const QStringList& row : qAsConst(fetchedRecords)) {
        out << row.join(" | ") << "\n";
    }

    file.close();
    emit logMessage("Export finished (PDF simulation): " + filePath);
}

void DataFetcher::exportToPUC() {
    qDebug() << "Exporting to PUC...";

    QString destinationPath = "/home/thermo/";

    if (fetchedRecords.isEmpty()) {
        emit logMessage("Export failed: No records to export.");
        return;
    }

    QString filePath = destinationPath + "/export_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".puc";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logMessage("Export failed: Could not open file " + filePath);
        return;
    }

    QTextStream out(&file);
    out << "PUC Format Export (Simulated)\n\n";

    for (const QStringList& row : qAsConst(fetchedRecords)) {
        out << row.join(" | ") << "\n";
    }

    file.close();
    emit logMessage("Export finished (PUC simulation): " + filePath);
}




/*// datafetcher.cpp
#include "datafetcher.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtConcurrent>
#include <QDebug>

DataFetcher::DataFetcher(QObject *parent) : QObject(parent) {}

void DataFetcher::setDateRange(const QDate &from, const QDate &to) {
    fromDate = from;
    toDate = to;
}

void DataFetcher::startFetch() {
    QtConcurrent::run([=]() {
        if (fromDate > toDate) {
            emit logMessage("Selected dates are not valid");
            emit fetchCompleted();
            return;
        }

        openDatabases();

        if (fromDate.year() == toDate.year()) {
            fetchSingleYear(fromDate, toDate);
        } else {
            fetchMultipleYears(fromDate, toDate);
        }

        closeDatabases();
        emit logMessage("Fetch completed.");
        emit fetchCompleted();
    });
}

void DataFetcher::openDatabases() {
    for (int year = fromDate.year(); year <= toDate.year(); ++year) {
        if (!dbMap.contains(year)) {
            if (!checkDatabaseExists(year)) {
                emit logMessage(QString("Database for year %1 not found").arg(year));
                continue;
            }
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QString("db_%1").arg(year));
            db.setDatabaseName(QString("/home/thermo/database_1/TsxUlt42_%1").arg(year));
            if (!db.open()) {
                emit logMessage(QString("Failed to open database for year %1").arg(year));
                continue;
            }
            if (!checkTableExists(db, "ChartDetails")) {
                emit logMessage(QString("ChartDetails table missing in %1").arg(year));
                db.close();
                continue;
            }
            dbMap.insert(year, db);
        }
    }
}

void DataFetcher::closeDatabases() {
    for (auto db : dbMap) {
        db.close();
    }
    dbMap.clear();
}

bool DataFetcher::checkDatabaseExists(int year) {
    QFile file(QString("/home/thermo/database_1/TsxUlt42_%1").arg(year));
    return file.exists();
}

bool DataFetcher::checkTableExists(QSqlDatabase &db, const QString &tableName) {
    return db.tables().contains(tableName);
}

void DataFetcher::fetchSingleYear(const QDate &from, const QDate &to) {
    QSqlDatabase db = dbMap.value(from.year());
    int days = from.daysTo(to);

    if (days <= 45) {
        emit logMessage(QString("Fetching from %1 to %2").arg(from.toString(), to.toString()));
        // Fetch data here
    } else {
        QDate tempFrom = from;
        QDate tempTo = tempFrom.addDays(30);

        while (tempFrom <= to) {
            if (tempTo > to)
                tempTo = to;

            emit logMessage(QString("Fetching from %1 to %2").arg(tempFrom.toString(), tempTo.toString()));
            // Fetch data here

            tempFrom = tempTo.addDays(1);
            tempTo = tempFrom.addDays(30);
        }
    }
}

void DataFetcher::fetchMultipleYears(const QDate &from, const QDate &to) {
    QDate tempFrom = from;
    QDate tempTo = tempFrom.addDays(30);

    while (tempFrom <= to) {
        int year = tempFrom.year();
        QDate yearEnd(year, 12, 31);

        if (tempTo > to)
            tempTo = to;
        else if (tempTo > yearEnd)
            tempTo = yearEnd;

        emit logMessage(QString("Fetching from %1 to %2 (Year: %3)").arg(tempFrom.toString(), tempTo.toString()).arg(year));
        // Fetch data here

        tempFrom = tempTo.addDays(1);
        tempTo = tempFrom.addDays(30);
    }
}*/
