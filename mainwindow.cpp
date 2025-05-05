






/*#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include <QTime>
#include <QSqlRecord>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *dateLayout = new QHBoxLayout();
    fromDateEdit = new QDateEdit(QDate::currentDate());
    toDateEdit = new QDateEdit(QDate::currentDate());
    fetchButton = new QPushButton("Fetch");
    exportButton = new QPushButton("Export");

    dateLayout->addWidget(fromDateEdit);
    dateLayout->addWidget(toDateEdit);
    dateLayout->addWidget(fetchButton);
    dateLayout->addWidget(exportButton);

    logLabel = new QLabel("Ready");
    startTimeLabel = new QLabel("Start Time: N/A");
    endTimeLabel = new QLabel("End Time: N/A");
    totalTimeLabel = new QLabel("Total Time: N/A");
    totalRecordsLabel = new QLabel("Total Records: N/A");

    mainLayout->addLayout(dateLayout);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(startTimeLabel);
    mainLayout->addWidget(endTimeLabel);
    mainLayout->addWidget(totalTimeLabel);
    mainLayout->addWidget(totalRecordsLabel);

    setCentralWidget(centralWidget);

    connect(fetchButton, &QPushButton::clicked, this, &MainWindow::onFetchClicked);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
}

MainWindow::~MainWindow() {
    closeAllDatabases();
}

void MainWindow::logMessage(const QString &msg) {
    QMetaObject::invokeMethod(this, [this, msg]() {
            logLabel->setText(msg);
            qDebug() << msg;
        }, Qt::QueuedConnection);
}

bool MainWindow::checkSqlDrivers() {
    if (QSqlDatabase::drivers().isEmpty()) {
        logMessage("No SQL drivers available.");
        return false;
    }
    return true;
}

bool MainWindow::checkDatabaseExists(int year) {
    QString dbPath = QString("/home/thermo/database_1/TsxUlt42_%1.sqlite").arg(year);
    return QFile::exists(dbPath);
}

void MainWindow::openDatabaseForYear(int year) {
    if (yearDbMap.contains(year)) return;
    QString dbPath = QString("/home/thermo/database_1/TsxUlt42_%1.sqlite").arg(year);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QString::number(year));
    db.setDatabaseName(dbPath);
    if (db.open()) {
        yearDbMap.insert(year, db);
    } else {
        logMessage("Failed to open database for year: " + QString::number(year));
    }
}

void MainWindow::closeAllDatabases() {
    for (auto db : yearDbMap.values()) {
        if (db.isOpen()) db.close();
    }
    yearDbMap.clear();
}

void MainWindow::onFetchClicked() {
    QDate fromDate = fromDateEdit->date();
    QDate toDate = toDateEdit->date();

    if (!checkSqlDrivers()) return;
    if (fromDate > toDate) {
        logMessage("Selected dates are not valid.");
        return;
    }

    // Disable button during operation
    fetchButton->setEnabled(false);
    exportButton->setEnabled(false);

    // Clear previously fetched data
    fetchedRecords.clear();

    QtConcurrent::run([=]() {
        fetchData(fromDate, toDate);

        QMetaObject::invokeMethod(this, [this]() {
                fetchButton->setEnabled(true);
                exportButton->setEnabled(true);
                logMessage("Fetch completed.");
            }, Qt::QueuedConnection);
    });
}

void MainWindow::fetchFromDatabase(QSqlDatabase& db, const QDate& from, const QDate& to) {
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

void MainWindow::fetchData(const QDate &fromDate, const QDate &toDate) {
    if (fromDate > toDate) {
        logMessage("Selected dates are not valid");
        return;
    }

    QTime startTime = QTime::currentTime();
    QMetaObject::invokeMethod(this, [=]() {
            startTimeLabel->setText("Start Time: " + startTime.toString("hh:mm:ss"));
        }, Qt::QueuedConnection);

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

    QTime endTime = QTime::currentTime();

    QMetaObject::invokeMethod(this, [=]() {
            endTimeLabel->setText("End Time: " + endTime.toString("hh:mm:ss"));
            int totalTime = startTime.msecsTo(endTime);
            totalTimeLabel->setText("Total Time: " + QString::number(totalTime / 1000.0) + " seconds");
            totalRecordsLabel->setText("Total Records: " + QString::number(totalRecordsFetched));
            onExportClicked();
        }, Qt::QueuedConnection);
}

void MainWindow::onExportClicked() {
    if (fetchedRecords.isEmpty()) {
        logMessage("No data to export.");
        return;
    }

    QtConcurrent::run([=]() {
        auto exportStartTime = QDateTime::currentDateTime();

        QString filePath = "/home/thermo/Export.csv";
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logMessage("Failed to open file for writing: " + filePath);
            return;
        }

        QTextStream out(&file);

        // Write CSV header
        out << "Date_Time,xValTime,yValRTD,yValTC1,yValTC2,yValTC3,yValTC4,yValTC5,yValTC6,yValTC7,yValTC8,yValTC9,yValTC10,"
               "yValSetPoint,MainBatVoltage,BusBatVoltage,LineInput,State,Offset,WarmWarn,ColdWarn,HSCOMP_SPEED,LSCOMP_SPEED,"
               "FanRPM,CaptubeState,AlgoState,BUSRTD,CalcRTD,RecordHashChart\n";

        // Write records
        for (const QStringList &row : fetchedRecords) {
            out << row.join(",") << "\n";
        }

        file.close();

        auto exportEndTime = QDateTime::currentDateTime();
        qint64 exportDuration = exportStartTime.msecsTo(exportEndTime);

        QMetaObject::invokeMethod(this, [=]() {
                logMessage("Export completed: " + filePath + " (Time: " + QString::number(exportDuration/1000.0) + " seconds)");
            }, Qt::QueuedConnection);
    });
}*/


