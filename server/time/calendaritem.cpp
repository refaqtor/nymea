/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stuerz <simon.stuerz@guh.guru>                *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \class guhserver::CalendarItem
    \brief Describes a clendar item for a time based \l{guhserver::Rule}{Rule}.

    \ingroup rules
    \inmodule core

    \sa Rule, TimeDescriptor
*/

#include "calendaritem.h"
#include "loggingcategories.h"

namespace guhserver {

/*! Construct a invalid \l{CalendarItem}. */
CalendarItem::CalendarItem():
    m_duration(0)
{

}

/*! Returns the date time of this \l{CalendarItem}. */
QDateTime CalendarItem::dateTime() const
{
    return m_dateTime;
}

/*! Sets the \a dateTime of this \l{CalendarItem}. */
void CalendarItem::setDateTime(const QDateTime &dateTime)
{
    m_dateTime = dateTime;
}

/*! Returns the start time of this \l{CalendarItem}. */
QTime CalendarItem::startTime() const
{
    return m_startTime;
}

/*! Sets the \a startTime of this \l{CalendarItem}. */
void CalendarItem::setStartTime(const QTime &startTime)
{
    m_startTime = startTime;
}

/*! Returns the duratiorn of this \l{CalendarItem}. */
uint CalendarItem::duration() const
{
    return m_duration;
}

/*! Sets the \a duration of this \l{CalendarItem}. */
void CalendarItem::setDuration(const uint &duration)
{
    m_duration = duration;
}

/*! Returns the \l{RepeatingOption} of this \l{CalendarItem}. */
RepeatingOption CalendarItem::repeatingOption() const
{
    return m_repeatingOption;
}

/*! Sets the \a repeatingOption of this \l{CalendarItem}. */
void CalendarItem::setRepeatingOption(const RepeatingOption &repeatingOption)
{
    m_repeatingOption = repeatingOption;
}

/*! Returns true if this \l{CalendarItem} is valid. A \l{CalendarItem} is invalid
    if start time and datetime are set or if the duration is 0.
*/
bool CalendarItem::isValid() const
{
    // If dateTime AND a repeating option definend...
    if (m_dateTime.isValid() && !repeatingOption().isEmtpy())
        // ...only yearly repeating mode is allowed
        if (repeatingOption().mode() != RepeatingOption::RepeatingModeYearly)
            return false;

    return (!m_startTime.isValid() != !m_dateTime.isValid()) && m_duration > 0;
}

/*! Returns true, if the given \a dateTime matches this \l{CalendarItem}. */
bool CalendarItem::evaluate(const QDateTime &dateTime) const
{
    if (m_startTime.isValid()) {
        switch (m_repeatingOption.mode()) {
        case RepeatingOption::RepeatingModeNone:
            // If there is no repeating option, we assume it is meant daily.
            return evaluateDaily(dateTime);
        case RepeatingOption::RepeatingModeHourly:
            return evaluateHourly(dateTime);
        case RepeatingOption::RepeatingModeDaily:
            return evaluateDaily(dateTime);
        case RepeatingOption::RepeatingModeWeekly:
            return evaluateWeekly(dateTime);
        case RepeatingOption::RepeatingModeMonthly:
            return evaluateMonthly(dateTime);
        case RepeatingOption::RepeatingModeYearly:
            return evaluateYearly(dateTime);
        default:
            return false;
        }
    } else {
        return dateTime >= m_dateTime && dateTime < m_dateTime.addSecs(duration() * 60);
    }
    return false;
}

bool CalendarItem::evaluateHourly(const QDateTime &dateTime) const
{
    // If the duration is longer than a hour, this calendar item is always true
    // 1 hour has 60 minutes
    if (duration() >= 60)
        return true;

    QDateTime startDateTime = QDateTime(dateTime.date(), QTime(dateTime.time().hour(), startTime().minute()));
    QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

    bool timeValid = dateTime >= startDateTime && dateTime < endDateTime;
    bool weekdayValid = repeatingOption().evaluateWeekDay(dateTime);
    bool monthdayValid = repeatingOption().evaluateMonthDay(dateTime);

    return timeValid && weekdayValid && monthdayValid;
}

bool CalendarItem::evaluateDaily(const QDateTime &dateTime) const
{
    // If the duration is longer than a day, this calendar item is always true
    // 1 day has 1440 minutes
    if (duration() >= 1440)
        return true;

    // get todays startTime
    QDateTime startDateTime = QDateTime(dateTime.date(), startTime());
    QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

    // get todays startTime
    QDateTime startDateTimeYesterday = QDateTime(dateTime.date().addDays(-1), startTime());
    QDateTime endDateTimeYesterday = startDateTimeYesterday.addSecs(duration() * 60);

    bool todayValid = dateTime >= startDateTime && dateTime < endDateTime;
    bool yesterdayValid = dateTime >= startDateTimeYesterday && dateTime < endDateTimeYesterday;

    return todayValid || yesterdayValid;
}

bool CalendarItem::evaluateWeekly(const QDateTime &dateTime) const
{
    // If the duration is longer than a week, this calendar item is always true
    // 1 week has 10080 minutes
    if (duration() >= 10080)
        return true;

    // Get the first day of this week with the correct start time
    QDateTime weekStartDateTime = dateTime.addDays(-dateTime.date().dayOfWeek());
    weekStartDateTime.setTime(m_startTime);

    // Check each week day in the list
    foreach (const int &weekDay, repeatingOption().weekDays()) {
        QDateTime startDateTime = weekStartDateTime.addDays(weekDay);
        QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

        // Check if dateTime matches for this week
        if (dateTime >= startDateTime && dateTime < endDateTime)
            // Return true if the current time is between start
            // and end of this calendar item
            return true;

        // Check if this calendar item overlaps a week...
        if (startDateTime.date().weekNumber() != endDateTime.date().weekNumber()) {
            // ...jump one week back in to the past
            QDateTime startDateTimePreviouseWeek = startDateTime.addDays(-7);
            QDateTime endDateTimePreviouseWeek = startDateTimePreviouseWeek.addSecs(duration() * 60);

            if (dateTime >= startDateTimePreviouseWeek && dateTime < endDateTimePreviouseWeek)
                // Return true if the current time is between start
                // and end of this calendar item from the previouse week
                return true;

        }
    }

    return false;
}

bool CalendarItem::evaluateMonthly(const QDateTime &dateTime) const
{
    // Get the first day of this month with the correct start time
    QDateTime monthStartDateTime = QDateTime(QDate(dateTime.date().year(), dateTime.date().month(), 1), m_startTime);

    // Check each month day in the list
    foreach (const int &monthDay, repeatingOption().monthDays()) {
        QDateTime startDateTime = monthStartDateTime.addDays(monthDay - 1);
        QDateTime endDateTime = startDateTime.addSecs(duration() * 60);

        // Check if this calendar item starts in the future...
        if (startDateTime > dateTime) {
            //...go one month back
            startDateTime = startDateTime.addMonths(-1);
            endDateTime =  startDateTime.addSecs(duration() * 60);
        }

        // Check if dateTime already matches for this month
        if (dateTime >= startDateTime && dateTime < endDateTime)
            // Return true if the current time is between start
            // and end of this calendar item
            return true;

        // Check if this calendar item overlaps a month...
        if (startDateTime.date().month() != endDateTime.date().month()) {
            // ...jump one month back in to the past
            QDateTime startDateTimePreviouseMonth = startDateTime.addMonths(-1);
            QDateTime endDateTimePreviouseMonth = startDateTimePreviouseMonth.addSecs(duration() * 60);

            if (dateTime >= startDateTimePreviouseMonth && dateTime < endDateTimePreviouseMonth)
                // Return true if the current time is between start
                // and end of this calendar item from the previouse month
                return true;
        }
    }

    return false;
}

bool CalendarItem::evaluateYearly(const QDateTime &dateTime) const
{
    Q_UNUSED(dateTime)

    return false;
}

}
