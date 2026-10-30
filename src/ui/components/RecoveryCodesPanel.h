// =====================================================================
//  RecoveryCodesPanel.h
//  Shows new recovery codes once: the codes, a Copy button, and a box
//  to tick when they have been saved.
// =====================================================================
#ifndef FINORA_RECOVERYCODESPANEL_H
#define FINORA_RECOVERYCODESPANEL_H

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

#include <string>
#include <vector>

class RecoveryCodesPanel : public QWidget
{
    Q_OBJECT

public:
    RecoveryCodesPanel(QWidget *parent = nullptr);

    // Shows the codes. They exist only in this window: Finora stores
    // their hashes, so it can never show them again.
    void showCodes(const std::vector<std::string> &codes);

    // Removes the codes from the window again.
    void clearCodes();

    bool isSavedTicked() const;
    QStringList shownCodes() const;   // for the tests

signals:
    void savedChanged(bool saved);

private slots:
    void copyCodes();

private:
    QGridLayout *codeGrid = nullptr;
    QStringList codes;
    QPushButton *copyButton = nullptr;
    QCheckBox *savedBox = nullptr;
    QLabel *copiedLabel = nullptr;
};

#endif // FINORA_RECOVERYCODESPANEL_H
