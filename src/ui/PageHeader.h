// =====================================================================
//  PageHeader.h
//  The title area at the top of every page.
// =====================================================================
#ifndef FINORA_PAGEHEADER_H
#define FINORA_PAGEHEADER_H

#include <QLabel>
#include <QWidget>

// A large bold title with a smaller grey subtitle underneath.
// Every page reuses this class, so all pages look the same.
class PageHeader : public QWidget
{
public:
    PageHeader(const QString &title, const QString &subtitle, QWidget *parent = nullptr);

private:
    QLabel *titleLabel = nullptr;
    QLabel *subtitleLabel = nullptr;
};

#endif // FINORA_PAGEHEADER_H
