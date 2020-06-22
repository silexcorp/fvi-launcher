// Pegasus Frontend
// Copyright (C) 2017-2019  Mátyás Mustoha
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.


#include <QtTest/QtTest>

#include "model/gaming/Collection.h"
#include "model/gaming/Game.h"
#include "providers/pegasus_favorites/Favorites.h"
#include "utils/HashMap.h"


class test_FavoriteDB : public QObject {
    Q_OBJECT

private slots:
    void write();
    void rewrite_empty();
    void read();

private:
    void create_dummy_data(QVector<model::Collection*>& out_collections, QVector<model::Game*>& out_games)
    {
        out_collections = {
            new model::Collection("coll1", this),
            new model::Collection("coll2", this),
        };
        out_games = {
            new model::Game(QFileInfo(":/a/b/coll1dummy1"), this),
            new model::Game(QFileInfo(":/coll1dummy2"), this),
            new model::Game(QFileInfo(":/x/y/z/coll2dummy1"), this),
        };
        (*out_collections.at(0))
            .addGame(out_games.at(0))
            .addGame(out_games.at(1))
            .finalize();
        (*out_collections.at(1))
            .addGame(out_games.at(2))
            .finalize();
        (*out_games.at(0))
            .addCollection(out_collections.at(0))
            .finalize();
        (*out_games.at(1))
            .addCollection(out_collections.at(0))
            .finalize();
        (*out_games.at(2))
            .addCollection(out_collections.at(1))
            .finalize();
    }
};


void test_FavoriteDB::write()
{
    QVector<model::Collection*> collections;
    QVector<model::Game*> games;
    create_dummy_data(collections, games);

    games.at(1)->setFavorite(true);
    games.at(2)->setFavorite(true);

    QTemporaryFile tmp_file;
    tmp_file.setAutoRemove(false);
    QVERIFY(tmp_file.open());

    const QString db_path = tmp_file.fileName();
    tmp_file.close();


    providers::favorites::Favorites favorite_db;
    favorite_db.load_with_dbpath(db_path);

    QSignalSpy spy_start(&favorite_db, &providers::favorites::Favorites::startedWriting);
    QSignalSpy spy_end(&favorite_db, &providers::favorites::Favorites::finishedWriting);
    QVERIFY(spy_start.isValid());
    QVERIFY(spy_end.isValid());

    favorite_db.onGameFavoriteChanged(games);

    QVERIFY(spy_start.count() || spy_start.wait());
    QVERIFY(spy_end.count() || spy_end.wait());
    QCOMPARE(spy_start.count(), 1);
    QCOMPARE(spy_end.count(), 1);


    QFile db_file(db_path);
    QVERIFY(db_file.open(QFile::ReadOnly | QFile::Text));

    QTextStream db_stream(&db_file);
    QStringList found_items;
    QString line;
    while (db_stream.readLineInto(&line)) {
        if (!line.startsWith('#'))
            found_items << line;
    }

    QCOMPARE(found_items.count(), 2);
    QVERIFY(found_items.contains(":/coll1dummy2"));
    QVERIFY(found_items.contains(":/x/y/z/coll2dummy1"));

    QFile::remove(db_path);
}

void test_FavoriteDB::rewrite_empty()
{
    QVector<model::Collection*> collections;
    QVector<model::Game*> games;
    create_dummy_data(collections, games);

    QTemporaryFile tmp_file;
    tmp_file.setAutoRemove(false);
    QVERIFY(tmp_file.open());

    const QString db_path = tmp_file.fileName();
    tmp_file.close();


    providers::favorites::Favorites favorite_db;
    favorite_db.load_with_dbpath(db_path);
    QSignalSpy spy_end(&favorite_db, &providers::favorites::Favorites::finishedWriting);
    QVERIFY(spy_end.isValid());

    games.at(1)->setFavorite(true);
    favorite_db.onGameFavoriteChanged(games);

    games.at(1)->setFavorite(false);
    favorite_db.onGameFavoriteChanged(games);

    QVERIFY(spy_end.count() == 2 || spy_end.wait());


    QFile db_file(db_path);
    QVERIFY(db_file.open(QFile::ReadOnly | QFile::Text));

    QTextStream db_stream(&db_file);
    QStringList found_items;
    QString line;
    while (db_stream.readLineInto(&line)) {
        if (!line.startsWith('#'))
            found_items << line;
    }

    QCOMPARE(found_items.count(), 0);
    QFile::remove(db_path);
}

void test_FavoriteDB::read()
{
    QVector<model::Collection*> collections;
    QVector<model::Game*> games;
    create_dummy_data(collections, games);

    QTemporaryFile tmp_file;
    tmp_file.setAutoRemove(false);
    QVERIFY(tmp_file.open());
    {
        QTextStream tmp_stream(&tmp_file);
        tmp_stream << QStringLiteral("# Favorite reader test") << endl;
        tmp_stream << games[2]->filesConst().first()->fileinfo().canonicalFilePath() << endl;
        tmp_stream << games[1]->filesConst().first()->fileinfo().canonicalFilePath() << endl;
        tmp_stream << QStringLiteral(":/somethingfake") << endl;
    }
    const QString db_path = tmp_file.fileName();
    tmp_file.close();

    providers::favorites::Favorites favorite_db;
    favorite_db.load_with_dbpath(db_path);

    HashMap<QString, model::GameFile*> path_map;
    for (const model::Game* const game : games) {
        model::GameFile* const gamefile = game->filesConst().first();
        QString path = gamefile->fileinfo().canonicalFilePath();
        QVERIFY(!path.isEmpty());
        path_map.emplace(std::move(path), gamefile);
    }

    favorite_db.findDynamicData({}, games, path_map);

    QVERIFY(!games[0]->isFavorite());
    QVERIFY(games[1]->isFavorite());
    QVERIFY(games[2]->isFavorite());

    QFile::remove(db_path);
}


QTEST_MAIN(test_FavoriteDB)
#include "test_FavoriteDB.moc"