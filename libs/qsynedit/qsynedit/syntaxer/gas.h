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
#ifndef QSYNEDIT_GAS_SYNTAXER_H
#define QSYNEDIT_GAS_SYNTAXER_H

#include "asm.h"
#include "syntaxer.h"

namespace QSynedit {

class GASSyntaxer : public ASMSyntaxer
{
public:
    enum class SyntaxMode {
        ATT,
        Intel
    };
    struct GASSyntaxState : SyntaxState {
        SyntaxMode syntaxMode;
        bool prefixRegisterNames;
        bool equals(const std::shared_ptr<SyntaxState> &s2) const override;
    };
    explicit GASSyntaxer();
    GASSyntaxer(const GASSyntaxer&)=delete;
    GASSyntaxer& operator=(const GASSyntaxer&)=delete;

    static const QSet<QString> Directives;
private:
    SyntaxMode mSyntaxMode;
    bool mPrefixRegisterNames;
    bool mThisLineHasSyntaxDirective;
    static QSet<QString> mNonprefixedKeywords;
    static QSet<QString> mPrefixedKeywords;
protected:
    bool isCommentStartChar(QChar ch) override;
    bool isRegister(const QString &ident) const override;
    bool isDirective(const QString& ident) override;
    void handleDirective(int line, const QString& directive) override;
    void handleIdent(int line, const QString& ident)  override;
    void setPrefixRegisterNames(bool prefix);
public:
    void setLine(int lineNumber, const QString &newLine, size_t lineSeq) override;
    PSyntaxState getState() const override;
    void setState(const PSyntaxState &rangeState) override;
    void resetState() override;
    QString languageName() const override;
    ProgrammingLanguage language() const override;
    QSet<QString> keywords() const override;
    bool prefixRegisterNames() const;
    SyntaxMode syntaxMode() const;
    void setSyntaxMode(SyntaxMode newSyntaxMode);
    virtual PSyntaxer createInstance() const override;
};

}

#endif // QSYNEDIT_GAS_SYNTAXER_H
