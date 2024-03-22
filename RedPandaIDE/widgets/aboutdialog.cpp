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
#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "../systemconsts.h"
#include "../utils.h"
#include "../settings.h"
#include <QDebug>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog{parent},
    ui{new Ui::AboutDialog}
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    ui->lblTitle->setText(ui->lblTitle->text() + tr("Version: ") + REDPANDA_CPP_VERSION);

#if defined(__clang__) // Clang always pretends to be GCC/MSVC. Check it first.
# if defined(_MSC_VER)
    QString templ = "Clang %1.%2.%3 %4 MSVC ABI";
# elif defined(__apple_build_version__)
    QString templ = "Apple Clang %1.%2.%3 %4";
# else
    QString templ = "Clang %1.%2.%3 %4";
# endif
    ui->lblQt->setText(ui->lblQt->text()
                       .arg(qVersion())
                       .arg(templ
                            .arg(__clang_major__)
                            .arg(__clang_minor__)
                            .arg(__clang_patchlevel__)
                            .arg(appArch()))
                       .arg(osArch()));
#elif defined(__GNUC__)
    ui->lblQt->setText(ui->lblQt->text()
            .arg(qVersion())
            .arg(QString("GCC %1.%2.%3 %4")
                 .arg(__GNUC__)
                 .arg(__GNUC_MINOR__)
                 .arg(__GNUC_PATCHLEVEL__)
                 .arg(appArch()))
            .arg(osArch()));
#elif defined(_MSC_VER)
# if (_MSC_VER >= 1940)
    QString name = tr("Next Generation Microsoft Visual C++");
# elif (_MSC_VER >= 1930)
    QString name = tr("Microsoft Visual C++ 2022");
# elif (_MSC_VER >= 1920)
    QString name = tr("Microsoft Visual C++ 2019");
# elif (_MSC_VER >= 1910)
    QString name = tr("Microsoft Visual C++ 2017");
# else
    QString name = tr("Legacy Microsoft Visual C++");
# endif
    ui->lblQt->setText(ui->lblQt->text()
            .arg(qVersion())
            .arg(name + " " + appArch())
            .arg(osArch()));
#else
    ui->lblQt->setText(ui->lblQt->text()
            .arg(qVersion())
            .arg(tr("Non-GCC Compiler"))
            .arg(osArch()));
#endif
    ui->lblCompileTime->setText(ui->lblCompileTime->text()
                                .arg(__DATE__, __TIME__));

    QString website="https://sourceforge.net/projects/redpanda-cpp/";
    if (pSettings->environment().language()=="zh_CN") {
        website = "https://royqh1979.gitee.io/redpandacpp";
    }
    ui->lblHomepage->setText(tr("Website: <a href=\"%1\">%1</a>").arg(website));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
