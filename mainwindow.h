#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QObject>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QMainWindow>
#include"xlsxdocument.h"
#include<QStringList>
#include<Eigen/Dense>
#include<carfunction.h>
#include<vector>
#include <QtCharts>
#include <QStandardItemModel>
#include "qcustomplot.h"


QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actiondaorushuju_triggered();

    void on_actioncal_triggered();

private:
    Ui::MainWindow *ui;

private:

    //模型拟合
    QCustomPlot* plotPCF;
    QCustomPlot* plotPACF;
    QCustomPlot* plotBIC;

    //预测精度
    QTableWidget* m_tableFit      = nullptr;
    QCustomPlot*  m_plotFit       = nullptr;
    QCustomPlot*  m_plotRes       = nullptr;
    QTableWidget* m_tableMetrics  = nullptr;

    //拟合精度
    QTableWidget* m_tableFit4     = nullptr;
    QCustomPlot*  m_plotFit4      = nullptr;
    QCustomPlot*  m_plotRes4      = nullptr;
    QTableWidget* m_tableMetrics4 = nullptr;

    QStandardItemModel* m_tableModel;
    QChart* m_chart;
    QLineSeries* m_series;
    QChartView* m_chartView;
    int m_DeformationModelDays;//参与建模的期数
    QVector<double> sheetObserves;//原始的观测数据
    Eigen::MatrixXd m_PCF, m_PACF, m_BIC, m_AIC;
    int m_Order;//模型的阶数
    double p_value;//白噪声
    Eigen::MatrixXd m_Perement;//自回归系数
    Eigen::MatrixXd m_FitY;
    Eigen::MatrixXd m_SourFitY;
    Eigen::VectorXd m_TP;
    std::vector<Eigen::MatrixXd> m_AR_PreDataSource, m_AR_Pre, m_AR_PreEoror;
    int m_PreStep;//预测步数
    void setupTab2Plot();
    void drawPCF();
    void drawPACF();
    void drawBIC_AIC();
    void setupTab3();
    void updateTab3Data();
    void setupTab4();
    void updateTab4Data();
    double calcRMSE(const Eigen::VectorXd& v);
    double calcSTD (const Eigen::VectorXd& v);
    void setupTestResultTable(double dFP, const Eigen::VectorXd& m_TP, double lb_PValue);



};
#endif // MAINWINDOW_H
