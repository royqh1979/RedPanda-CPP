#include "projecttemplate.h"
#include "SimpleIni.h"

#include <QFile>
ProjectTemplate::ProjectTemplate(QObject *parent) : QObject(parent)
{

}

int ProjectTemplate::unitCount()
{
    if (mFileName.isEmpty() || !QFile(mFileName).exists())
        return -1;
    SimpleIni ini;
    ini.LoadFile(toByteArray(mFileName));
    int ver = ini.GetLongValue("Template","Ver",0);
    if (ver<=0)
        return 0;
    return ini.GetLongValue("Project","UnitCount",0);
}

PTemplateUnit ProjectTemplate::unit(int index)
{
    if (mFileName.isEmpty() || !QFile(mFileName).exists())
        return PTemplateUnit();
    SimpleIni ini;
    ini.LoadFile(toByteArray(mFileName));
    int ver = ini.GetLongValue("Template","Ver",0);
    if (ver<=0)
        return PTemplateUnit();
    QString section = QString("Unit%1").arg(index);
    PTemplateUnit unit = std::make_shared<TemplateUnit>();
    unit->CText = fromByteArray(ini.GetValue(toByteArray(section), "C", ""));
    unit->CppText = fromByteArray(ini.GetValue(toByteArray(section), "Cpp", ""));
    if (unit->CppText.isEmpty())
        unit->CppText = unit->CText;

    unit->CName = fromByteArray(ini.GetValue(toByteArray(section), "CName", ""));
    unit->CppName = fromByteArray(ini.GetValue(toByteArray(section), "CppName", ""));
    if (unit->CppName.isEmpty())
        unit->CppName = unit->CName;
}

void ProjectTemplate::setUnit(int index, PTemplateUnit newUnit)
{
    if (!newUnit)
        return;
    if (mFileName.isEmpty() || !QFile(mFileName).exists())
        return;
    SimpleIni ini;
    ini.LoadFile(toByteArray(mFileName));
    int ver = ini.GetLongValue("Template","Ver",0);
    if (ver<=0)
        return;
    QByteArray section = toByteArray(QString("Unit%1").arg(index));
    ini.SetValue(section,"C", toByteArray(newUnit->CText));
    ini.SetValue(section,"Cpp", toByteArray(newUnit->CppText));
    ini.SetValue(section,"CName", toByteArray(newUnit->CName));
    ini.SetValue(section,"CppName", toByteArray(newUnit->CppName));
    ini.SaveFile(toByteArray(mFileName));
}
