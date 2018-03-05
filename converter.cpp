#include "converter.h"
#include "ui_converter.h"

#include <QComboBox>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVector>
#include <QtWidgets>
#include <cmath>
#include <iostream>

Converter::Converter(QWidget *parent) :
    QMainWindow(parent),
    _ui(new Ui::Converter),
    _cached(QVector<bool>(_currencyCodes.size())),
    _cachedValueDatetime(QVector<QDateTime>(_currencyCodes.size())),
    _currencyValues(QVector<QVector<double>>(_currencyCodes.size())),
    _currentIndexComboBox1(_currencyCodes.indexOf("GBP")),
    _currentIndexComboBox2(_currencyCodes.indexOf("EUR")),
    _qNetworkAccessManager(new QNetworkAccessManager(this)),
    _qValidator(new QDoubleValidator(INPUT_VAL_MIN, INPUT_VAL_MAX, 2, this))
{
    _ui->setupUi(this);

    // Initialization
    for(int i = 0; i < _currencyValues.size(); i++)
    {
         _currencyValues[i] = QVector<double>(_currencyCodes.size());
    }

    for(const QString currencyCode : _currencyCodes)
    {
        _ui->comboBox->addItem(currencyCode);
        _ui->comboBox_2->addItem(currencyCode);
    }

    _ui->comboBox->setCurrentIndex(_currentIndexComboBox1);
    _ui->comboBox_2->setCurrentIndex(_currentIndexComboBox2);

    // Setting input validation
    _ui->lineEdit->setValidator(_qValidator);

    // Signals to slots connections
    connect(_ui->lineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(HandleLineEditTextEditedSignal(const QString &)));
    connect(_ui->lineEdit_2, SIGNAL(textEdited(const QString &)), this, SLOT(HandleLineEditTextEditedSignal(const QString &)));
    connect(_qNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(ReplyFinished(QNetworkReply*)));
    connect(_ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeCurrencyComboBox(const int)));
    connect(_ui->comboBox_2, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeCurrencyComboBox(const int)));

    // Showing default message in the status bar
    statusBar()->showMessage(tr("Insert the value you want to convert"));

    // The second qLineEdit start disabled
    DisableLineEdit(_ui->lineEdit_2);

    // First conversion starts as soon the program begins
    HandleEnteredText(_ui->lineEdit->text(), _ui->lineEdit, _ui->lineEdit_2);
}

Converter::~Converter()
{
    delete _ui;
}



void Converter::HandleLineEditTextEditedSignal(const QString &inputText)
{
    // The empty string is not filtered by validator
    if(inputText == nullptr || inputText == "")
    {
        return;
    }

    QObject * obj = sender();
    QLineEdit * qLineEdit;
    QLineEdit * otherQLineEdit;
    if(obj == _ui->lineEdit)
    {
        _isFirstLineEdit = true;
        qLineEdit = _ui->lineEdit;
        otherQLineEdit = _ui->lineEdit_2;
    }
    else if(obj == _ui->lineEdit_2)
    {
        _isFirstLineEdit = false;
        qLineEdit = _ui->lineEdit_2;
        otherQLineEdit = _ui->lineEdit;
    }
    else
    {
        return;
    }

    HandleEnteredText(inputText, qLineEdit, otherQLineEdit);
}

void Converter::ReplyFinished(QNetworkReply * reply)
{
    QLineEdit * qLineEdit = _isFirstLineEdit ? _ui->lineEdit : _ui->lineEdit_2;
    QLineEdit * otherQLineEdit = _isFirstLineEdit ? _ui->lineEdit_2 : _ui->lineEdit;

    if(reply->error())
    {
        qDebug() << "ERROR!";
        qDebug() << reply->errorString();
    }
    else
    {
        const int index = _isFirstLineEdit ? _currentIndexComboBox1 : _currentIndexComboBox2;
        const int otherIndex = _isFirstLineEdit ? _currentIndexComboBox2 : _currentIndexComboBox1;

        const QJsonDocument qJsonDocument = QJsonDocument::fromJson(reply->readAll());
        QJsonObject qJsonObject = qJsonDocument.object();
        QJsonValue qJsonValue = qJsonObject.value("rates");
        qJsonObject = qJsonValue.toObject();

        int size = _currencyCodes.size();
        for(int i = 0; i < size; i++)
        {
            QString currencyCode = _currencyCodes[i];
            qJsonValue = qJsonObject.value(currencyCode);
            double currencyRelVal = qJsonValue.toDouble();

            if(i == index)
            {
                continue;
            }

            _currencyValues[index][i] = currencyRelVal;
        }

        _cached[index] = true;
        _cachedValueDatetime[index] = QDateTime::currentDateTime();

        ConvertAndShowValue(otherQLineEdit, index, otherIndex);
    }

    // Reenabling qLineEditInput
    EnableLineEdit(qLineEdit);
    EnableLineEdit(otherQLineEdit);

    reply->deleteLater();
}

void Converter::ChangeCurrencyComboBox(const int newIndexComboBox)
{
    QObject * obj = sender();
    QComboBox * otherComboBox;
    int comboBoxIndex = -1;
    QLineEdit * qLineEdit;
    QLineEdit * otherQLineEdit;
    if(obj == _ui->comboBox)
    {
        otherComboBox = _ui->comboBox_2;
        comboBoxIndex = _currentIndexComboBox1;
        _currentIndexComboBox1 = newIndexComboBox;

        _isFirstLineEdit = true;
        qLineEdit = _ui->lineEdit;
        otherQLineEdit = _ui->lineEdit_2;
    }
    else if(obj == _ui->comboBox_2)
    {
        otherComboBox = _ui->comboBox;
        comboBoxIndex = _currentIndexComboBox2;
        _currentIndexComboBox2 = newIndexComboBox;

        _isFirstLineEdit = false;
        qLineEdit = _ui->lineEdit_2;
        otherQLineEdit = _ui->lineEdit;
    }
    else
    {
        return;
    }

    const int otherComboBoxIndex = otherComboBox->currentIndex();
    if(newIndexComboBox == otherComboBoxIndex)
    {
        otherComboBox->setCurrentIndex(comboBoxIndex);

        return;
    }

    HandleEnteredText(qLineEdit->text(), qLineEdit, otherQLineEdit);

    return;
}


void Converter::HandleEnteredText(const QString &inputText, QLineEdit * const qLineEdit, QLineEdit * const otherQLineEdit)
{
    const int index = _isFirstLineEdit ? _currentIndexComboBox1 : _currentIndexComboBox2;
    const int otherIndex = _isFirstLineEdit ? _currentIndexComboBox2 : _currentIndexComboBox1;

    // The input is converted into a number.
    _inputNumber = ConvertStringToNumber(inputText);

    // Disabling qLineEditInput
    DisableLineEdit(qLineEdit);
    DisableLineEdit(otherQLineEdit);

    // Let's check if we can use a cached value or values needs updating
    bool updateCurrencies = false;
    if(_cached[index])
    {
        int secondsPassedFromLastUpdate = _cachedValueDatetime[index].secsTo(QDateTime::currentDateTime());
        if(secondsPassedFromLastUpdate > UPDATE_CURRENCIES_INTERVAL)
        {
            updateCurrencies = true;
        }
        else
        {
            ConvertAndShowValue(otherQLineEdit, index, otherIndex);
        }
    }
    else
    {
        updateCurrencies = true;
    }

    if(updateCurrencies)
    {
        QString url = QString("https://api.fixer.io/latest?base=") + _currencyCodes[index];
        _qNetworkAccessManager->get(QNetworkRequest(QUrl(url)));
    }
    else
    {
        EnableLineEdit(qLineEdit);
        EnableLineEdit(otherQLineEdit);
    }
}

void Converter::ConvertAndShowValue(QLineEdit * const qLineEdit, const int currentIndexFrom, const int currentIndexTo)
{
    const double relVal = _currencyValues[currentIndexFrom][currentIndexTo];
    double result = _inputNumber * relVal;
    result = roundf(result * 100.0f) / 100.0f;

    qLineEdit->setText(QString::number(result));

    return;
}

void Converter::DisableLineEdit(QLineEdit * const qLineEdit)
{
    qLineEdit->setReadOnly(true);

    // TODO: trasformare in campo della classe
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Base, Qt::white);
    palette->setColor(QPalette::Text, Qt::darkGray);

    qLineEdit->setPalette(*palette);

    return;
}

void Converter::EnableLineEdit(QLineEdit * const qLineEdit)
{
    qLineEdit->setReadOnly(false);
    qLineEdit->setPalette(qLineEdit->style()->standardPalette());

    return;
}

const int Converter::ConvertStringToNumber(const QString &inputText)
{
    const QByteArray qByteArray = inputText.toLatin1();

    const char *inputTextCharArray = qByteArray.data();

    double num = -1.0f;
    int sscanfRetVal = sscanf_s(inputTextCharArray, "%lf", &num);

    if(sscanfRetVal == 0 || sscanfRetVal == EOF)
    {
        return 0;
    }

    return num;
}

