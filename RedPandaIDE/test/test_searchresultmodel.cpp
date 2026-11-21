#include <QSignalSpy>
#include <QTest>
#include <QImage>
#include <QPainter>
#include "test_searchresultmodel.h"
#include "src/widgets/searchresultview.h"

void TestSearchResultModel::test_file_checkbox_synchronizes_children()
{
    SearchResultModel model;
    PSearchResults results = model.addSearchResults(
                "a", QSynedit::ssoNone, false, SearchFileScope::currentFile);
    PSearchResultTreeItem file = std::make_shared<SearchResultTreeItem>();
    file->filename = "test.cpp";
    for (int i = 0; i < 2; ++i) {
        PSearchResultTreeItem item = std::make_shared<SearchResultTreeItem>();
        item->parent = file.get();
        item->matchedText = "a";
        file->results.append(item);
    }
    model.addResultToSearchResults(results, file);
    SearchResultTreeModel *tree = model.treeModel();
    tree->setSelectable(true);

    QModelIndex root = tree->index(0, 0, {});
    QModelIndex first = tree->index(0, 0, root);
    QModelIndex second = tree->index(1, 0, root);
    QCOMPARE(tree->parent(first), root);
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::Checked));

    QSignalSpy changeSpy(tree, &QAbstractItemModel::dataChanged);
    QVERIFY(tree->setData(root, Qt::Unchecked, Qt::CheckStateRole));
    QVERIFY(!file->results.at(0)->selected);
    QVERIFY(!file->results.at(1)->selected);
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::Unchecked));
    QVERIFY(changeSpy.count() >= 2);

    QVERIFY(tree->setData(root, Qt::Checked, Qt::CheckStateRole));
    QVERIFY(file->results.at(0)->selected);
    QVERIFY(file->results.at(1)->selected);
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::Checked));

    QVERIFY(tree->setData(first, Qt::Unchecked, Qt::CheckStateRole));
    QVERIFY(!file->results.at(0)->selected);
    QVERIFY(file->results.at(1)->selected);
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::PartiallyChecked));

    QVERIFY(tree->setData(second, Qt::Unchecked, Qt::CheckStateRole));
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::Unchecked));
    QVERIFY(tree->setData(first, Qt::Checked, Qt::CheckStateRole));
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::PartiallyChecked));

    QVERIFY(tree->setData(root, Qt::Checked, Qt::CheckStateRole));
    QVERIFY(file->results.at(0)->selected);
    QVERIFY(file->results.at(1)->selected);
    QCOMPARE(root.data(Qt::CheckStateRole).toInt(), static_cast<int>(Qt::Checked));
}

void TestSearchResultModel::test_selected_match_uses_highlight_palette()
{
    SearchResultModel model;
    PSearchResults results = model.addSearchResults(
                "value", QSynedit::ssoNone, false, SearchFileScope::currentFile);
    PSearchResultTreeItem file = std::make_shared<SearchResultTreeItem>();
    file->filename = "test.cpp";
    file->parent = nullptr;
    PSearchResultTreeItem item = std::make_shared<SearchResultTreeItem>();
    item->parent = file.get();
    item->line = 0;
    item->start = 4;
    item->len = 5;
    item->text = "int value;";
    file->results.append(item);
    model.addResultToSearchResults(results, file);
    SearchResultTreeModel *tree = model.treeModel();
    QModelIndex root = tree->index(0, 0, {});
    QModelIndex match = tree->index(0, 0, root);

    QImage image(500, 32, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, image.width(), image.height());
    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    option.palette.setColor(QPalette::Highlight, QColor(10, 110, 200));
    option.palette.setColor(QPalette::HighlightedText, Qt::white);
    QPainter painter(&image);
    SearchResultTreeViewDelegate delegate(tree);
    delegate.paint(&painter, option, match);
    painter.end();

    int backgroundPixels = 0;
    int foregroundPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor color = image.pixelColor(x, y);
            if (color == option.palette.color(QPalette::Highlight))
                ++backgroundPixels;
            if (color == option.palette.color(QPalette::HighlightedText))
                ++foregroundPixels;
        }
    }
    QVERIFY(backgroundPixels > 0);
    QVERIFY(foregroundPixels > 0);
}
