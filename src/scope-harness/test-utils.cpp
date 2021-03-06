/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU Lesser General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include "test-utils.h"

#include <QObject>
#include <QTest>
#include <QFile>
#include <QFileInfo>
#include <QGSettings>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QScopedPointer>
#include <QSignalSpy>

#include <Unity/scopes.h>
#include <Unity/scope.h>
#include <Unity/categories.h>
#include <Unity/resultsmodel.h>

namespace sc = unity::scopes;
namespace ng = scopes_ng;
namespace ss = unity::shell::scopes;

namespace unity {
namespace scopeharness {

void TestUtils::throwIf(bool condition, const std::string& message)
{
    if (condition)
    {
        throw std::domain_error(message);
    }
}

void TestUtils::throwIfNot(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::domain_error(message);
    }
}

void TestUtils::checkedFirstResult(unity::shell::scopes::CategoriesInterface* categories, sc::Result::SPtr& result, bool& success)
{
    // ensure categories have > 0 rows
    QVERIFY(categories->rowCount() > 0);
    QVariant results_var = categories->data(categories->index(0), ng::Categories::Roles::RoleResults);
    QVERIFY(results_var.canConvert<ng::ResultsModel*>());

    // ensure results have some data
    auto results = results_var.value<ng::ResultsModel*>();
    QVERIFY(results);
    QVERIFY(results->rowCount() > 0);
    auto result_var = results->data(results->index(0), ng::ResultsModel::RoleResult);
    QCOMPARE(result_var.isNull(), false);
    result = result_var.value<std::shared_ptr<sc::Result>>();
    success = true;
}

bool TestUtils::getFirstResult(unity::shell::scopes::CategoriesInterface* categories, sc::Result::SPtr& result)
{
    bool success = false;
    checkedFirstResult(categories, result, success);
    return success;
}

void TestUtils::refreshSearch(ng::Scope::Ptr scope)
{
    QCOMPARE(scope->searchInProgress(), false);
    QSignalSpy spy(scope.data(), SIGNAL(searchInProgressChanged()));
    // refresh the search
    scope->refresh();
    QVERIFY(scope->searchInProgress() || spy.count() > 1);
    if (scope->searchInProgress()) {
        // wait for the search to finish
        QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    }
    QCOMPARE(scope->searchInProgress(), false);
}

void TestUtils::performSearch(QSharedPointer<ss::ScopeInterface> scope, QString const& searchString)
{
    QCOMPARE(scope->searchInProgress(), false);
    QSignalSpy spy(scope.data(), SIGNAL(searchInProgressChanged()));
    // perform a search
    scope->setSearchQuery(searchString);
    // search should not be happening yet
    QCOMPARE(scope->searchInProgress(), false);
    QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    if (scope->searchInProgress()) {
        // wait for the search to finish
        QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    }
    QCOMPARE(scope->searchInProgress(), false);
}

void TestUtils::waitForResultsChange(QSharedPointer<ss::ScopeInterface> scope)
{
    QCOMPARE(scope->searchInProgress(), false);
    // wait for the search to finish
    QSignalSpy spy(scope.data(), SIGNAL(searchInProgressChanged()));
    QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    if(spy.size() == 1) {
        QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    }
    QCOMPARE(scope->searchInProgress(), false);
}

void TestUtils::waitForSearchFinish(QSharedPointer<ss::ScopeInterface> scope)
{
    QCOMPARE(scope->searchInProgress(), true);
    QSignalSpy spy(scope.data(), SIGNAL(searchInProgressChanged()));
    QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    if (scope->searchInProgress()) {
        // wait for the search to finish
        QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    }
    QCOMPARE(scope->searchInProgress(), false);
}

void TestUtils::waitForFilterStateChange(QSharedPointer<ss::ScopeInterface> scope)
{
    QSignalSpy spy(scope->filters(), SIGNAL(filterStateChanged()));
    QVERIFY(spy.wait(SIG_SPY_TIMEOUT));
    QCOMPARE(spy.count(), 1);
}

void TestUtils::setFavouriteScopes(const QStringList& cannedQueries)
{
    setenv("GSETTINGS_BACKEND", "memory", 1);
    QGSettings settings("com.canonical.Unity.Dash", QByteArray(), nullptr);
    settings.set("favoriteScopes", QVariant(cannedQueries));
}

QStringList TestUtils::getFavoriteScopes()
{
    setenv("GSETTINGS_BACKEND", "memory", 1);
    QGSettings settings("com.canonical.Unity.Dash", QByteArray(), nullptr);
    QStringList favs;
    for (auto const favvar: settings.get("favoriteScopes").toList()) {
        favs.push_back(favvar.toString());
    }
    return favs;
}

}
}
