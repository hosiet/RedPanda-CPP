#ifndef PROJECTGENERALWIDGET_H
#define PROJECTGENERALWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectGeneralWidget;
}

class ProjectGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectGeneralWidget();

private:
    Ui::ProjectGeneralWidget *ui;
    QString mIconPath;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnBrowse_clicked();
    void on_btnRemove_clicked();
};

#endif // PROJECTGENERALWIDGET_H