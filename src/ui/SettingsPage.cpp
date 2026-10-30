// =====================================================================
//  SettingsPage.cpp
// =====================================================================
#include "SettingsPage.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "PageHeader.h"
#include "Theme.h"

const int THEME_BUTTON_LIGHT = 0;
const int THEME_BUTTON_DARK = 1;

SettingsPage::SettingsPage(DatabaseManager &sharedDatabase, QWidget *parent)
    : Page(parent), database(sharedDatabase)
{
    setObjectName("settingsPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(new PageHeader("Settings", "Appearance, your data and information about Finora",
                                            content));
    contentLayout->addWidget(createAppearanceCard());
    contentLayout->addWidget(createDataCard());
    contentLayout->addWidget(createAboutCard());
    contentLayout->addStretch();

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName("pageScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    refresh();
}

// A card with a title, a grey line under it and an empty body.
QFrame *SettingsPage::createSection(const QString &title, const QString &subtitle, QVBoxLayout *&bodyLayout)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");
    card->setMaximumWidth(760);

    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("cardTitle");
    QLabel *subtitleLabel = new QLabel(subtitle, card);
    subtitleLabel->setObjectName("cardSubtitle");
    subtitleLabel->setWordWrap(true);

    bodyLayout = new QVBoxLayout();
    bodyLayout->setSpacing(12);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 18, 22, 20);
    layout->setSpacing(6);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(8);
    layout->addLayout(bodyLayout);
    return card;
}

QFrame *SettingsPage::createAppearanceCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("Appearance", "Choose how Finora looks. Your choice is saved for next time.", body);

    // Light | Dark as a segmented control.
    QFrame *themeSwitch = new QFrame(card);
    themeSwitch->setObjectName("segmented");
    QHBoxLayout *switchLayout = new QHBoxLayout(themeSwitch);
    switchLayout->setContentsMargins(3, 3, 3, 3);
    switchLayout->setSpacing(2);

    themeGroup = new QButtonGroup(card);
    QPushButton *lightButton = new QPushButton("Light", themeSwitch);
    QPushButton *darkButton = new QPushButton("Dark", themeSwitch);
    QPushButton *themeButtons[2] = {lightButton, darkButton};
    for (int i = 0; i < 2; i++) {
        themeButtons[i]->setObjectName("segmentButton");
        themeButtons[i]->setCheckable(true);
        themeButtons[i]->setCursor(Qt::PointingHandCursor);
        themeButtons[i]->setMinimumWidth(90);
        themeGroup->addButton(themeButtons[i], i);
        switchLayout->addWidget(themeButtons[i]);
    }
    lightButton->setAccessibleName("Light theme");
    darkButton->setAccessibleName("Dark theme");
    connect(themeGroup, &QButtonGroup::idClicked, this, &SettingsPage::onThemeButtonClicked);

    QLabel *themeLabel = new QLabel("Theme", card);
    themeLabel->setObjectName("bodyText");
    QHBoxLayout *themeRow = new QHBoxLayout();
    themeRow->addWidget(themeLabel);
    themeRow->addStretch();
    themeRow->addWidget(themeSwitch);

    reducedMotionBox = new QCheckBox("Reduce motion", card);
    reducedMotionBox->setObjectName("reducedMotionBox");
    QLabel *motionHelp = new QLabel("Shows changes at once, without fading messages or growing charts.", card);
    motionHelp->setObjectName("mutedText");
    motionHelp->setWordWrap(true);
    connect(reducedMotionBox, &QCheckBox::toggled, this, &SettingsPage::onReducedMotionToggled);

    body->addLayout(themeRow);
    body->addWidget(reducedMotionBox);
    body->addWidget(motionHelp);
    return card;
}

QFrame *SettingsPage::createDataCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("Your data",
                                 "Finora keeps everything on this computer, in one SQLite database file. "
                                 "Before an upgrade changes the file, a backup copy is saved in its "
                                 "\"backups\" folder.", body);

    locationLabel = new QLabel(card);
    locationLabel->setObjectName("bodyText");
    locationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);   // the path can be copied
    locationLabel->setWordWrap(true);

    QPushButton *openButton = new QPushButton("Open data folder", card);
    openButton->setObjectName("secondaryButton");
    openButton->setCursor(Qt::PointingHandCursor);
    connect(openButton, &QPushButton::clicked, this, &SettingsPage::onOpenFolderClicked);

    body->addWidget(locationLabel);
    body->addWidget(openButton, 0, Qt::AlignLeft);
    return card;
}

QFrame *SettingsPage::createAboutCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("About Finora", "Version " + QCoreApplication::applicationVersion()
                                 + ". A personal finance program, made as a university project.", body);

    QLabel *credits = new QLabel("Built with Qt 6 and SQLite. Fonts: Inter and Manrope, both under the SIL "
                                 "Open Font License 1.1.", card);
    credits->setObjectName("bodyText");
    credits->setWordWrap(true);

    QPushButton *licencesButton = new QPushButton("View font licences", card);
    licencesButton->setObjectName("secondaryButton");
    licencesButton->setCursor(Qt::PointingHandCursor);
    connect(licencesButton, &QPushButton::clicked, this, &SettingsPage::onLicencesClicked);

    body->addWidget(credits);
    body->addWidget(licencesButton, 0, Qt::AlignLeft);
    return card;
}

void SettingsPage::refresh()
{
    // blockSignals: showing the saved state must not count as a new choice.
    themeGroup->blockSignals(true);
    if (currentThemeMode() == THEME_DARK) {
        themeGroup->button(THEME_BUTTON_DARK)->setChecked(true);
    } else {
        themeGroup->button(THEME_BUTTON_LIGHT)->setChecked(true);
    }
    themeGroup->blockSignals(false);

    reducedMotionBox->blockSignals(true);
    reducedMotionBox->setChecked(isReducedMotion());
    reducedMotionBox->blockSignals(false);

    locationLabel->setText("Database file:  " + QDir::toNativeSeparators(database.databasePath()));
}

void SettingsPage::onThemeButtonClicked(int id)
{
    emit darkThemeChosen(id == THEME_BUTTON_DARK);
}

void SettingsPage::onReducedMotionToggled(bool checked)
{
    emit reducedMotionChosen(checked);
}

void SettingsPage::onOpenFolderClicked()
{
    QString folder = QFileInfo(database.databasePath()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));   // opens it in File Explorer
}

// Shows the full licence text of both fonts, read from the files that
// are built into Finora.exe next to the fonts themselves.
void SettingsPage::onLicencesClicked()
{
    QString text;
    const int FILE_COUNT = 2;
    const char *LICENCE_FILES[FILE_COUNT] = {":/fonts/Inter-OFL.txt", ":/fonts/Manrope-OFL.txt"};
    const char *FONT_NAMES[FILE_COUNT] = {"Inter", "Manrope"};
    for (int i = 0; i < FILE_COUNT; i++) {
        QFile file(LICENCE_FILES[i]);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            text += "==== " + QString(FONT_NAMES[i]) + " ====\n\n" + QString::fromUtf8(file.readAll()) + "\n\n";
            file.close();
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Font licences");
    dialog.resize(640, 520);

    QLabel *heading = new QLabel("Font licences", &dialog);
    heading->setObjectName("dialogTitle");
    QPlainTextEdit *licenceText = new QPlainTextEdit(text, &dialog);
    licenceText->setObjectName("licenceText");
    licenceText->setReadOnly(true);
    QPushButton *closeButton = new QPushButton("Close", &dialog);
    closeButton->setObjectName("primaryButton");
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(licenceText, 1);
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    dialog.exec();
}
