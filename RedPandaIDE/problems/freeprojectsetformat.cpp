#include "freeprojectsetformat.h"

#include <QFile>
#include <QXmlStreamReader>
#

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
    QString currentText;
    QString currentEleName;
    while(!xml.atEnd()) {
        xml.readNext();
        switch (xml.tokenType()) {
        case QXmlStreamReader::TokenType::StartElement:
            currentEleName = xml.name().toString();
            if (xml.name()=="item") {
                currentProblem=std::make_shared<OJProblem>();
            } else if (currentProblem &&
                       ( xml.name()=="sample_input"
                         || xml.name()=="test_input")) {
                currentCase = std::make_shared<OJProblemCase>();
                foreach (const QXmlStreamAttribute& attr, xml.attributes()) {
                    if (attr.name() == "name") {
                        currentCase->name = attr.value().toString();
                        break;
                    }
                }
                currentCase->name = QObject::tr("Problem Case %1").arg(currentProblem->cases.count()+1);
            } else if (currentProblem &&
                       xml.name()=="time_limit") {
                foreach (const QXmlStreamAttribute& attr, xml.attributes()) {
                    if (attr.name() == "unit" && attr.value()=="ms") {
                        currentEleName = xml.name().toString();
                        break;
                    }
                }
            } else if (currentProblem &&
                       xml.name()=="memory_limit") {
                foreach (const QXmlStreamAttribute& attr, xml.attributes()) {
                    if (attr.name() == "unit" && attr.value()=="mb") {
                        currentEleName = xml.name().toString();
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
            if (currentCase &&
                    (
                        currentEleName=="test_input" ||
                        currentEleName=="sample_input")) {
                    currentCase->input = xml.text().toString();
            } else if (currentCase && currentProblem &&
                       (
                           currentEleName=="test_output" ||
                           currentEleName=="sample_output")) {
                currentCase->expected = xml.text().toString();
                currentProblem->cases.append(currentCase);
                currentCase.reset();
            } else if (currentProblem &&  currentEleName=="description") {
                currentProblem->description = xml.text().toString();
            } else if (currentProblem &&  currentEleName=="hint") {
                currentProblem->hint = xml.text().toString();
            } else if (currentProblem &&  currentEleName=="title") {
                currentProblem->name = xml.text().toString();
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
