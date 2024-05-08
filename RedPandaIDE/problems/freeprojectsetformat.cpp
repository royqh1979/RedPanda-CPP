#include "freeprojectsetformat.h"
#include "../utils.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QList<POJProblem> importFreeProblemSet(const QString &filename)
{
    QFile file(filename);
    QList<POJProblem> problems;
    if (!file.open(QFile::ReadOnly)) {
        throw FileError(QObject::tr("Can't open file \"%1\" for read.").arg(filename));
    }
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
    if (!file.open(QFile::WriteOnly|QFile::Truncate)) {
        throw FileError(QObject::tr("Can't open file \"%1\" for write.").arg(filename));
    }
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    //fps
    {
        writer.writeStartElement("fps");
        writer.writeAttribute("version","1.4");
        writer.writeAttribute("url","https://github.com/zhblue/freeproblemset/");
       {
            writer.writeStartElement("generator");
            writer.writeAttribute("name","RedPanda-C++");
            writer.writeAttribute("url","http://royqh.net/redpandacpp/");
            writer.writeEndElement(); // generator
        }
        foreach(const POJProblem& problem,problems) {
            writer.writeStartElement("item");
            {
                writer.writeStartElement("title");
                writer.writeCDATA(problem->name);
                writer.writeEndElement(); //title
            }
            {
                writer.writeStartElement("url");
                writer.writeCDATA(problem->url);
                writer.writeEndElement();//url
            }
            {
                QString unit;
                switch(problem->timeLimitUnit) {
                case ProblemTimeLimitUnit::Milliseconds:
                    unit = "ms";
                    break;
                case ProblemTimeLimitUnit::Seconds:
                    unit = "s";
                    break;
                }
                writer.writeStartElement("time_limit");
                writer.writeAttribute("unit",unit);
                writer.writeCDATA(QString("%1").arg(problem->timeLimit));
                writer.writeEndElement(); //time_limit
            }
            {
                QString unit;
                switch(problem->memoryLimitUnit) {
                case ProblemMemoryLimitUnit::MB:
                    unit = "mb";
                    break;
                case ProblemMemoryLimitUnit::KB:
                    unit = "kb";
                    break;
                case ProblemMemoryLimitUnit::GB:
                    unit = "gb";
                    break;
                }
                writer.writeStartElement("memory_limit");
                writer.writeAttribute("unit",unit);
                writer.writeCDATA(QString("%1").arg(problem->memoryLimit));
                writer.writeEndElement(); //memory_limit
            }
            {
                writer.writeStartElement("description");
                writer.writeCDATA(problem->description);
                writer.writeEndElement(); //description
            }
            foreach(const POJProblemCase& pCase, problem->cases) {
                writer.writeStartElement("test_input");
                writer.writeAttribute("name",pCase->name);
                writer.writeCDATA(pCase->input);
                writer.writeEndElement(); //test_input
                writer.writeStartElement("test_output");
                writer.writeCDATA(pCase->expected);
                writer.writeEndElement(); //test_output
            }
            {
                writer.writeStartElement("hint");
                writer.writeCDATA(problem->hint);
                writer.writeEndElement(); //hint
            }
            writer.writeEndElement(); //item
        }
        writer.writeEndElement(); //fps
    }
    writer.writeEndDocument();
    if (writer.hasError()) {
        throw FileError(QObject::tr("Error when writing file \"%1\".").arg(filename));
    }
}
