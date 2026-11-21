#include <QSignalSpy>
#include <QTest>
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
