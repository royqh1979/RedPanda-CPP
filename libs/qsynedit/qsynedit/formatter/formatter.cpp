#include "formatter.h"
namespace QSynedit {
    Formatter::Formatter()
    {
    }

    const QStringList &Formatter::optionNames() const
    {
        return mOptionNames;
    }

    void Formatter::setOption(const QString &name, QVariant val)
    {
        Q_ASSERT(mOptions.contains("name"));
        mOptions.insert(name,val);
    }

    QVariant Formatter::getOption(const QString &name, const QVariant& defaultValue) const
    {
        return mOptions.value(name,defaultValue);
    }

    bool Formatter::getBoolOption(const QString &name, bool defaultValue) const
    {
        QVariant val = getOption(name);
        if (val.isValid())
            return val.toBool();
        else
            return defaultValue;
    }

    QString Formatter::getStringOption(const QString &name, const QString &defaultValue) const
    {
        return getOption(name,defaultValue).toString();
    }

    int Formatter::getIntOption(const QString &name, int defaultValue) const
    {
        bool ok;
        int val = getOption(name,defaultValue).toInt(&ok);
        if (!ok)
            return defaultValue;
        else
            return val;
    }

    void Formatter::initOptions()
    {
        doInitOptions();
        mOptionNames.clear();
        foreach(const QString& s, mOptions.keys())
            mOptionNames.append(s);
    }
}
