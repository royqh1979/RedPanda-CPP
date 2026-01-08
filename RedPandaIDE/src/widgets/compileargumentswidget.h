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
#ifndef COMPILEARGUMENTSWIDGET_H
#define COMPILEARGUMENTSWIDGET_H

#include <QMap>
#include <QTabWidget>
#include "../settings.h"

namespace Ui {
class CompileArgumentsWidget;
}

class CompileArgumentsWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit CompileArgumentsWidget(QWidget *parent = nullptr);
    ~CompileArgumentsWidget();

    QMap<QString, QString> arguments(bool includeUnset) const;

    void resetUI(PCompilerSet pSet, const QMap<QString,QString>& options);

    void setBoolArgument(const QString &argKey, bool checked);

private:
    CompilerType mCompilerType;
};

#endif // COMPILEARGUMENTSWIDGET_H
