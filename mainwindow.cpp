#include "mainwindow.h"
#include "ui_mainwindow.h"
QXLSX_USE_NAMESPACE
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    ui->setupUi(this);

    // ========== 1. 修复表格不显示：绑定模型到 tableView（核心） ==========
    m_tableModel = new QStandardItemModel(this);
    m_tableModel->setColumnCount(1);
    m_tableModel->setHeaderData(0, Qt::Horizontal, "Value");
    ui->tableView->setModel(m_tableModel); // 必须加这行，表格才会显示数据

    // ========== 2. 初始化图表组件 ==========
    m_series = new QLineSeries();
    m_chart = new QChart();
    m_chart->addSeries(m_series);
    m_chart->createDefaultAxes();
    m_chart->setTitle("Data Visualization");

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // ========== 新增：设置chartView的大小策略（关键！） ==========
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // （让chartView能跟随父控件（Splitter）的大小自动扩展）

    // 3. Splitter布局处理
    ui->tableView->setParent(nullptr);
    QList<QWidget*> splitterWidgets = ui->splitter->findChildren<QWidget*>();
    for (QWidget* w : splitterWidgets) {
        w->setParent(nullptr);
    }
    ui->splitter->addWidget(ui->tableView);
    ui->splitter->addWidget(m_chartView);

    // 4. 强制设置Splitter的初始大小比例
    QList<int> sizes;
    sizes << 100 << 400; // 左侧占100px，右侧占400px（和1:4比例对应）
    ui->splitter->setSizes(sizes);

    // 原拉伸比例设置（保留）
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 4);
    setupTab2Plot();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupTab2Plot(){
    // splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, ui->tab_2);

    // 左侧容器
    QWidget* leftWidget = new QWidget;
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);

    plotPCF  = new QCustomPlot;
    plotPACF = new QCustomPlot;
    plotBIC  = new QCustomPlot;

    leftLayout->addWidget(plotPCF);
    leftLayout->addWidget(plotPACF);

    splitter->addWidget(leftWidget);
    splitter->addWidget(plotBIC);

    QHBoxLayout* mainLayout = new QHBoxLayout(ui->tab_2);
    mainLayout->addWidget(splitter);
    ui->tab_2->setLayout(mainLayout);

}

//导入数据
void MainWindow::on_actiondaorushuju_triggered()
{
    QList<QVector<double>>observes;
    QStringList  sheetNameList;
    QString filePath = QFileDialog::getOpenFileName(
        nullptr, // 父窗口指针，暂时设为 nullptr
        QObject::tr("导入数据"),
        QDir::currentPath(), // 初始目录
        QObject::tr("Excel文件 (*.xlsx *.xls)") // 过滤器
        );

    if (filePath.isEmpty()) {
        // 用户取消导入
        return ;
    }

    // 2. 读取 Excel 文件 (对应 IWorkbook st = new XSSFWorkbook(s);)
    // QXlsx 推荐使用 Document 类来处理 .xlsx 文件
    QXlsx::Document xlsx(filePath);

    if (!xlsx.load()) {
        qDebug() << "无法打开文件:" << filePath;
        return;
    }

    // 3. 遍历工作表 (对应 for (int i = 0; i < st.NumberOfSheets; i++))
    // QXlsx 将所有 sheet name 作为一个列表返回
    QStringList sheetNames = xlsx.sheetNames();

    for (const QString& sheetName : sheetNames)
    {
        Worksheet* sheet = static_cast<Worksheet*>(xlsx.sheet(sheetName));
        if (!sheet) {
            // 某些 sheet 可能无法加载，跳过
            continue;
        }

        // 获取工作表的边界 (对应 sheet.LastRowNum)
        // QXlsx 的行索引从 1 开始
        int rowCount = sheet->dimension().lastRow();

        sheetNameList.append(sheetName); // 存储表名

        if (rowCount < 1)
        {
            QMessageBox::warning(nullptr, QObject::tr("警告"), QObject::tr("警告：表 %1 导入的数据不存在").arg(sheetName));
            continue;
        }

        // 准备存储观测值 (对应 m_Observes.Add(new double[...]))
        // 这里的 QXlsx rowCount 已经包含了数据的总行数
        sheetObserves.reserve(rowCount); // 预分配内存

        // 4. 遍历行并读取第一列 (对应 for (int j = 0; j <= sheet.LastRowNum; j++))
        // 循环从第 1 行开始 (j=1) 到最后一行 rowCount
        for (int row = 1; row <= rowCount; ++row)
        {
            // 获取第一列的值 (列索引为 1)
            Cell* cell = sheet->cellAt(row, 1).get();

            if (cell)
            {
                // QXlsx 默认使用 QVariant，我们尝试转换为 double
                QVariant cellValue = cell->value();
                bool ok;
                double value = cellValue.toDouble(&ok);

                if (ok) {
                    sheetObserves.append(value);
                } else {
                    // 如果转换失败，可能是空或非数字，可以根据需要处理错误
                    qDebug() << "Warning: Could not convert cell (" << row << ", 1) to double. Value:" << cellValue;
                    // 为了与 C# 保持一致，这里假设 C# 的 Convert.ToDouble 成功
                }
            }
            // 否则，该单元格为空。C# 代码似乎没有检查空单元格，这里也简化处理。
        }
        qDebug() <<sheetObserves.size();
        observes.append(sheetObserves);
    }

    // 5. 数据导入成功 (对应 toolStripStatusLabel2.Text)
    qDebug() << "数据导入成功! 导入了" << observes.size() << "张表。";


    //左侧可视化
    // 刷新表格数据
    m_tableModel->removeRows(0, m_tableModel->rowCount());
    for (int i = 0; i < sheetObserves.size(); ++i)
    {
        QStandardItem* item = new QStandardItem(QString::number(sheetObserves[i]));
        m_tableModel->appendRow(item);
    }
    qDebug() << "rows =" << m_tableModel->rowCount()
             << "cols =" << m_tableModel->columnCount();

    // ========== 修复图表数据：X轴用实际索引，且强制刷新坐标轴 ==========
    m_series->clear();
    for (int i = 0; i < sheetObserves.size(); ++i)
    {
        m_series->append(i, sheetObserves[i]); // X轴用索引i（0~1899+）
    }

    // ========== 强制设置X轴和Y轴范围（彻底解决压缩问题） ==========
    m_chart->createDefaultAxes(); // 先自动计算，再手动覆盖
    QValueAxis* xAxis = qobject_cast<QValueAxis*>(m_chart->axisX());
    QValueAxis* yAxis = qobject_cast<QValueAxis*>(m_chart->axisY());
    if (xAxis && yAxis) {
        // 1. 手动设置X轴范围：0到数据总量-1（覆盖1900个点）
        xAxis->setRange(0, sheetObserves.size() - 1);
        xAxis->setLabelFormat("%d"); // 显示整数索引
        xAxis->setTickCount(10); // 只显示10个刻度，避免拥挤

        // 2. 手动设置Y轴范围：适配数据的最小/最大值
        double yMin = *std::min_element(sheetObserves.begin(), sheetObserves.end());
        double yMax = *std::max_element(sheetObserves.begin(), sheetObserves.end());
        yAxis->setRange(yMin, yMax);
        // 可选：加10%余量，避免数据贴边
        yAxis->setRange(yMin - 0.1*(yMax-yMin), yMax + 0.1*(yMax-yMin));
    }

    // 强制重绘图表
    m_chartView->repaint();
    return ;


}
QVector<double> eigenToVectorR(const Eigen::MatrixXd& m)
{
    QVector<double> v;
    for (int i = 0; i < m.rows(); ++i)
        v.push_back(m(i, 0));
    return v;
}
QVector<double> eigenToVectorV(const Eigen::MatrixXd& m)
{
    QVector<double> v;
    for (int i = 0; i < m.cols(); ++i)
        v.push_back(m(0, i));
    return v;
}



//AR建模的具体函数
void MainWindow::on_actioncal_triggered()
{
    //做一些检验的工作

    if (sheetObserves.isEmpty())
    {
        QMessageBox::warning(this, "提示", "没有数据，请先导入数据！");
        return;
    }

    if (ui->lineEdit->text().isEmpty() &&
        ui->lineEdit_2->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "请选择建模期数和预测期数！");
        return;
    }
    else if (ui->lineEdit_2->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "请选择预测期数！");
        return;
    }
    else if (ui->lineEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "请选择建模期数！");
        return;
    }

    double level=0.0;

    bool ok3 = false;
    level = ui->lineEdit_4->text().toDouble(&ok3);

    if (!ok3)
    {
        QMessageBox::warning(this, "提示", "显著水平必须为 double 类型数据！");
        return;
    }

    if (level <= 0.0 || level >= 1.0)
    {
        QMessageBox::warning(this, "提示", "显著水平必须大于0且小于1！");
        return;
    }

    if (ui->lineEdit_4->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "请选择预测步数！");
        return;
    }



    Eigen::MatrixXd data = Eigen::Map<Eigen::MatrixXd>(
        sheetObserves.data(),
        sheetObserves.size(),
        1
        );
    int T_lag = 40;
    Eigen::MatrixXd m_PeriodicTerm;
    Eigen::MatrixXd Deal_Y =  CARFunction::RemovePeridTerm(data,  m_PeriodicTerm);
    //取得实际参与建模的数据
    int rownum=ui->lineEdit->text().toInt();
    Eigen::MatrixXd Deal_Y_Model= Deal_Y.block(0,0,rownum,Deal_Y.cols());
    Eigen::MatrixXd Deal_Y_PreModel=Deal_Y.block(rownum,0,Deal_Y.rows()-rownum,Deal_Y.cols());
    m_PCF = CARFunction::GetACF(Deal_Y_Model, T_lag);
    m_PACF=CARFunction:: GetPACF( T_lag, m_PCF);
    m_BIC = CARFunction::GetBICAndAIC(Deal_Y_Model, T_lag,m_AIC);

    drawPCF();
    drawPACF();
    drawBIC_AIC();


    Eigen::Index r, c;
    m_BIC.minCoeff(&r, &c);
    m_Order = static_cast<int>(r) + 1;//确定模型的阶数
    Eigen::MatrixXd _A;
    m_Perement = CARFunction::LSE(m_Order, Deal_Y_Model,  _A);
    m_FitY = _A * m_Perement;
    m_SourFitY=Deal_Y_Model.block(m_Order,0,Deal_Y_Model.rows()-m_Order,Deal_Y_Model.cols());
    double dFP;//F检验的概率值
    m_TP = CARFunction::ModingTest(m_SourFitY, m_Order, m_FitY, m_Perement, _A, dFP);
    p_value=CARFunction::LjungBoxTest(m_SourFitY,  m_FitY, m_Order);
    setupTestResultTable( dFP,  m_TP,  p_value);

    Eigen::MatrixXd m_f_temp=m_PeriodicTerm.block(m_Order,0,Deal_Y_Model.rows()-m_Order,m_PeriodicTerm.cols());
    m_FitY = m_FitY +m_PeriodicTerm.block(m_Order,0,Deal_Y_Model.rows()-m_Order,m_PeriodicTerm.cols());
    m_SourFitY = m_SourFitY + m_PeriodicTerm.block(m_Order,0,Deal_Y_Model.rows()-m_Order,m_PeriodicTerm.cols());//加上去掉的趋势项

    int PreDays = ui->lineEdit_2->text().toInt();//获取预测期数
    m_DeformationModelDays=ui->lineEdit->text().toInt();//获取建模期数
    m_PreStep=ui->lineEdit_3->text().toInt();
    //多步预测
    CARFunction::MulPre(m_Order,
                      PreDays,
                      m_DeformationModelDays,
                      Deal_Y, m_Perement,
                      m_PeriodicTerm,
                      m_PreStep,
                      m_AR_PreDataSource,
                      m_AR_Pre,
                      m_AR_PreEoror);
    PreDays++;

    setupTab3();
    updateTab3Data();
    setupTab4();
    updateTab4Data();


}
void MainWindow::drawPCF()
{
    QVector<double> y = eigenToVectorV(m_PCF);
    QVector<double> x(y.size());

    for (int i = 0; i < x.size(); ++i)
        x[i] = i + 1;

    QCPBars* bars = new QCPBars(plotPCF->xAxis, plotPCF->yAxis);
    bars->setData(x, y);

    plotPCF->xAxis->setLabel("时间延迟");
    plotPCF->yAxis->setLabel("PCF");

    plotPCF->rescaleAxes();
    plotPCF->replot();
}
void MainWindow::drawPACF()
{
    QVector<double> y = eigenToVectorV(m_PACF);
    QVector<double> x(y.size());

    for (int i = 0; i < x.size(); ++i)
        x[i] = i + 1;

    QCPBars* bars = new QCPBars(plotPACF->xAxis, plotPACF->yAxis);
    bars->setData(x, y);

    plotPACF->xAxis->setLabel("时间延迟");
    plotPACF->yAxis->setLabel("PACF");

    plotPACF->rescaleAxes();
    plotPACF->replot();
}

void MainWindow::drawBIC_AIC()
{


    QVector<double> bic = eigenToVectorR(m_BIC);
    QVector<double> aic = eigenToVectorR(m_AIC);

    QVector<double> x(bic.size());
    for (int i = 0; i < x.size(); ++i)
        x[i] = i + 1;

    // --- 1. 寻找 BIC 最小值及其对应的阶数 ---
    auto it = std::min_element(bic.begin(), bic.end());
    double minBic = *it;
    int minIndex = std::distance(bic.begin(), it);
    double bestLag = x[minIndex]; // 最优延迟阶数

    // --- 2. 绘制 BIC 曲线 ---
    plotBIC->addGraph();
    plotBIC->graph(0)->setData(x, bic);
    plotBIC->graph(0)->setName("BIC");
    QPen penBIC(Qt::red);
    penBIC.setWidth(2);
    plotBIC->graph(0)->setPen(penBIC);

    // --- 3. 标注 BIC 最小值点 (使用散点图层高亮) ---
    plotBIC->addGraph();
    plotBIC->graph(1)->setData(QVector<double>{bestLag}, QVector<double>{minBic});
    plotBIC->graph(1)->setPen(QPen(Qt::red));
    plotBIC->graph(1)->setLineStyle(QCPGraph::lsNone); // 不画线，只画点
    plotBIC->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10)); // 实心圆，大小为10
    plotBIC->graph(1)->setName("最优阶数 (BIC最小)");
    plotBIC->graph(1)->removeFromLegend(); // 最小值点通常不需要单独出现在图例中

    // --- 4. 绘制 AIC 曲线 ---
    plotBIC->addGraph();
    plotBIC->graph(2)->setData(x, aic);
    plotBIC->graph(2)->setName("AIC");
    QPen penAIC(Qt::blue);
    penAIC.setWidth(2);
    plotBIC->graph(2)->setPen(penAIC);

    // --- 5. 添加文本标签标注 ---
    QCPItemText *textLabel = new QCPItemText(plotBIC);
    textLabel->position->setType(QCPItemPosition::ptPlotCoords);
    textLabel->position->setCoords(bestLag, minBic); // 设置在最小值点位置
    textLabel->setText(QString("最优阶数: %1\nMin BIC: %2").arg(bestLag).arg(minBic, 0, 'f', 2));
    textLabel->setTextAlignment(Qt::AlignCenter);
    textLabel->setFont(QFont(font().family(), 9));
    textLabel->setPadding(QMargins(5, 5, 5, 5));
    textLabel->setBrush(QBrush(QColor(255, 255, 255, 150))); // 半透明背景，防止遮挡曲线
    textLabel->setPen(QPen(Qt::black));
    // 将标签放置在点上方
    textLabel->setPositionAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    // --- 6. 图表样式与刷新 ---
    plotBIC->legend->setVisible(true);
    plotBIC->xAxis->setLabel("时间延迟");
    plotBIC->yAxis->setLabel("BIC / AIC");

    plotBIC->rescaleAxes();
    // 稍微扩大 y 轴范围，防止标签超出顶部
    plotBIC->yAxis->scaleRange(1.1, plotBIC->yAxis->range().center());

    plotBIC->replot();
}

void MainWindow::setupTab3()
{
    QWidget* tab = ui->tab_3;

    /* ========== 总体左右布局 ========== */
    QHBoxLayout* mainLayout = new QHBoxLayout(tab);

    /* ========== 左侧：表格 ========== */
    m_tableFit = new QTableWidget(tab);
    m_tableFit->setColumnCount(4);
    m_tableFit->setHorizontalHeaderLabels(
        {"期数", "观测值", "拟合值", "残差"}
        );
    m_tableFit->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableFit->verticalHeader()->setVisible(false);

    mainLayout->addWidget(m_tableFit, 3);

    /* ========== 右侧整体 ========== */
    QVBoxLayout* rightLayout = new QVBoxLayout;

    /* ---------- 上：拟合图 ---------- */
    m_plotFit = new QCustomPlot(tab);
    m_plotFit->addGraph(); // 观测
    m_plotFit->addGraph(); // 拟合

    m_plotFit->graph(0)->setName("观测值");
    m_plotFit->graph(0)->setPen(QPen(Qt::green, 1.5));

    m_plotFit->graph(1)->setName("拟合值");
    m_plotFit->graph(1)->setPen(QPen(Qt::red, 1.5));

    m_plotFit->legend->setVisible(true);
    m_plotFit->xAxis->setLabel("期数");
    m_plotFit->yAxis->setLabel("位移 / mm");

    rightLayout->addWidget(m_plotFit, 3);

    /* ---------- 中：残差图 ---------- */
    m_plotRes = new QCustomPlot(tab);
    m_plotRes->addGraph();
    m_plotRes->graph(0)->setName("残差");
    m_plotRes->graph(0)->setPen(QPen(Qt::blue));

    m_plotRes->xAxis->setLabel("期数");
    m_plotRes->yAxis->setLabel("Δ / mm");

    rightLayout->addWidget(m_plotRes, 2);

    /* ---------- 下：指标表 ---------- */
    m_tableMetrics = new QTableWidget(tab);
    m_tableMetrics->setRowCount(2);
    m_tableMetrics->setColumnCount(2);
    m_tableMetrics->setHorizontalHeaderLabels({"指标", "值"});
    m_tableMetrics->verticalHeader()->setVisible(false);
    m_tableMetrics->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_tableMetrics->setItem(0,0,new QTableWidgetItem("RMSE (mm)"));
    m_tableMetrics->setItem(1,0,new QTableWidgetItem("残差标准差 (mm)"));

    rightLayout->addWidget(m_tableMetrics, 1);

    mainLayout->addLayout(rightLayout, 7);
}

void MainWindow::updateTab3Data()
{
    Eigen::MatrixXd& M =m_AR_PreEoror[0];
    Eigen::VectorXd v = M.col(0);

    int n = v.size();

    /* ========== 填表 ========== */
    m_tableFit->setRowCount(n);

    for (int i = 0; i < n; ++i)
    {
        m_tableFit->setItem(i,0,new QTableWidgetItem(QString::number(i+1)));
        m_tableFit->setItem(i,1,new QTableWidgetItem(QString::number(m_AR_PreDataSource[0](i,0),'f',3)));
        m_tableFit->setItem(i,2,new QTableWidgetItem(QString::number(m_AR_Pre[0](i,0),'f',3)));
        m_tableFit->setItem(i,3,new QTableWidgetItem(QString::number(v[i],'f',3)));
    }

    /* ========== 画图数据 ========== */
    QVector<double> x(n), obs(n), fit(n), res(n);
    for (int i = 0; i < n; ++i)
    {
        x[i]   = i + 1;
        obs[i] = m_AR_PreDataSource[0](i,0);
        fit[i] = m_AR_Pre[0](i,0);
        res[i] = v[i];
    }

    /* 拟合图 */
    m_plotFit->graph(0)->setData(x, obs);
    m_plotFit->graph(1)->setData(x, fit);
    m_plotFit->rescaleAxes();
    m_plotFit->replot();

    /* 残差图 */
    m_plotRes->graph(0)->setData(x, res);
    m_plotRes->rescaleAxes();
    m_plotRes->replot();

    /* ========== 指标 ========== */
    double rmse = calcRMSE(v);
    double stdv = calcSTD (v);

    m_tableMetrics->setItem(0,1,new QTableWidgetItem(QString::number(rmse,'f',3)));
    m_tableMetrics->setItem(1,1,new QTableWidgetItem(QString::number(stdv,'f',3)));
}
double MainWindow::calcRMSE(const Eigen::VectorXd& v)
{
    return std::sqrt(v.array().square().mean());
}

double MainWindow::calcSTD(const Eigen::VectorXd& v)
{
    double mean = v.mean();
    return std::sqrt((v.array() - mean).square().mean());
}

void MainWindow::setupTab4()
{
    QWidget* tab = ui->tab_4;

    QHBoxLayout* mainLayout = new QHBoxLayout(tab);

    /* ---------- 左：数据表 ---------- */
    m_tableFit4 = new QTableWidget(tab);
    m_tableFit4->setColumnCount(4);
    m_tableFit4->setHorizontalHeaderLabels(
        {"期数", "原始值", "拟合值", "残差"}
        );
    m_tableFit4->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableFit4->verticalHeader()->setVisible(false);

    mainLayout->addWidget(m_tableFit4, 3);

    /* ---------- 右：图像区 ---------- */
    QVBoxLayout* rightLayout = new QVBoxLayout;

    /* 拟合图 */
    m_plotFit4 = new QCustomPlot(tab);
    m_plotFit4->addGraph(); // 原始
    m_plotFit4->addGraph(); // 拟合

    m_plotFit4->graph(0)->setName("原始数据");
    m_plotFit4->graph(0)->setPen(QPen(Qt::darkGreen, 1.5));

    m_plotFit4->graph(1)->setName("拟合数据");
    m_plotFit4->graph(1)->setPen(QPen(Qt::red, 1.5));

    m_plotFit4->legend->setVisible(true);
    m_plotFit4->xAxis->setLabel("期数");
    m_plotFit4->yAxis->setLabel("数值");

    rightLayout->addWidget(m_plotFit4, 3);

    /* 残差图 */
    m_plotRes4 = new QCustomPlot(tab);
    m_plotRes4->addGraph();
    m_plotRes4->graph(0)->setName("残差");
    m_plotRes4->graph(0)->setPen(QPen(Qt::blue));

    m_plotRes4->xAxis->setLabel("期数");
    m_plotRes4->yAxis->setLabel("残差");

    rightLayout->addWidget(m_plotRes4, 2);

    /* 指标表 */
    m_tableMetrics4 = new QTableWidget(tab);
    m_tableMetrics4->setRowCount(2);
    m_tableMetrics4->setColumnCount(2);
    m_tableMetrics4->setHorizontalHeaderLabels({"指标", "值"});
    m_tableMetrics4->verticalHeader()->setVisible(false);
    m_tableMetrics4->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_tableMetrics4->setItem(0,0,new QTableWidgetItem("RMSE"));
    m_tableMetrics4->setItem(1,0,new QTableWidgetItem("残差标准差"));

    rightLayout->addWidget(m_tableMetrics4, 1);

    mainLayout->addLayout(rightLayout, 7);
}
void MainWindow::updateTab4Data()
{
    int n = m_SourFitY.rows();

    Eigen::VectorXd y0 = m_SourFitY.col(0);
    Eigen::VectorXd y1 = m_FitY.col(0);
    Eigen::VectorXd res = y0 - y1;

    /* ---------- 表格 ---------- */
    m_tableFit4->setRowCount(n);

    for (int i = 0; i < n; ++i)
    {
        m_tableFit4->setItem(i,0,new QTableWidgetItem(QString::number(i+1)));
        m_tableFit4->setItem(i,1,new QTableWidgetItem(QString::number(y0[i],'f',3)));
        m_tableFit4->setItem(i,2,new QTableWidgetItem(QString::number(y1[i],'f',3)));
        m_tableFit4->setItem(i,3,new QTableWidgetItem(QString::number(res[i],'f',3)));
    }

    /* ---------- 图像 ---------- */
    QVector<double> x(n), orig(n), fit(n), r(n);
    for (int i = 0; i < n; ++i)
    {
        x[i]    = i + 1;
        orig[i] = y0[i];
        fit[i]  = y1[i];
        r[i]    = res[i];
    }

    m_plotFit4->graph(0)->setData(x, orig);
    m_plotFit4->graph(1)->setData(x, fit);
    m_plotFit4->rescaleAxes();
    m_plotFit4->replot();

    m_plotRes4->graph(0)->setData(x, r);
    m_plotRes4->rescaleAxes();
    m_plotRes4->replot();

    /* ---------- 指标 ---------- */
    double rmse = std::sqrt(res.array().square().mean());
    double stdv = std::sqrt((res.array() - res.mean()).square().mean());

    m_tableMetrics4->setItem(0,1,new QTableWidgetItem(QString::number(rmse,'f',4)));
    m_tableMetrics4->setItem(1,1,new QTableWidgetItem(QString::number(stdv,'f',4)));
}

void MainWindow::setupTestResultTable(double dFP, const Eigen::VectorXd& m_TP, double lb_PValue) {
    // 1. 获取或创建布局
    if (!ui->tab_5->layout()) {
        ui->tab_5->setLayout(new QVBoxLayout());
    }
    double t = ui->lineEdit_4->text().toDouble();
    // 2. 创建表格
    QTableWidget* table = new QTableWidget(3, 3); // 3行3列
    QString headerText = QString("是否通过 (α=%1)").arg(t, 0, 'f', 2);

    table->setHorizontalHeaderLabels({"检验项目", "P-Value (计算值)", headerText});


    // 设置行高和列宽自适应
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);

    //double t=ui->lineEdit_4.

    // --- 第一行：全局显著性 (F检验) ---
    table->setItem(0, 0, new QTableWidgetItem("全局显著性 (F-test)"));
    table->setItem(0, 1, new QTableWidgetItem(QString::number(dFP, 'f', 4)));
    QString fStatus = (dFP < t) ? "通过" : "未通过";
    table->setItem(0, 2, new QTableWidgetItem(fStatus));

    // --- 第二行：局部显著性 (T检验) ---
    // 取所有参数 P 值的最大值，若最大值都 < 0.05，则全部显著
    double maxTP = m_TP.maxCoeff();
    table->setItem(1, 0, new QTableWidgetItem("局部显著性 (T-test)"));
    table->setItem(1, 1, new QTableWidgetItem(QString("Max P: %1").arg(maxTP, 0, 'f', 4)));
    QString tStatus = (maxTP < t) ? "通过" : "部分不显著";
    table->setItem(1, 2, new QTableWidgetItem(tStatus));

    // --- 第三行：有效性检验 (Ljung-Box) ---
    table->setItem(2, 0, new QTableWidgetItem("模型有效性 (白噪声)"));
    table->setItem(2, 1, new QTableWidgetItem(QString::number(lb_PValue, 'f', 4)));
    // 注意：白噪声检验 P > 0.05 才说明是白噪声（通过）
    QString lbStatus = (lb_PValue >t) ? "通过" : "无效(非白噪声)";
    table->setItem(2, 2, new QTableWidgetItem(lbStatus));

    // 将表格添加到 tab_5
    ui->tab_5->layout()->addWidget(table);
}
