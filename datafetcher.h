// DataFetcher.h
#ifndef DATAFETCHER_H
#define DATAFETCHER_H

#include <QObject>
#include <QDate>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class DataFetcher : public QObject
{
    Q_OBJECT

public:
    explicit DataFetcher(QObject *parent = nullptr);

    Q_INVOKABLE void startExport(const QString &type, const QString &length, const QDate &fromDate, const QDate &toDate);

signals:
    void logMessage(const QString &message);
    void exportComplete();

private:
    void fetchData(const QDate &fromDate, const QDate &toDate);
    void fetchFromDatabase(QSqlDatabase &db, const QDate &fromDate, const QDate &toDate);
    void exportToCSV();
    void exportToPDF();
    void exportToPUC();
    QList<QStringList> fetchedRecords;


    int totalRecordsFetched;



};

#endif // DATAFETCHER_H







/*// datafetcher.h
#ifndef DATAFETCHER_H
#define DATAFETCHER_H

#include <QObject>
#include <QDate>
#include <QSqlDatabase>
#include <QMap>
#include <QStringList>

class DataFetcher : public QObject {
    Q_OBJECT

public:
    explicit DataFetcher(QObject *parent = nullptr);
    void setDateRange(const QDate &from, const QDate &to);
    void startFetch();
    void startExport();

signals:
    void logMessage(const QString &msg);
    void fetchCompleted();
    void exportCompleted(bool success);

private:
    QDate fromDate;
    QDate toDate;
    QMap<int, QSqlDatabase> dbMap;
    QStringList exportBuffer;

    void openDatabases();
    void closeDatabases();
    bool checkDatabaseExists(int year);
    bool checkTableExists(QSqlDatabase &db, const QString &tableName);
    void fetchSingleYear(const QDate &from, const QDate &to);
    void fetchMultipleYears(const QDate &from, const QDate &to);
    void exportToCsv();
    bool isUsbAvailable();
};

#endif // DATAFETCHER_H
*/
