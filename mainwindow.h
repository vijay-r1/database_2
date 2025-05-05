#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include <QSqlDatabase>
#include <QMap>
#include <QDate>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onFetchClicked();
    void onExportClicked();

private:
    void logMessage(const QString &msg);
    bool checkSqlDrivers();
    bool checkDatabaseExists(int year);
    bool checkTableExists(QSqlDatabase &db, const QString &tableName);
    void openDatabaseForYear(int year);
    void closeAllDatabases();
    void fetchData(const QDate &fromDate, const QDate &toDate);
    void fetchFromDatabase(QSqlDatabase &db, const QDate &from, const QDate &to);
    int totalRecordsFetched;
    QList<QStringList> fetchedRecords;



    QDateEdit *fromDateEdit;
    QDateEdit *toDateEdit;
    QPushButton *fetchButton;
    QPushButton *exportButton;
    QLabel *logLabel;
    QLabel *startTimeLabel;
    QLabel *endTimeLabel;
    QLabel *totalTimeLabel;
    QLabel *totalRecordsLabel;// Declare total records label

    QMap<int, QSqlDatabase> yearDbMap;
};

#endif // MAINWINDOW_H
