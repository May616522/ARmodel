#ifndef CARFUNCTION_H
#define CARFUNCTION_H
#include<Eigen/Dense>
#include<cmath>
#include <boost/math/distributions/fisher_f.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include<vector>

class CARFunction
{
    static const double PI ;
public:
    CARFunction();
    static Eigen::MatrixXd RemovePeridTerm(Eigen::MatrixXd Y,  Eigen::MatrixXd &m_PeriodicTerm);
    static Eigen::MatrixXd GetACF(Eigen::MatrixXd Deal_Y_Model, int T_lag);
    static Eigen::MatrixXd GetPACF(int T_lag, Eigen::MatrixXd PCF);
    static double LjungBoxTest(Eigen::MatrixXd Y, Eigen::MatrixXd FitY, int m_Order);
    static Eigen::MatrixXd GetBICAndAIC(Eigen::MatrixXd Deal_Y_Model, int T_lag,Eigen::MatrixXd &AIC);
    static Eigen::MatrixXd LSE(int m_Order, Eigen::MatrixXd Deal_Y_Model, Eigen::MatrixXd &_A);
    static Eigen::VectorXd ModingTest(Eigen::MatrixXd Y, int m_Order, Eigen::MatrixXd FitY,
                                      Eigen::MatrixXd m_Perement, Eigen::MatrixXd _A,  double &dFP);
    static void MulPre(int m_Order,
                             int PreDays,
                             int m_DeformationModelDays,
                             const Eigen::MatrixXd& Deal_Y,
                             const Eigen::MatrixXd& m_Perement,
                             const Eigen::MatrixXd& m_PeriodicTerm,
                             int i_PreStep,
                             std::vector<Eigen::MatrixXd>& AR_PreDataSource,
                             std::vector<Eigen::MatrixXd>& AR_Pre,
                       std::vector<Eigen::MatrixXd>& AR_PreError);
};

#endif // CARFUNCTION_H
