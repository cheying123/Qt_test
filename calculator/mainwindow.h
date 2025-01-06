#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Qlabel>
#include <Qstack>
#include <QMap>
#include <QPushButton>
#include <QMessageBox>

#include <QKeyEvent>

#include <QtMath>
#include <cmath> // 用于科学计算功能

#include <bitset> // 用于按位操作

#include <QDate>

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

    QString opcode;             //加减乘除符号存储

    QString operand;            //数字存储
    QStack<QString> operands;
    QMap<int,QPushButton*> digitBTNs;

    QMap<int,QPushButton*> operators;

    QString last_num;           //最后一个处理数，（不是被处理数）

    QString calculation();      //计算

    bool isScientificMode;      // 科学模式标志
    bool isProgrammerMode;      // 程序员模式标志
    bool isDateMode;            // 日期计算模式

    int currentBase;            // 表示当前进制（2=二进制，8=八进制，10=十进制，16=十六进制）

private slots:
    void on_btnEqual_clicked();

    void binaryOperatorClicked();

    void btnNumClicked();

    void on_btnPeriod_clicked();

    void on_btnDel_clicked();

    void on_btnClear_clicked();

    void on_btnSquare_clicked();

    void on_btnSqtr_clicked();

    void on_btnInverse_clicked();

    void on_btnPercentage_clicked();

    void on_btnSign_clicked();

    void on_btnClearError_clicked();

    virtual void keyPressEvent(QKeyEvent *event);

    //科学模式
    void on_btnScientificMode_clicked();

    void on_btnSin_clicked();

    void on_btnCos_clicked();

    void on_btnTan_clicked();

    void on_btnLog_clicked();

    void on_btnLn_clicked();

    void on_btnExp_clicked();

    void on_btnPi_clicked();

    void on_btnE_clicked();


    //程序员模式
    void on_btnOperatorMode_clicked();

    void on_btnProgrammerMode_clicked();

    void on_btnBase2_clicked();

    void on_btnBase8_clicked();

    void on_btnBase10_clicked();

    void on_btnBase16_clicked();

    void on_btnAnd_clicked();

    void on_btnOr_clicked();

    void on_btnXor_clicked();

    void on_btnNot_clicked();

    void on_btnLeftShift_clicked();

    void on_btnEqual_11_clicked();

    void on_btnRightShift_clicked();

    void handleBitwiseOperation(const QString &operation);
    QString bitwiseCalculation();

    //日期计算模式
    void on_btnDateSum_clicked();

    void on_btnDateSub_clicked();

    void on_btnDateDiff_clicked();
    void on_btnDataMode_clicked();

    QString dateCalculation();
    void on_btnChangeMoney_clicked();

    void on_pushButton_clicked();

    void on_btnConvertCurrency_clicked();

    void on_btnSetCustomRate_clicked();

    //汇率转换
    void onConvertClicked();    // 转换按钮点击事件
    void onSaveRateClicked();   // 保存汇率按钮点击事件

    void on_btnConvert_clicked();

private:
    Ui::MainWindow *ui;

    QString convertBase(QString value, int fromBase, int toBase); // 进制转换函数
    QMap<QString, double> exchangeRates;  // 汇率表
    QMap<QString, double> exchanges;       //单位表
};
#endif // MAINWINDOW_H
