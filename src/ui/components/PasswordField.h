// =====================================================================
//  PasswordField.h
//  A password box with a "show password" button and, if wanted, a
//  strength meter that updates while the person types.
// =====================================================================
#ifndef FINORA_PASSWORDFIELD_H
#define FINORA_PASSWORDFIELD_H

#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

#include <string>

class PasswordField : public QWidget
{
    Q_OBJECT

public:
    // showStrength adds the meter and its tip below the box (for new passwords).
    PasswordField(const QString &accessibleName, bool showStrength, QWidget *parent = nullptr);

    // The typed password as std::string, and the box is emptied at once,
    // so the text stays in the window as briefly as possible. The receiver
    // (AuthService) wipes the std::string after use.
    std::string takePassword();

    QLineEdit *lineEdit() const;
    bool isPasswordVisible() const;
    QString strengthText() const;   // for example "Strength: Fair", for the tests
    void refreshIcons();

signals:
    void returnPressed();

private slots:
    void toggleVisibility();
    void updateStrength();

private:
    QLineEdit *edit = nullptr;
    QPushButton *showButton = nullptr;
    QProgressBar *meter = nullptr;          // only with showStrength
    QLabel *strengthLabel = nullptr;        // only with showStrength
    bool passwordVisible = false;
};

#endif // FINORA_PASSWORDFIELD_H
