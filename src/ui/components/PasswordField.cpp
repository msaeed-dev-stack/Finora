// =====================================================================
//  PasswordField.cpp
// =====================================================================
#include "PasswordField.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "core/PasswordPolicy.h"
#include "ui/Theme.h"

PasswordField::PasswordField(const QString &accessibleName, bool showStrength, QWidget *parent)
    : QWidget(parent)
{
    edit = new QLineEdit(this);
    edit->setEchoMode(QLineEdit::Password);
    edit->setAccessibleName(accessibleName);
    edit->setMaxLength(MAX_PASSWORD_LENGTH * 4);   // UTF-8 characters can take up to 4 bytes
    // No copying out of a password box: Ctrl+C and the context menu are off.
    edit->setContextMenuPolicy(Qt::NoContextMenu);

    // A real button (not an icon inside the box), so it can also be
    // reached with the Tab key and pressed with Space.
    showButton = new QPushButton(this);
    showButton->setObjectName("iconButton");
    showButton->setCheckable(true);
    showButton->setCursor(Qt::PointingHandCursor);
    showButton->setFixedSize(38, 38);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(8);
    row->addWidget(edit, 1);
    row->addWidget(showButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addLayout(row);

    if (showStrength) {
        meter = new QProgressBar(this);
        meter->setRange(0, 5);
        meter->setTextVisible(false);
        meter->setAccessibleName("Password strength");
        strengthLabel = new QLabel(this);
        strengthLabel->setObjectName("mutedText");
        strengthLabel->setWordWrap(true);
        layout->addWidget(meter);
        layout->addWidget(strengthLabel);
        connect(edit, &QLineEdit::textChanged, this, &PasswordField::updateStrength);
        updateStrength();
    }

    connect(showButton, &QPushButton::clicked, this, &PasswordField::toggleVisibility);
    connect(edit, &QLineEdit::returnPressed, this, &PasswordField::returnPressed);
    refreshIcons();
}

std::string PasswordField::takePassword()
{
    std::string password = edit->text().toStdString();
    edit->clear();
    return password;
}

QLineEdit *PasswordField::lineEdit() const
{
    return edit;
}

bool PasswordField::isPasswordVisible() const
{
    return passwordVisible;
}

QString PasswordField::strengthText() const
{
    if (strengthLabel == nullptr) {
        return QString();
    }
    return strengthLabel->text();
}

void PasswordField::refreshIcons()
{
    QString iconName = "eye";
    QString label = "Show password";
    if (passwordVisible) {
        iconName = "eye_off";
        label = "Hide password";
    }
    showButton->setIcon(QIcon(tintedIcon(iconName, themeColors().text, 18)));
    showButton->setToolTip(label);
    showButton->setAccessibleName(label);
}

void PasswordField::toggleVisibility()
{
    passwordVisible = !passwordVisible;
    if (passwordVisible) {
        edit->setEchoMode(QLineEdit::Normal);
    } else {
        edit->setEchoMode(QLineEdit::Password);
    }
    showButton->setChecked(passwordVisible);
    refreshIcons();
}

// The meter shows the strength in 5 steps, in words and in colour.
void PasswordField::updateStrength()
{
    std::string password = edit->text().toStdString();
    if (password.empty()) {
        meter->setValue(0);
        setTone(meter, "neutral");
        strengthLabel->setText("Use at least " + QString::number(MIN_PASSWORD_LENGTH)
                               + " characters, mixing letters, numbers and symbols.");
        return;
    }

    PasswordStrength strength = passwordStrength(password);
    std::string tip = passwordTip(password);
    meter->setValue(static_cast<int>(strength) + 1);   // 1 to 5 filled steps

    QString tone = "success";
    if (strength <= STRENGTH_WEAK) {
        tone = "danger";
    } else if (strength == STRENGTH_FAIR) {
        tone = "warning";
    }
    setTone(meter, tone);

    QString text = "Strength: " + QString::fromStdString(passwordStrengthName(strength));
    if (!tip.empty()) {
        text += " · " + QString::fromStdString(tip);
    }
    strengthLabel->setText(text);
    meter->setAccessibleDescription(text);
}
