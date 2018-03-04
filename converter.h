#ifndef CONVERTER_H
#define CONVERTER_H

#include <QDateTime>
#include <QMainWindow>

class QComboBox;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QValidator;

namespace Ui {
class Converter;
}

class Converter : public QMainWindow
{
    Q_OBJECT

public:
    explicit Converter(QWidget *parent = 0);
    ~Converter();

private slots:
    void HandleLineEditTextEditedSignal(const QString & inputText);
    void ReplyFinished (QNetworkReply * reply);
    void ChangeCurrencyComboBox(const int newIndexComboBox);

private:
    Ui::Converter *_ui;

    const QVector<QString> _currencyCodes =
    {
        "AUD",
        "BGN",
        "BRL",
        "CAD",
        "CHF",
        "CNY",
        "CZK",
        "DKK",
        "EUR",
        "GBP",
        "HKD",
        "HRK",
        "HUF",
        "IDR",
        "ILS",
        "INR",
        "JPY",
        "KRW",
        "MXN",
        "MYR",
        "NOK",
        "NZD",
        "PHP",
        "PLN",
        "RON",
        "RUB",
        "SEK",
        "SGD",
        "THB",
        "TRY",
        "USD",
        "ZAR"
    };

    static constexpr double INPUT_VAL_MAX = std::numeric_limits<double>::max();
    static constexpr double INPUT_VAL_MIN = 0.0f;
    static constexpr double UPDATE_CURRENCIES_INTERVAL = 3600.0f; // seconds

    QVector<QDateTime> _cachedValueDatetime;
    QNetworkAccessManager * _qNetworkAccessManager;
    QValidator * _qValidator;

    QVector<QVector<double>> _currencyValues;
    QVector<bool> _cached;
    bool _isFirstLineEdit;
    double _inputNumber;
    int _currentIndexComboBox1;
    int _currentIndexComboBox2;

    void ConvertAndShowValue(QLineEdit * const qLineEdit, const int currentIndexFrom, const int currentIndexTo);
    void DisableLineEdit(QLineEdit * const qLineEdit);
    void EnableLineEdit(QLineEdit * const qLineEdit);
    void HandleEnteredText(const QString &inputText, QLineEdit * const qLineEdit, QLineEdit * const otherQLineEdit);
};

#endif // CONVERTER_H
