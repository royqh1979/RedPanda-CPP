#include "freeprojectsetformat.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QList<POJProblem> importFreeProblemSet(const QString &filename)
{
    QFile file(filename);
    QList<POJProblem> problems;
    if (!file.open(QFile::ReadOnly))
        return problems;
    QXmlStreamReader xml;
    xml.setDevice(&file);
    POJProblem currentProblem;
    POJProblemCase currentCase;
    QString currentEleName;
    while(!xml.atEnd()) {
        xml.readNext();
        switch (xml.tokenType()) {
        case QXmlStreamReader::TokenType::StartElement:
            currentEleName = xml.name().toString();
            if (xml.name()=="item") {
                currentProblem=std::make_shared<OJProblem>();
            } else if (currentProblem &&
                       (xml.name()=="test_input")) {
                currentCase = std::make_shared<OJProblemCase>();
                foreach (const QXmlStreamAttribute& attr, xml.attributes()) {
                    if (attr.name() == "name") {
                        currentCase->name = attr.value().toString().trimmed();
                        break;
                    }
                }
                currentCase->name = QObject::tr("Problem Case %1").arg(currentProblem->cases.count()+1);
            } else if (currentProblem &&
                       xml.name()=="time_limit") {
                currentEleName = xml.name().toString();
                foreach (const QXmlStreamAttribute& attr, xml.attributes()) {
                    if (attr.name() == "unit") {
                        if (attr.value()=="ms")
                            currentProblem->timeLimitUnit = ProblemTimeLimitUnit::Milliseconds;
                        else if (attr.value()=="s")
                            currentProblem->timeLimitUnit = ProblemTimeLimitUnit::Seconds;
                        break;
                    }
                }
            } else if (currentProblem &&
                       xml.name()=="memory_limit") {
                currentEleName = xml.name().toString();
                foreach (const QXmlStreamAttribute& attr, xml.attributes()) {
                    if (attr.name() == "unit") {
                        if (attr.value()=="mb")
                            currentProblem->memoryLimitUnit = ProblemMemoryLimitUnit::MB;
                        else if (attr.value()=="kb")
                            currentProblem->memoryLimitUnit = ProblemMemoryLimitUnit::KB;
                        else if (attr.value()=="gb")
                            currentProblem->memoryLimitUnit = ProblemMemoryLimitUnit::GB;
                        break;
                    }
                }
            }
            break;
        case QXmlStreamReader::TokenType::EndElement:
            currentEleName.clear();
            if (currentProblem && xml.name()=="item") {
                problems.append(currentProblem);
                currentProblem.reset();
            }
            break;
        case QXmlStreamReader::TokenType::Characters:
            if (currentCase && currentProblem && currentEleName=="test_input") {
                    currentCase->input = xml.text().toString();
            } else if (currentCase && currentProblem && currentEleName=="test_output" ) {
                currentCase->expected = xml.text().toString();
                currentProblem->cases.append(currentCase);
                currentCase.reset();
            } else if (currentProblem &&  currentEleName=="description") {
                currentProblem->description = xml.text().toString();
            } else if (currentProblem &&  currentEleName=="hint") {
                currentProblem->hint = xml.text().toString();
            } else if (currentProblem &&  currentEleName=="title") {
                currentProblem->name = xml.text().toString().trimmed().replace("&nbsp;"," ");
            } else if (currentProblem &&  currentEleName=="url") {
                currentProblem->url = xml.text().toString().trimmed();
            } else if (currentProblem &&  currentEleName=="time_limit") {
                currentProblem->timeLimit = xml.text().toInt();
            } else if (currentProblem &&  currentEleName=="memory_limit") {
                currentProblem->memoryLimit = xml.text().toInt();
            }
        default:
            break;
        }
    }
    return problems;
}

void exportFreeProblemSet(const QList<POJProblem> &problems, const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::WriteOnly|QFile::Truncate))
        return;
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeEndDocument();
}
