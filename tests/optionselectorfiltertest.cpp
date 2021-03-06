/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Pawel Stolowski <pawel.stolowski@canonical.com>
 */

#include <QSignalSpy>
#include <QScopedPointer>
#include <QTest>
#include <QList>
#include "filters.h"
#include "optionselectorfilter.h"

#include <unity/scopes/OptionSelectorFilter.h>

using namespace scopes_ng;
namespace uss = unity::shell::scopes;

class OptionSelectorFilterTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void init()
    {
        unity::scopes::FilterState filterState;
        filtersModel.reset(new Filters(filterState));

        f1 = unity::scopes::OptionSelectorFilter::create("f1", "Filter1", false);
        f1o1 = f1->add_option("f1o1", "Option1");
        f1o2 = f1->add_option("f1o2", "Option2");

        f2 = unity::scopes::OptionSelectorFilter::create("f2", "Filter2", true);
        f2o1 = f2->add_option("f2o1", "Option1");
        f2o2 = f2->add_option("f2o2", "Option2");

        backendFilters.clear();
        backendFilters.append(f1);
        backendFilters.append(f2);
        filtersModel->update(backendFilters);

        f1->update_state(filterState, f1o1, true);

        filtersModel->update(filterState);
    }

    void testOptions()
    {
        // check filters model data
        auto idx1 = filtersModel->index(0, 0);
        auto idx2 = filtersModel->index(1, 0);

        QCOMPARE(filtersModel->rowCount(), 2);
        QCOMPARE(filtersModel->data(idx1, unity::shell::scopes::FiltersInterface::Roles::RoleFilterId).toString(), QString("f1"));
        QCOMPARE(filtersModel->data(idx1, unity::shell::scopes::FiltersInterface::Roles::RoleFilterType).toInt(),
                static_cast<int>(unity::shell::scopes::FiltersInterface::FilterType::OptionSelectorFilter));
        QCOMPARE(filtersModel->data(idx1, unity::shell::scopes::FiltersInterface::Roles::RoleFilterId).toString(), QString("f1"));
        QCOMPARE(filtersModel->data(idx1, unity::shell::scopes::FiltersInterface::Roles::RoleFilterType).toInt(),
                static_cast<int>(unity::shell::scopes::FiltersInterface::FilterType::OptionSelectorFilter));

        {
            // get 1st option selector filter
            auto opf = filtersModel->data(idx1, uss::FiltersInterface::Roles::RoleFilter).value<OptionSelectorFilter*>();
            QVERIFY(opf != nullptr);
            QCOMPARE(opf->filterId(), QString("f1"));
            QCOMPARE(opf->label(), QString("Filter1"));
            QVERIFY(!opf->multiSelect());

            // check options
            auto opts = opf->options();
            QVERIFY(opts != nullptr);
            QCOMPARE(opts->rowCount(), 2);

            auto idx = opts->index(0, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f1o1"));
            QVERIFY(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool()); // option1 is checked
            idx = opts->index(1, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f1o2"));
            QVERIFY(!opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool());
        }

        {
            // get 2nd option selector filter
            auto opf = filtersModel->data(idx2, uss::FiltersInterface::Roles::RoleFilter).value<OptionSelectorFilter*>();
            QVERIFY(opf != nullptr);
            QCOMPARE(opf->filterId(), QString("f2"));
            QCOMPARE(opf->label(), QString("Filter2"));
            QVERIFY(opf->multiSelect());

            // check options
            auto opts = opf->options();
            QVERIFY(opts != nullptr);
            QCOMPARE(opts->rowCount(), 2);

            auto idx = opts->index(0, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f2o1"));
            QVERIFY(!opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool());
            idx = opts->index(1, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f2o2"));
            QVERIFY(!opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool());
        }
    }

    void testBackendCheckedStateChange()
    {
        auto idx1 = filtersModel->index(0, 0);
        {
            // get 1st option selector filter
            auto opf = filtersModel->data(idx1, uss::FiltersInterface::Roles::RoleFilter).value<OptionSelectorFilter*>();
            QVERIFY(opf != nullptr);
            auto opts = opf->options();
            QVERIFY(opts != nullptr);
            QCOMPARE(opts->rowCount(), 2);

            QSignalSpy dataChangedSignal(opts, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
            QSignalSpy stateChangeSignal(filtersModel.data(), SIGNAL(filterStateChanged()));

            // enable second option (which disables 1st option)
            f1->update_state(filterState, f1o2, true);
            filtersModel->update(filterState);

            QCOMPARE(dataChangedSignal.count(), 2);
            QCOMPARE(stateChangeSignal.count(), 0); // change initiated by backend, so no state change signal

            // verify arguments of dataChanged signal
            {
                auto args = dataChangedSignal.takeFirst();
                auto topLeft = args.at(0).value<QModelIndex>();
                auto roles = args.at(2).value<QVector<int>>();
                QCOMPARE(topLeft.row(), 0);
                QCOMPARE(roles[0], static_cast<int>(uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked));
            }

            auto idx = opts->index(0, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f1o1"));
            QVERIFY(!opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool()); // option 1 is now off
            idx = opts->index(1, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f1o2"));
            QVERIFY(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool()); // option 2 is now on
        }
    }

    void testBackendOptionLabelChange()
    {
        auto idx = filtersModel->index(1, 0);
        {
            // get 1st option selector filter
            auto opf = filtersModel->data(idx, uss::FiltersInterface::Roles::RoleFilter).value<OptionSelectorFilter*>();
            QVERIFY(opf != nullptr);
            QCOMPARE(opf->filterId(), QString("f2"));
            auto opts = opf->options();
            QVERIFY(opts != nullptr);
            QCOMPARE(opts->rowCount(), 2);

            auto idx1 = opts->index(0, 0);
            auto idx2 = opts->index(1, 0);
            QCOMPARE(opts->data(idx1, uss::OptionSelectorOptionsInterface::Roles::RoleOptionLabel).toString(), QString("Option1"));
            QCOMPARE(opts->data(idx2, uss::OptionSelectorOptionsInterface::Roles::RoleOptionLabel).toString(), QString("Option2"));

            QSignalSpy dataChangedSignal(opts, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
            QSignalSpy stateChangeSignal(filtersModel.data(), SIGNAL(filterStateChanged()));

            unity::scopes::OptionSelectorFilter::SPtr f3 = unity::scopes::OptionSelectorFilter::create("f2", "Filter2", true);
            auto f3o1 = f3->add_option("f2o1", "Option1");
            auto f3o2 = f3->add_option("f2o2", "UpdatedOption2");

            QList<unity::scopes::FilterBase::SCPtr> backendFilters2;
            backendFilters2.append(f1);
            backendFilters2.append(f3);

            // sync model with new backend filters
            filtersModel->update(backendFilters2);

            if (dataChangedSignal.empty()) {
                QVERIFY(dataChangedSignal.wait());
            }
            QCOMPARE(dataChangedSignal.count(), 1);

            QCOMPARE(opts->data(idx1, uss::OptionSelectorOptionsInterface::Roles::RoleOptionLabel).toString(), QString("Option1"));
            QCOMPARE(opts->data(idx2, uss::OptionSelectorOptionsInterface::Roles::RoleOptionLabel).toString(), QString("UpdatedOption2"));

            QCOMPARE(stateChangeSignal.count(), 0); // no state change signal

            // verify arguments of dataChanged signal
            {
                auto args = dataChangedSignal.takeFirst();
                auto topLeft = args.at(0).value<QModelIndex>();
                auto roles = args.at(2).value<QVector<int>>();
                QCOMPARE(topLeft.row(), 1);
                QCOMPARE(roles[0], static_cast<int>(uss::OptionSelectorOptionsInterface::Roles::RoleOptionLabel));
            }
        }
    }

    void testUICheckedStateChange()
    {
        auto idx1 = filtersModel->index(0, 0);
        {
            // get 1st option selector filter
            auto opf = filtersModel->data(idx1, uss::FiltersInterface::Roles::RoleFilter).value<OptionSelectorFilter*>();
            QVERIFY(opf != nullptr);
            auto opts = opf->options();
            QVERIFY(opts != nullptr);
            QCOMPARE(opts->rowCount(), 2);

            QSignalSpy dataChangedSignal(opts, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
            QSignalSpy stateChangeSignal(filtersModel.data(), SIGNAL(filterStateChanged()));

            // enable second option (which disables 1st option)
            opts->setChecked(1, true);

            if (stateChangeSignal.empty()) {
                stateChangeSignal.wait();
            }
            if (dataChangedSignal.empty()) {
                dataChangedSignal.wait();
            }
            QCOMPARE(stateChangeSignal.count(), 1);
            QCOMPARE(dataChangedSignal.count(), 2);

            // verify arguments of dataChanged signal
            {
                auto args = dataChangedSignal.takeFirst();
                auto topLeft = args.at(0).value<QModelIndex>();
                auto roles = args.at(2).value<QVector<int>>();
                QCOMPARE(topLeft.row(), 0);
                QCOMPARE(roles[0], static_cast<int>(uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked));
            }

            auto idx = opts->index(0, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f1o1"));
            QVERIFY(!opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool()); // option 1 is now off
            idx = opts->index(1, 0);
            QCOMPARE(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionId).toString(), QString("f1o2"));
            QVERIFY(opts->data(idx, uss::OptionSelectorOptionsInterface::Roles::RoleOptionChecked).toBool()); // option 2 is now on
        }

    }

private:
    unity::scopes::FilterState filterState;
    QScopedPointer<Filters> filtersModel;
    unity::scopes::OptionSelectorFilter::SPtr f1, f2;
    unity::scopes::FilterOption::SCPtr f1o1, f1o2, f2o1, f2o2;
    QList<unity::scopes::FilterBase::SCPtr> backendFilters;
};

QTEST_GUILESS_MAIN(OptionSelectorFilterTest)
#include <optionselectorfiltertest.moc>
