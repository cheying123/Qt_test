#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,isScientificMode(false)            //默认为隐藏模式
    ,isProgrammerMode(false)            //默认为隐藏模式
    , currentBase(10)                   // 默认十进制
    ,isDateMode(false)                  //默认为隐藏模式
{
    ui->setupUi(this);
    ui->groupScientific->hide();        //初始时隐藏起来（初始为标准模式）
    ui->groupProgrammer->hide();        //初始时隐藏起来
    ui->groupDate->hide();

    digitBTNs={
        {Qt::Key_0,ui->btnNum0},  //<int,button>
        {Qt::Key_1,ui->btnNum1},
        {Qt::Key_2,ui->btnNum2},
        {Qt::Key_3,ui->btnNum3},
        {Qt::Key_4,ui->btnNum4},
        {Qt::Key_5,ui->btnNum5},
        {Qt::Key_6,ui->btnNum6},
        {Qt::Key_7,ui->btnNum7},
        {Qt::Key_8,ui->btnNum8},
        {Qt::Key_9,ui->btnNum9},
    };

    operators = {
        {Qt::Key_Plus,ui->btnPlus},
        {Qt::Key_Minus,ui->btnMinus},
        {Qt::Key_Asterisk,ui->btnMutli},
        {Qt::Key_Slash,ui->btnDivide}
    };

    foreach(auto btn,operators){
        connect(btn,SIGNAL(clicked()),this,SLOT(binaryOperatorClicked()));
    }

    foreach(auto btn,digitBTNs ){
        connect(btn,SIGNAL(clicked()),this,SLOT(btnNumClicked()));      //每一个都过一遍，但只有点击那个才能响应
    }


    // 初始化汇率表
    exchangeRates = {
        {"USD->CNY", 7.2},    // 美元 -> 人民币
        {"CNY->USD", 0.14},   // 人民币 -> 美元
        {"USD->EUR", 0.92},   // 美元 -> 欧元
        {"EUR->USD", 1.09},   // 欧元 -> 美元
        {"CNY->EUR", 0.13},   // 人民币 -> 欧元
        {"EUR->CNY", 7.5}     // 欧元 -> 人民币
    };

    // 初始化金额 ComboBox
    ui->comboBoxCurrencyFrom->addItems({"USD", "CNY", "EUR"});
    ui->comboBoxCurrencyTo->addItems({"USD", "CNY", "EUR"});

    // 设置默认值
    ui->comboBoxCurrencyFrom->setCurrentText("USD");
    ui->comboBoxCurrencyTo->setCurrentText("CNY");

    // 初始化网络管理器
    networkManager = new QNetworkAccessManager(this);



    // 初始化转换表表
    exchanges = {
        {"L->ml", 1000.0},     // 升 -> 毫升
        {"ml->L", 0.001},      // 毫升 -> 升
        {"L->gal", 0.264172},   // 升 -> 加仑
        {"gal->L", 3.78541},    // 加仑 -> 升
        {"ml->gal", 0.000264172}, // 毫升 -> 加仑
        {"gal->ml", 3785.41}    // 加仑 -> 毫升
    };
    // 初始化金额 ComboBox
    ui->comboBoxFrom->addItems({"ml", "L", "gal"});
    ui->comboBoxTo->addItems({"ml", "L", "gal"});

    // 设置默认值
    ui->comboBoxCurrencyFrom->setCurrentText("L");
    ui->comboBoxCurrencyTo->setCurrentText("ml");

    //语言切换设置
    ui->languageComboBox->addItems({"中文","English"});

    // 设置默认语言为英文
    changeLanguage("en");

    // 设置语言选择框默认值
    ui->languageComboBox->setCurrentText("English");

    // 加载用户设置
    loadUserSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::calculation(){                          //计算处理
    double result = NULL;
    QString Error = "除数不能为0";

    double operand1 = operands.front().toDouble();          //第一个数
    operands.pop_front();

    double operand2 = last_num.toDouble();

    if( operands.size() == 1 ){                             //如果还有数字，就更新最后处理数
        operand2 = operands.front().toDouble();             //第二个数
        last_num = operands.front();                        //最后一个处理数
        operands.pop_front();
    }



    QString code = opcode;

    qDebug()<<operand1<<code<<operand2;

    if( code == "+" ){
        result = operand1 + operand2;
    }else if( code == "-"){
            result = operand1 - operand2;
    }else if( code == "×" ){
        result = operand1 * operand2;
    }else if( code == "/"){
        if( operand2 == 0 ){
            return Error;
        }else{
            result = operand1 / operand2;
        }
    }
    QString message = QString("%1 %2 %3").arg(operand1).arg(code).arg(operand2);

    ui->statusbar->showMessage(message);

    // 记录历史
    HistoryEntry entry;
    entry.expression = message;
    entry.result = QString::number(result);
    entry.mode = "Standard"; // 根据当前模式设置
    history.append(entry);

    return QString::number(result);
}

void MainWindow::on_btnEqual_clicked()                      // "="的处理
{
    if (!operand.isEmpty()) {  // 如果当前有输入数字
        operands.push_back(operand);  // 将当前数字存入操作数列表
        operand = "";  // 清空当前输入
    }

    QString result = "";
    if ((operands.size() == 2 || !last_num.isEmpty()) && !opcode.isEmpty()) {  // 如果有两个操作数或一个操作数和处理数
        if (opcode == "AND" || opcode == "OR" || opcode == "XOR") {
            // 按位操作
            result = bitwiseCalculation();  // 执行按位计算
        }else {
            // 常规计算
            result = calculation();  // 执行普通四则运算
        }

        if (result != "除数不能为0" && result != "Error") {  // 处理有效结果
            operand = result;
        }

        qDebug() << "operand:" << operand;
        ui->display->setText(result);  // 显示计算结果
    } else {
        if (!operands.isEmpty()) {
            operand = operands.front();
            operands.pop_front();
        }
    }
}


void MainWindow::binaryOperatorClicked(){                   // 加减乘除 操作

    if( qobject_cast<QPushButton*>(sender())->text() != "=" ){
        opcode = qobject_cast<QPushButton*>(sender())->text();  //读取操作符(你的点击)
    }
    qDebug()<<"读取的操作符1:"<<opcode;


    if( operand != "" ){                                    //数据读取及其存储
        operands.push_back(operand);                        //将符号前的数字放入栈中
        operand = "";

        QString result = "";
        if((operands.size() == 2) && opcode != ""){
            result = calculation();                         //结果输出

            //operands.push_back(result);                     //将结果放入
            if( result != "除数不能为0" ){
                operand = result;
            }
            qDebug()<<"operand:"<<operand;
            ui->display->setText(result);
        }

        qDebug()<<"读取的操作符:"<<opcode;
        qDebug()<<"储存的数据数:"<<operands.size();

    }

}

void MainWindow::btnNumClicked()            //数字点击
{
    QString digit = qobject_cast<QPushButton*>(sender())->text();       //将要添加的 字符 。

    if( operand == "0" ){               //当前只有0  （后面应该只用判断是否小数点就行了）
        if( digit == "0" ){             //输入为0
            digit = "";
        }
        else if( digit != "." ){        //输入不为0且不为小数点
            digit = "";
        }
    }

    operand += digit;

    ui->display->setText(operand);
    QString last_num = "    the last num: ";
    if( !operands.empty() ){
        last_num += operands.top();
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked" + last_num );
    //staatusbar表示的是下面的状态栏
    //qobject_cast<QPushButton*>(sender())->text() 是获取点击按钮的text文本，并且返回

}




void MainWindow::on_btnPeriod_clicked()         //小数点
{
    if(operand.contains(".") || operand == ""){             //如果包含了小数点 或者 当前输入栏目为空
        return ;
    }
    operand += qobject_cast<QPushButton*>(sender())->text();

    ui->display->setText(operand);
    QString last_num = "    the last num: ";
    if( !operands.empty() ){
        last_num += operands.top();
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked" + last_num );
}


void MainWindow::on_btnDel_clicked()            //删除最右边的一个数字
{
    operand = operand.left(operand.length() - 1);
    ui->display->setText(operand);
    QString last_num = "    the last num: ";
    if( !operands.empty() ){
        last_num += operands.top();
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked" + last_num );
}


void MainWindow::on_btnClear_clicked()          //全部清除
{
    operand.clear();                            //当前数字清除
    opcode.clear();                             //当前操作符清除
    last_num.clear();                           //处理数清除
    ui->display->setText(operand);
    while( !operands.empty() ){                 //将存储的全部清除
        operands.pop();
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnClearError_clicked()
{
    operand.clear();                            //当前数字清除
    ui->display->setText(operand);
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::keyPressEvent(QKeyEvent *event)        //键盘事件
{
    foreach(auto btnKey,digitBTNs.keys()){              //有键盘输入就每一个都过一遍，只有满足的才进行输出
        if( event->key() == btnKey ){
            digitBTNs[btnKey]->animateClick();
        }
    }

    foreach(auto btnKey,operators.keys()) {
        if( event->key() == btnKey ){
            operators[btnKey]->animateClick();
        }
    }

}


void MainWindow::on_btnSquare_clicked()         //将当前文本的数字进行平方
{
    qDebug()<<"operand:"<<operand;
    if(operand != ""){
        double number = operand.toDouble();
        number = pow(number,2);
        operand = QString::number(number);
        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}


void MainWindow::on_btnSqtr_clicked()           //对当前数进行开根号
{
    if(operand != ""){
        double number = operand.toDouble();
        number = sqrt(number);
        operand = QString::number(number);
        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}


void MainWindow::on_btnInverse_clicked()        //对当前数进行倒数处理
{
    if(operand != ""){
        double number = operand.toDouble();
        number = 1.0/number;
        operand = QString::number(number);
        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}


void MainWindow::on_btnPercentage_clicked()     //变成百分比数
{
    if(operand != ""){
        double number = operand.toDouble();
        number = number/100.0;
        operand = QString::number(number);
        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}


void MainWindow::on_btnSign_clicked()
{
    if(operand != ""){
        double number = operand.toDouble();
        number = -number;
        operand = QString::number(number);
        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

//科学模式

void MainWindow::on_btnScientificMode_clicked()
{
    if (!isScientificMode) { //如果原为false
        ui->groupScientific->show(); // 显示科学计算相关按钮
        isScientificMode = !isScientificMode;
    } else {                //如果原为true
        ui->groupScientific->hide(); // 隐藏科学计算相关按钮
        isScientificMode = !isScientificMode;
    }
}

void MainWindow::on_btnSin_clicked()
{
    if (!operand.isEmpty()) {
        double number = operand.toDouble();
        double radians = number * M_PI / 180.0;     // 将角度转换为弧度
        operand = QString::number(sin(radians));    // 计算结果

        // 记录历史
        QString message = QString("sin(%1)").arg(number);  // 记录表达式
        HistoryEntry entry;
        entry.expression = message;
        entry.result = operand;  // 结果
        entry.mode = "Trigonometric"; // 设置当前模式
        history.append(entry);

        ui->display->setText(operand);  // 显示结果
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnCos_clicked()
{
    if (!operand.isEmpty()) {
        double number = operand.toDouble();
        double radians = number * M_PI / 180.0;     // 将角度转换为弧度
        operand = QString::number(cos(radians));    // 计算结果

        // 记录历史
        QString message = QString("cos(%1)").arg(number);
        HistoryEntry entry;
        entry.expression = message;
        entry.result = operand;
        entry.mode = "Trigonometric";
        history.append(entry);

        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnTan_clicked()
{
    if (!operand.isEmpty()) {
        double number = operand.toDouble();
        double radians = number * M_PI / 180.0;     // 将角度转换为弧度
        operand = QString::number(tan(radians));    // 计算结果

        // 记录历史
        QString message = QString("tan(%1) ").arg(number);
        HistoryEntry entry;
        entry.expression = message;
        entry.result = operand;
        entry.mode = "Trigonometric";
        history.append(entry);

        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnLog_clicked()
{
    if (!operand.isEmpty()) {
        double number = operand.toDouble();
        operand = QString::number(log10(number)); // 计算结果

        // 记录历史
        QString message = QString("log(%1) ").arg(number);
        HistoryEntry entry;
        entry.expression = message;
        entry.result = operand;
        entry.mode = "Logarithmic";
        history.append(entry);

        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnLn_clicked()
{
    if (!operand.isEmpty()) {
        double number = operand.toDouble();
        operand = QString::number(log(number)); // 计算结果

        // 记录历史
        QString message = QString("ln(%1) ").arg(number);
        HistoryEntry entry;
        entry.expression = message;
        entry.result = operand;
        entry.mode = "Logarithmic";
        history.append(entry);

        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnExp_clicked()
{
    if (!operand.isEmpty()) {
        double number = operand.toDouble();
        operand = QString::number(exp(number)); // 计算结果

        // 记录历史
        QString message = QString("exp(%1) ").arg(number);
        HistoryEntry entry;
        entry.expression = message;
        entry.result = operand;
        entry.mode = "Exponential";
        history.append(entry);

        ui->display->setText(operand);
    }
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnPi_clicked()
{
    operand = QString::number(M_PI);

    // 记录历史
    QString message = QString("π");
    HistoryEntry entry;
    entry.expression = message;
    entry.result = operand;
    entry.mode = "Constant";
    history.append(entry);

    ui->display->setText(operand);
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}

void MainWindow::on_btnE_clicked()
{
    operand = QString::number(M_E);

    // 记录历史
    QString message = QString("e ");
    HistoryEntry entry;
    entry.expression = message;
    entry.result = operand;
    entry.mode = "Constant";
    history.append(entry);

    ui->display->setText(operand);
    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}



//程序员模式


void MainWindow::on_btnOperatorMode_clicked(){}     //暂时废弃


void MainWindow::on_btnProgrammerMode_clicked()
{
    if (!isProgrammerMode) { //如果原为false
        ui->groupProgrammer->show(); // 显示科学计算相关按钮
        isProgrammerMode = !isProgrammerMode;
    } else {                //如果原为true
        ui->groupProgrammer->hide(); // 隐藏科学计算相关按钮
        isProgrammerMode = !isProgrammerMode;
    }
}

// 进制转换函数
QString MainWindow::convertBase(QString value, int fromBase, int toBase)
{
    bool ok;
    int decimalValue = value.toInt(&ok, fromBase);          //输入当前进制和数字，方便装换，ok返回的是成功与否
    if (!ok) return "Error";
    return QString::number(decimalValue, toBase).toUpper(); // 转换为目标进制
}

void MainWindow::on_btnBase2_clicked()
{
    if(!operand.isEmpty()){
        operand = convertBase(operand, currentBase, 2);        //转换为目标进制
        currentBase = 2;                                       //修改当前进制
        ui->display->setText(operand);                         //展示出来

        ui->statusbar->showMessage(tr("已经切换到2进制"));
    }
}



void MainWindow::on_btnBase8_clicked()
{
    if(!operand.isEmpty()){
        operand = convertBase(operand, currentBase, 8);        //转换为目标进制
        currentBase = 8;                                       //修改当前进制
        ui->display->setText(operand);                         //展示出来

        ui->statusbar->showMessage(tr("已经切换到8进制"));
    }
}


void MainWindow::on_btnBase10_clicked()
{
    if(!operand.isEmpty()){
        operand = convertBase(operand, currentBase, 10);        //转换为目标进制
        currentBase = 10;                                       //修改当前进制
        ui->display->setText(operand);                         //展示出来

        ui->statusbar->showMessage(tr("已经切换到10进制"));
    }
}


void MainWindow::on_btnBase16_clicked()
{
    if(!operand.isEmpty()){
        operand = convertBase(operand, currentBase, 16);        //转换为目标进制
        currentBase = 16;                                       //修改当前进制
        ui->display->setText(operand);                         //展示出来

        ui->statusbar->showMessage(tr("已经切换到16进制"));
    }
}


void MainWindow::on_btnAnd_clicked() {
    handleBitwiseOperation("AND");
}

void MainWindow::on_btnOr_clicked() {
    handleBitwiseOperation("OR");
}

void MainWindow::on_btnXor_clicked() {
    handleBitwiseOperation("XOR");
}


void MainWindow::handleBitwiseOperation(const QString &operation) {
    if (!operand.isEmpty()) {  // 如果当前有输入数字
        operands.push_back(operand);  // 存储当前数字
        operand = "";
    }

    if (!operation.isEmpty()) {
        opcode = operation;  // 设置当前操作符为按位操作符
    }

    qDebug() << "按位操作符:" << opcode;

    if (operands.size() == 2 && !opcode.isEmpty()) {  // 如果已经有两个操作数且有操作符
        QString result = bitwiseCalculation();  // 执行按位运算
        if (result != "Error") {  // 如果结果有效
            operand = result;
        }
        ui->display->setText(result);  // 显示计算结果
    }
}

QString MainWindow::bitwiseCalculation() {
    if (operands.size() < 2 || opcode.isEmpty()) {  // 确保至少有两个操作数和操作符
        return "Error";
    }

    bool ok1, ok2;
    int a = operands.front().toInt(&ok1, currentBase);  // 以当前进制解析第一个数字
    operands.pop_front();  // 移除第一个数字

    int b = operands.front().toInt(&ok2, currentBase);  // 以当前进制解析第二个数字
    operands.pop_front();  // 移除第二个数字

    if (!ok1 || !ok2) {
        return "Error";  // 如果解析失败
    }

    int result = 0;  // 存储按位操作结果
    if (opcode == "AND") {
        result = a & b;
    } else if (opcode == "OR") {
        result = a | b;
    } else if (opcode == "XOR") {
        result = a ^ b;
    } else {
        return "Error";  // 未知操作符
    }

    QString message = QString("%1 %2 %3 = %4").arg(a).arg(opcode).arg(b).arg(result);
    // 记录历史
    HistoryEntry entry;
    entry.expression = message;
    entry.result = QString::number(result);
    entry.mode = "Programmer"; // 根据当前模式设置
    history.append(entry);

    opcode = "";  // 重置操作符
    return QString::number(result, currentBase).toUpper();  // 返回结果，按当前进制
}

void MainWindow::on_btnNot_clicked(){}


void MainWindow::on_btnLeftShift_clicked()
{
    int a = operand.toInt(nullptr, currentBase);
    operand = QString::number(a << 1, currentBase).toUpper();
    ui->display->setText(operand);

    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}


void MainWindow::on_btnEqual_11_clicked(){}


void MainWindow::on_btnRightShift_clicked()
{
    int a = operand.toInt(nullptr, currentBase);
    operand = QString::number(a >> 1, currentBase).toUpper();
    ui->display->setText(operand);

    ui->statusbar->showMessage(qobject_cast<QPushButton*>(sender())->text() + "  btn clicked");
}


//日期计算模式

void MainWindow::on_btnDataMode_clicked()
{
    if (!isDateMode) { //如果原为false
        ui->groupDate->show(); // 显示科学计算相关按钮
        isDateMode = !isDateMode;
    } else {                //如果原为true
        ui->groupDate->hide(); // 隐藏科学计算相关按钮
        isDateMode = !isDateMode;
    }
}



void MainWindow::on_btnDateSum_clicked()
{
    if (!operand.isEmpty()) {  // 如果当前有输入数字
        operands.push_back(operand);  // 将当前数字存入操作数列表
        operand = "";  // 清空当前输入
    }

    opcode = "DateSum";  // 设置操作符为日期加法

    QString result = dateCalculation();  // 执行日期计算
    ui->display->setText(result);  // 显示计算结果
}

void MainWindow::on_btnDateSub_clicked()
{
    if (!operand.isEmpty()) {  // 如果当前有输入数字
        operands.push_back(operand);  // 将当前数字存入操作数列表
        operand = "";  // 清空当前输入
    }

    opcode = "DateSub";  // 设置操作符为日期减法

    QString result = dateCalculation();  // 执行日期计算
    ui->display->setText(result);  // 显示计算结果
}

QString MainWindow::dateCalculation()
{
    // 获取日期输入
    QDate date1 = ui->dateEdit->date();  // 获取第一个日期
    QString day = "0";  // 初始化天数为默认值0

    // 检查 `operands` 是否为空
    if (!operands.isEmpty()) {
        day = operands.front();  // 从操作数列表中获取天数
        operands.pop_front();    // 移除第一个操作数
    } else if (!last_num.isEmpty()) {
        day = last_num;  // 如果没有新的操作数，则使用上一次的值
    }

    qDebug() << "Current day:" << day << ", opcode:" << opcode;  // 调试日志

    if (opcode == "DateSum") {
        // 日期加法
        QDate resultDate = date1.addDays(day.toInt());  // 将天数加到 date1 上
        last_num = day;  // 记录当前操作数

        // 记录历史
        QString message = QString("%1 %2 %3 ")
                              .arg(date1.toString("yyyy-MM-dd"))  // 操作的起始日期
                              .arg("+")                        // 操作符（如 "DateSum" 或 "DateSub"）
                              .arg(day.toInt());                       // 计算的天数

        HistoryEntry entry;
        entry.expression = message;
        entry.result = resultDate.toString("yyyy-MM-dd");
        entry.mode = "Date"; // 根据当前模式设置
        history.append(entry);

        return resultDate.toString("yyyy-MM-dd");  // 返回结果
    } else if (opcode == "DateSub") {
        // 日期减法
        QDate resultDate = date1.addDays(-day.toInt());  // 从 date1 中减去天数
        last_num = day;  // 记录当前操作数

        // 记录历史
        QString message = QString("%1 %2 %3 ")
                              .arg(date1.toString("yyyy-MM-dd"))  // 操作的起始日期
                              .arg("-")                        // 操作符（如 "DateSum" 或 "DateSub"）
                              .arg(day.toInt());                       // 计算的天数

        HistoryEntry entry;
        entry.expression = message;
        entry.result = resultDate.toString("yyyy-MM-dd");
        entry.mode = "Date"; // 根据当前模式设置
        history.append(entry);

        return resultDate.toString("yyyy-MM-dd");  // 返回结果
    }

    return "";  // 如果没有匹配的操作符，返回空字符串
}




void MainWindow::on_btnDateDiff_clicked()
{
    QDate date1 = ui->dateEdit->date();
    QDate date2 = ui->dateEdit2->date();

    int daysDiff = date1.daysTo(date2);  // 计算两个日期之间的天数差

    // 记录历史
    QString message = QString("%1 %2 %3 ")
                          .arg(date2.toString("yyyy-MM-dd"))  // 操作的起始日期
                          .arg("-")                        // 操作符（如 "DateSum" 或 "DateSub"）
                          .arg(date1.toString("yyyy-MM-dd"));                       // 计算的天数

    HistoryEntry entry;
    entry.expression = message;
    entry.result = QString::number(daysDiff);  // 将结果转换为字符串
    entry.mode = "Date"; // 根据当前模式设置
    history.append(entry);


    ui->display->setText(QString::number(daysDiff));  // 显示结果
}



void MainWindow::onConvertClicked(){}       //废弃
void MainWindow::onSaveRateClicked(){}      //废弃

void MainWindow::on_btnChangeMoney_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::on_btnConvertCurrency_clicked()
{
    QString fromCurrency = ui->comboBoxCurrencyFrom->currentText();  // 获取源货币
    QString toCurrency = ui->comboBoxCurrencyTo->currentText();      // 获取目标货币
    QString key = fromCurrency + "->" + toCurrency;                 // 生成汇率表键值

    // 检查源货币与目标货币是否相同
    if (fromCurrency == toCurrency) {
        ui->lineMoneyEdit2->setText(ui->lineMoneyEdit->text());  // 金额直接复制到目标输入框
        return;
    }

    // 检查输入金额是否合法
    double amount = ui->lineMoneyEdit->text().toDouble();
    if (amount <= 0) {
        QMessageBox::warning(this, "Invalid Input", "请输入有效金额！");
        return;
    }

    // 填写您的 API 密钥
    QString apiKey = "51f43aa5be8ee16565ec7184"; // 替换为您的实际API密钥

    // 构造 API URL
    QString apiUrl = QString("https://v6.exchangerate-api.com/v6/%1/latest/%2")
                         .arg(apiKey)
                         .arg(fromCurrency);

    QUrl url(apiUrl); // 构造 QUrl 对象
    QNetworkRequest request(url); // 用 QUrl 构造 QNetworkRequest

    // 发送网络请求
    QNetworkReply *reply = networkManager->get(request);

    // 处理网络请求完成的信号
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            if (jsonObj.contains("conversion_rates")) {
                QJsonObject rates = jsonObj.value("conversion_rates").toObject();
                if (rates.contains(toCurrency)) {
                    double rate = rates.value(toCurrency).toDouble(); // 获取目标货币的汇率
                    double convertedAmount = amount * rate;
                    ui->lineMoneyEdit2->setText(QString::number(convertedAmount, 'f', 2));
                    ui->labelCurrencyResult->setText(
                        tr("实时汇率：1 %1 = %2 %3").arg(fromCurrency).arg(rate).arg(toCurrency));
                }
            }
        } else {
            QMessageBox::warning(this, "Error", "无法获取实时汇率，请检查网络连接！");
        }
        reply->deleteLater(); // 释放内存
    });
}




void MainWindow::on_btnSetCustomRate_clicked()
{
    QString fromCurrency = ui->comboBoxCurrencyFrom->currentText();  // 获取源货币
    QString toCurrency = ui->comboBoxCurrencyTo->currentText();      // 获取目标货币
    QString key = fromCurrency + "->" + toCurrency;          // 生成汇率表键值

    // 检查输入的汇率是否合法
    double customRate = ui->lineMoneyEdit2->text().toDouble();
    if (customRate <= 0) {
        QMessageBox::warning(this, "Invalid Rate", "Please enter a valid exchange rate.");
        return;
    }

    // 更新汇率表
    exchangeRates[key] = customRate;
    QMessageBox::information(this, "Rate Saved", QString("Custom rate saved: 1 %1 = %2 %3")
                                                     .arg(fromCurrency).arg(customRate).arg(toCurrency));
}

//容量单位转换
void MainWindow::on_btnConvert_clicked()
{
    QString from = ui->comboBoxFrom->currentText();  // 获取源单位
    QString to = ui->comboBoxTo->currentText();      // 获取目标单位
    QString key = from + "->" + to;          // 生成单位表键值

    // 检查源货币与目标货币是否相同
    if (from== to) {
        ui->lineEdit2->setText(ui->lineEdit->text());  // 数量直接复制到目标输入框
        return;
    }

    // 检查输入金额是否合法
    double amount = ui->lineEdit->text().toDouble();
    if (amount <= 0) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid amount.");
        return;
    }

    // 查找汇率并计算
    if (exchanges.contains(key)) {
        double rate = exchanges[key];
        double convertedAmount = amount * rate;
        // 在 QLineEdit 中显示转换结果
        ui->lineEdit2->setText(QString::number(convertedAmount, 'f', 5));  // 显示结果，保留5位小数

        ui->labelResult->setText(tr("Converted Amount: %1 %2").arg(convertedAmount).arg(to));
    } else {
        ui->labelCurrencyResult->setText("Conversion rate not available.");
    }
}

//历史记录
void MainWindow::on_pushButton_3_clicked()      //历史记录界面跳转
{
    ui->stackedWidget->setCurrentIndex(2);
    on_btnUpdataHistory_clicked();  // 更新历史记录显示
}


void MainWindow::on_btnback_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::on_btnUpdataHistory_clicked()
{
    ui->historyListWidget->clear();
    for (const auto& entry : history) {
        QString itemText = QString("%1 = %2 (%3)").arg(entry.expression).arg(entry.result).arg(entry.mode);
        new QListWidgetItem(itemText, ui->historyListWidget);
    }
}


void MainWindow::on_btnHistoryClear_clicked()
{
    history.clear();
    on_btnUpdataHistory_clicked();
}

// -----------------------语言装换------------------------------------------

void MainWindow::changeLanguage(const QString &languageCode) {
    // 卸载之前的翻译
    qApp->removeTranslator(&translator);

    // 获取程序当前可执行文件的路径
    QString basePath = QCoreApplication::applicationDirPath(); // 可执行文件所在目录
    QString translationFile;

    if (languageCode == "zh_CN") {
        translationFile = QDir::cleanPath(basePath + "/../resource/language/lang_zh_CN.qm"); // 规范化路径
    } else if (languageCode == "en") {
        translationFile = QDir::cleanPath(basePath + "/../resource/language/lang_en.qm"); // 规范化路径
    } else {
        qDebug() << "Unsupported language code: " << languageCode;
        return;
    }

    // 加载新的翻译文件
    if (translator.load(translationFile)) {
        qApp->installTranslator(&translator);

        // 更新界面翻译
        ui->retranslateUi(this);

        qDebug() << "语言切换成功：" << languageCode;
    } else {
        qDebug() << "Failed to load translation file: " << translationFile;
    }
}






void MainWindow::on_languageComboBox_currentTextChanged(const QString &language)
{
    if (language == "English") {
        changeLanguage("en");
    } else if (language == "中文") {
        changeLanguage("zh_CN");
    }
}


void MainWindow::on_btnDayNight_clicked()
{
    // 使用按钮的属性记录当前是否是夜间模式
    bool isNightMode = ui->btnDayNight->property("isNightMode").toBool();

    // 获取当前背景颜色样式（如果存在自定义背景颜色，保留）
    QString currentBackgroundColor;
    QRegularExpression bgColorRegex("background-color:([^;]+);");
    QRegularExpressionMatch match = bgColorRegex.match(this->styleSheet());
    if (match.hasMatch()) {
        currentBackgroundColor = match.captured(1);
    } else {
        currentBackgroundColor = isNightMode ? "white" : "#333"; // 默认颜色
    }

    // 更新样式表（仅更新字体颜色和按钮样式，保留背景颜色）
    QString newStyle;
    if (isNightMode) {
        // 切换到白天模式
        newStyle = QString(
                       "QMainWindow { background-color: %1; color: black; }"
                       "QPushButton { background-color: white; color: black; border: 1px solid gray; font-size:14pt; }"
                       "QPushButton::hover { background-color: green; }"
                       "QPushButton::pressed { background-color: red; }")
                       .arg(currentBackgroundColor);
        ui->btnDayNight->setText(tr("夜间模式"));
    } else {
        // 切换到夜间模式
        newStyle = QString(
                       "QMainWindow { background-color: %1; color: white; }"
                       "QPushButton { background-color: #444; color: white; font-size:14pt; }"
                       "QPushButton::hover { background-color: green; }"
                       "QPushButton::pressed { background-color: red; }")
                       .arg(currentBackgroundColor);
        ui->btnDayNight->setText(tr("白天模式"));
    }

    // 更新按钮属性
    ui->btnDayNight->setProperty("isNightMode", !isNightMode);

    // 应用新的样式表
    this->setStyleSheet(newStyle);

    // 更新界面以应用新样式
    this->update();
}




//自定义

void MainWindow::on_btnback_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::on_btnCustomize_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
}


void MainWindow::on_btnBackgroundColor_clicked()
{
    // 打开颜色选择对话框
    QColor color = QColorDialog::getColor(Qt::white, this, "选择背景颜色");
    if (color.isValid())
    {
        // 获取当前样式表
        QString currentStyleSheet = this->styleSheet();

        // 替换或追加背景颜色样式
        QRegularExpression bgColorRegex("background-color:([^;]+);");
        if (bgColorRegex.match(currentStyleSheet).hasMatch()) {
            currentStyleSheet.replace(bgColorRegex, QString("background-color: %1;").arg(color.name()));
        } else {
            currentStyleSheet += QString("QMainWindow { background-color: %1; }").arg(color.name());
        }

        // 应用新的样式表
        this->setStyleSheet(currentStyleSheet);

        // 保存背景颜色到用户设置
        QSettings settings("MyCompany", "MyApp");
        settings.setValue("backgroundColor", color.name());
    }
}

void MainWindow::on_btnTextColor_clicked(){}    //暂时废弃


void MainWindow::on_fontSizeSlider_valueChanged(int value)
{
    QString buttonStyle = QString(
                              "QPushButton { font-size: %1pt; background-color: white; color: black; border: 1px solid gray; }"
                              "QPushButton::hover { background-color: green; }"
                              "QPushButton::pressed { background-color: red; }"
                              ).arg(value);

    // 获取当前样式表
    QString currentStyle = this->styleSheet();

    // 删除旧的按钮样式
    currentStyle.remove(QRegularExpression("QPushButton\\s*\\{[^\\}]*font-size:[^;]+;[^\\}]*\\}"));

    // 添加新的按钮样式
    currentStyle += buttonStyle;

    // 更新样式表
    this->setStyleSheet(currentStyle);

    // 保存设置
    QSettings settings("MyCompany", "MyApp");
    settings.setValue("fontSize", value);
}

//加载用户设置
void MainWindow::loadUserSettings()
{
    QSettings settings("MyCompany", "MyApp");

    // 加载背景颜色
    QString bgColor = settings.value("backgroundColor", "#ffffff").toString();
    QString newStyle = QString("QMainWindow { background-color: %1; }").arg(bgColor);

    // 加载字体大小
    int fontSize = settings.value("fontSize", 14).toInt();

    // 更新按钮样式
    on_fontSizeSlider_valueChanged(fontSize);

    // 应用样式表
    this->setStyleSheet(newStyle);

    // 更新滑块位置
    ui->fontSizeSlider->setValue(fontSize);
}





