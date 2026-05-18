#include "carfunction.h"
using namespace boost::math;

const double CARFunction::PI = 3.14159265358979323846;
CARFunction::CARFunction()
{
    //PI = 3.14159265358979323846;
}

Eigen::MatrixXd CARFunction:: RemovePeridTerm(Eigen::MatrixXd Y,  Eigen::MatrixXd &m_PeriodicTerm)
{
    Eigen::MatrixXd A (Y.rows(), 4);//构造系数矩阵
    for (int i = 0; i < Y.rows(); i++)
    {
        A(i, 0) = 1;
        A(i, 1) = (i + 1) * 1.0 / 365;
        A(i, 2) = sin(2 * PI * (i + 1) / 365.0);
        A(i, 3) = cos(2 * PI * (i + 1) / 365.0);
    }
    Eigen::MatrixXd Q=A.transpose() * A;
    Eigen::MatrixXd phi =Q.inverse()*A.transpose() * Y;
    m_PeriodicTerm = A * phi;
    Eigen::MatrixXd Deal_Y = Y - A * phi;
    return Deal_Y;
}

Eigen::MatrixXd CARFunction:: GetACF(Eigen::MatrixXd Deal_Y_Model, int T_lag)
{

    Eigen::MatrixXd PCF(1,T_lag + 1);
    PCF(0, 0) = 1;
    Eigen::MatrixXd temp=(Deal_Y_Model.transpose())*Deal_Y_Model;
    double R0=temp(0,0)/Deal_Y_Model.rows();//序列方差
    for (int i = 0; i < T_lag; i++)
    {
         // Eigen::MatrixXd temp33=Deal_Y_Model.block()
        //Eigen::MatrixXd subMatrix = originalMatrix.block(r1, 0, numRows, originalMatrix.cols());

        Eigen::MatrixXd temp1=Deal_Y_Model.block(0,0,Deal_Y_Model.rows()-i-1,Deal_Y_Model.cols());
        Eigen::MatrixXd temp2=Deal_Y_Model.block(i+1,0,Deal_Y_Model.rows()-i-1,Deal_Y_Model.cols());
        double R =(temp1.transpose()*temp2)(0,0)/(Deal_Y_Model.rows() - i - 1);
        PCF(0, i + 1) = R/R0;
    }
    return PCF;
}

Eigen::MatrixXd CARFunction:: GetPACF(int T_lag, Eigen::MatrixXd PCF)
{
    Eigen::MatrixXd PACF(1, T_lag + 1);
    PACF(0, 0) = PCF(0, 0);
    for (int i = 0; i < T_lag; i++)
    {
        Eigen::MatrixXd TR= Eigen::MatrixXd::Identity(i+1, i+1);
        for (int j = 0; j <= i; j++)
        {
            double k = PCF(0, j);
            TR(0, j) = PCF(0, j);
        }
        for (int m = 1; m <= i; m++)
        {
            // Eigen::MatrixXd _ACFm(1,1);
            // _ACFm(0, 0) = PCF(0, m);
            // Eigen::MatrixXd tempp=TR.block(m - 1, 0, 1, i);
            // TR.row(m) =(_ACFm(0,0),TR.block(m - 1, 0, 1, i));
            double pcf_value = PCF(0, m);



            TR.block(m, 0, 1, 1 + i) <<
                Eigen::MatrixXd::Constant(1, 1, pcf_value), // 1x1 部分
                TR.block(m - 1, 0, 1, i);
        }
        Eigen::MatrixXd Phi = TR.inverse() * PCF.block(0, 1, PCF.rows(), i + 1).transpose();
        PACF(0, i + 1) = Phi(i, 0);
    }
    return PACF;
}



Eigen::MatrixXd CARFunction::GetBICAndAIC(Eigen::MatrixXd Deal_Y_Model, int T_lag,Eigen::MatrixXd & AIC)
{
    AIC.resize(T_lag, 1);
    Eigen::MatrixXd BIC (T_lag, 1);
    for (int i = 0; i < T_lag; i++)
    {
        Eigen::MatrixXd A ;
        for (int m = 0; m <= i; m++)
        {
            int r1 = m;
            int r2 = Deal_Y_Model.rows() - 1 - i + m - 1;
             int rows = r2 - r1 + 1;
             int cols = Deal_Y_Model.cols();
             Eigen::MatrixXd R=Deal_Y_Model.block(r1, 0, rows, cols);
             if (A.size()==0){
                 A= R;
             }
            else
            {
                Eigen::MatrixXd tmp(rows, R.cols() + A.cols());

                tmp.block(0, 0, rows, R.cols()) = R;
                tmp.block(0, R.cols(), rows, A.cols()) = A;

                A.swap(tmp);
            }

            Eigen::MatrixXd tempR=Deal_Y_Model.block(i+1,0,Deal_Y_Model.rows() - i-1,Deal_Y_Model.cols());
            Eigen::MatrixXd PHI = (A.transpose()*A).inverse()*A.transpose()*tempR;
            //Eigen::MatrixXd PHI = Matrix.MatrixInv(~A * A) * ~A * Deal_Y_Model.GetR(i + 1, Deal_Y_Model.Rows - 1);
            Eigen::MatrixXd Error =Deal_Y_Model.block(i+1,0,Deal_Y_Model.rows()-i-1,Deal_Y_Model.cols())-A*PHI;
            //Eigen::MatrixXd Error = Deal_Y_Model.GetR(i + 1, Deal_Y_Model.Rows - 1) - A * PHI;
            double E = 0.0;
            for (int q = 0; q < Error.rows(); q++)
            {
                E += pow(Error(q, 0), 2);
            }

            AIC(i, 0) = log(E / Error.rows()) + 2 * (i + 1) / Deal_Y_Model.rows();
            BIC(i, 0) = log(E / Error.rows()) + (i + 1) * log(Deal_Y_Model.rows())/ Deal_Y_Model.rows();
        }
    }
    return BIC;
}
Eigen::MatrixXd CARFunction::LSE(int m_Order, Eigen::MatrixXd Deal_Y_Model, Eigen::MatrixXd &_A)
{
    _A.resize(0, 0);
    for (int i = 0; i < m_Order; i++)
    {
        int r1 = i;
        int r2 = Deal_Y_Model.rows() - 1 - m_Order + 1 + i - 1;
        int rows = r2 - r1 + 1;
        int cols = Deal_Y_Model.cols();
        Eigen::MatrixXd R=Deal_Y_Model.block(r1,0,rows,cols);
        if (_A.size()==0)
        {
            _A=R;
        }
        else
        {
            Eigen::MatrixXd tmp(rows, R.cols() + _A.cols());
            tmp.block(0, 0, rows, R.cols()) = R;
            tmp.block(0, R.cols(), rows, _A.cols()) = _A;
            _A.swap(tmp);
        }
    }
    Eigen::MatrixXd tempR=Deal_Y_Model.block(m_Order,0,Deal_Y_Model.rows() -m_Order,Deal_Y_Model.cols());
    Eigen::MatrixXd m_Perement=(_A.transpose()*_A).inverse()*_A.transpose()*tempR;
    return m_Perement;
}

double CARFunction::LjungBoxTest(Eigen::MatrixXd Y, Eigen::MatrixXd FitY, int m_Order)
{
    int n = FitY.rows();
    // 1. 计算残差序列 e = Y - FitY
    Eigen::VectorXd res = Y.col(0) - FitY.col(0);

    // 2. 确定延迟阶数 p (对应图片公式中的求和上限)
    // 通常取 ln(n) 或固定取 10-20，这里取 10 以保证检验强度
    int p = (m_Order + 10 > n / 2) ? n / 2 : m_Order + 10;

    double res_mean = res.mean();
    double res_var = (res.array() - res_mean).square().sum();

    // 3. 计算自相关系数 rho_k 并求平方和
    double sum_rho_sq = 0.0;
    for (int k = 1; k <= p; ++k) {
        double numerator = 0.0;
        for (int i = 0; i < n - k; ++i) {
            numerator += (res(i) - res_mean) * (res(i + k) - res_mean);
        }
        double rho_k = numerator / res_var; // 自相关系数
        sum_rho_sq += std::pow(rho_k, 2);   // 累加平方
    }

    // 4. 计算 Q 统计量 (严格按照图片公式: Q = n * sum(rho^2))
    double Q = n * sum_rho_sq;

    // 5. 计算 P 值
    // 根据图片，Q 统计量服从自由度为 p 的卡方分布
    // 注意：在 AR 拟合中，更精确的自由度通常是 p - m_Order
    int df = p - m_Order;
    if (df <= 0) df = 1;

    using boost::math::chi_squared;
    chi_squared dist(df);

    // P-value 为 1 - CDF
    double p_value = 1.0 - boost::math::cdf(dist, Q);

    return p_value;
}

Eigen::VectorXd CARFunction::ModingTest(Eigen::MatrixXd Y, int m_Order, Eigen::MatrixXd FitY, Eigen::MatrixXd m_Perement, Eigen::MatrixXd _A,  double & dFP)
{

    int N = FitY.rows();
    int n1 = m_Order;
    int n2 = N - m_Order - 1;

    if (n2 <= 0) {
        dFP = 1.0; // 自由度不足，无法拒绝原假设
        return Eigen::VectorXd::Zero(m_Order);
    }

    // 1. 计算平方和 (使用离差形式)
    double meanY = Y.mean();
    double dSSE = (FitY - Y).squaredNorm();                // 残差平方和
    double dSSR = (FitY.array() - meanY).square().sum();   // 回归平方和

    // 2. F 检验 (全局显著性)
    double dF = (dSSR / n1) / (dSSE / n2);
    fisher_f f_dist(n1, n2);
    dFP = 1.0 - cdf(f_dist, dF);

    // 3. T 检验 (参数显著性)
    double dD2 = dSSE / n2; // 使用自由度修正后的剩余方差
    Eigen::MatrixXd Qphi = (_A.transpose() * _A).inverse();

    Eigen::VectorXd m_TP(m_Order);
    students_t t_dist(n2);

    for (int i = 0; i < m_Order; i++)
    {
        double standard_error = sqrt(Qphi(i, i) * dD2);
        double t_stat = m_Perement(i, 0) / standard_error;

        // 使用双侧检验计算 P 值
        m_TP(i) = 2.0 * (1.0 - cdf(t_dist, std::abs(t_stat)));
    }

    return m_TP;
}

void CARFunction::MulPre(
    int m_Order,
    int PreDays,
    int m_DeformationModelDays,
    const Eigen::MatrixXd& Deal_Y,
    const Eigen::MatrixXd& m_Perement,      // m_Order × 1
    const Eigen::MatrixXd& m_PeriodicTerm,
    int i_PreStep,
    std::vector<Eigen::MatrixXd>& AR_PreDataSource,
    std::vector<Eigen::MatrixXd>& AR_Pre,
    std::vector<Eigen::MatrixXd>& AR_PreError)
{
    // ---------- 安全检查 ----------
    if (m_Perement.rows() != m_Order || m_Perement.cols() != 1)
        throw std::runtime_error("m_Perement must be (m_Order × 1)");

    // ---------- 中间窗口 ----------
    int mid_start = m_DeformationModelDays - m_Order;
    int mid_rows  = PreDays + m_Order;

    Eigen::MatrixXd Mid_Deal_Pre_Data =
        Deal_Y.middleRows(mid_start, mid_rows);

    // ---------- 构造 B1 ----------
    std::vector<Eigen::MatrixXd> B1(m_Order);
    for (int i = 0; i < m_Order; ++i)
    {
        int start = m_Order - i - 1;
        int rows  = Mid_Deal_Pre_Data.rows() - i - 1 - start;
        B1[i] = Mid_Deal_Pre_Data.middleRows(start, rows);
    }

    // ---------- 输出容器 ----------
    AR_Pre.resize(i_PreStep);
    AR_PreDataSource.resize(i_PreStep);
    AR_PreError.resize(i_PreStep);

    // ---------- 多步递推预测 ----------
    for (int i = 0; i < i_PreStep; ++i)
    {
        Eigen::MatrixXd ARPre;

        for (int j = 0; j < m_Order; ++j)
        {
            if (j == 0)
                ARPre = Eigen::MatrixXd::Zero(B1[j].rows(), 1);
            ARPre.noalias() += B1[j] * m_Perement(j, 0);
        }

        // ---------- B1 向后平移 ----------
        for (int p = m_Order - 2; p >= 0; --p)
            B1[p + 1] = B1[p];

        B1[0] = ARPre;

        // ---------- 每个 B1 去掉最后一行 ----------
        for (int k = 0; k < m_Order; ++k)
            B1[k] = B1[k].topRows(B1[k].rows() - 1);

        // ---------- 周期项 ----------
        Eigen::MatrixXd periodic =
            m_PeriodicTerm.middleRows(
                m_DeformationModelDays + i,
                PreDays - i);

        AR_Pre[i] = ARPre + periodic;

        AR_PreDataSource[i] =
            periodic +
            Deal_Y.middleRows(
                m_DeformationModelDays + i,
                PreDays - i);

        AR_PreError[i] = AR_Pre[i] - AR_PreDataSource[i];
    }
}


