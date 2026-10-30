// =====================================================================
//  RecoveryCodesPanel.cpp
// =====================================================================
#include "RecoveryCodesPanel.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ui/UiHelpers.h"

RecoveryCodesPanel::RecoveryCodesPanel(QWidget *parent)
    : QWidget(parent)
{
    QWidget *gridArea = new QWidget(this);
    codeGrid = new QGridLayout(gridArea);
    codeGrid->setContentsMargins(0, 0, 0, 0);
    codeGrid->setHorizontalSpacing(10);
    codeGrid->setVerticalSpacing(8);

    copyButton = new QPushButton("Copy codes", this);
    copyButton->setObjectName("secondaryButton");
    copyButton->setCursor(Qt::PointingHandCursor);
    copiedLabel = new QLabel(this);
    copiedLabel->setObjectName("mutedText");

    savedBox = new QCheckBox("I have saved these codes somewhere safe", this);
    savedBox->setObjectName("codesSavedBox");

    QHBoxLayout *copyRow = new QHBoxLayout();
    copyRow->addWidget(copyButton);
    copyRow->addWidget(copiedLabel, 1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(gridArea);
    layout->addLayout(copyRow);
    layout->addWidget(savedBox);

    connect(copyButton, &QPushButton::clicked, this, &RecoveryCodesPanel::copyCodes);
    connect(savedBox, &QCheckBox::toggled, this, &RecoveryCodesPanel::savedChanged);
}

void RecoveryCodesPanel::showCodes(const std::vector<std::string> &newCodes)
{
    clearCodes();
    int count = static_cast<int>(newCodes.size());
    for (int i = 0; i < count; i++) {
        codes.append(QString::fromStdString(newCodes[i]));
        QLabel *codeLabel = new QLabel(QString::number(i + 1) + ".  " + codes.last(), this);
        codeLabel->setObjectName("recoveryCode");
        codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        // Two columns: codes 1-5 on the left, 6-10 on the right.
        codeGrid->addWidget(codeLabel, i % 5, i / 5);
    }
}

void RecoveryCodesPanel::clearCodes()
{
    clearLayout(codeGrid);
    codes.clear();
    savedBox->setChecked(false);
    copiedLabel->clear();
}

bool RecoveryCodesPanel::isSavedTicked() const
{
    return savedBox->isChecked();
}

QStringList RecoveryCodesPanel::shownCodes() const
{
    return codes;
}

void RecoveryCodesPanel::copyCodes()
{
    QString text = "Finora recovery codes (each works once)\n";
    for (int i = 0; i < codes.size(); i++) {
        text += QString::number(i + 1) + ". " + codes[i] + "\n";
    }
    QGuiApplication::clipboard()->setText(text);
    copiedLabel->setText("Copied. Paste them into a safe place, then clear your clipboard.");
}
