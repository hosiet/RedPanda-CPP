/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef PROJECTOUTPUTWIDGET_H
#define PROJECTOUTPUTWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectOutputWidget;
}

class ProjectOutputWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectOutputWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectOutputWidget();

private:
    Ui::ProjectOutputWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnOutputDir_triggered(QAction *arg1);
    void on_btnObjOutputDir_triggered(QAction *arg1);
    void on_btnCompileLog_triggered(QAction *arg1);

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // PROJECTOUTPUTWIDGET_H
