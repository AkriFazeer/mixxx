#include "util/timer.h"
#include "util/experiment.h"
#include "waveform/guitick.h"

Timer::Timer(const QString& key, Stat::ComputeFlags compute)
        : m_key(key),
          m_compute(Stat::experimentFlags(compute)),
          m_running(false) {
}

void Timer::start() {
    m_running = true;
    m_time.start();
}

qint64 Timer::restart(bool report) {
    if (m_running) {
        qint64 nsec = m_time.restart();
        if (report) {
            // Ignore the report if it crosses the experiment boundary.
            Experiment::Mode oldMode = Stat::modeFromFlags(m_compute);
            if (oldMode == Experiment::mode()) {
                Stat::track(m_key, Stat::DURATION_NANOSEC, m_compute, nsec);
            }
        }
        return nsec;
    } else {
        start();
        return 0;
    }
}

qint64 Timer::elapsed(bool report) {
    qint64 nsec = m_time.elapsed();
    if (report) {
        // Ignore the report if it crosses the experiment boundary.
        Experiment::Mode oldMode = Stat::modeFromFlags(m_compute);
        if (oldMode == Experiment::mode()) {
            Stat::track(m_key, Stat::DURATION_NANOSEC, m_compute, nsec);
        }
    }
    return nsec;
}


SuspendableTimer::SuspendableTimer(const QString& key,
                                   Stat::ComputeFlags compute)
        : Timer(key, compute),
          m_leapTime(0) {
}

void SuspendableTimer::start() {
    m_leapTime = 0;
    Timer::start();
}

qint64 SuspendableTimer::suspend() {
    m_leapTime += m_time.elapsed();
    m_running = false;
    return m_leapTime;
}

void SuspendableTimer::go() {
    Timer::start();
}

qint64 SuspendableTimer::elapsed(bool report) {
    m_leapTime += m_time.elapsed();
    if (report) {
        // Ignore the report if it crosses the experiment boundary.
        Experiment::Mode oldMode = Stat::modeFromFlags(m_compute);
        if (oldMode == Experiment::mode()) {
            Stat::track(m_key, Stat::DURATION_NANOSEC, m_compute, m_leapTime);
        }
    }
    return m_leapTime;
}

GuiTickTimer::GuiTickTimer(QObject* pParent)
        : QObject(pParent),
          m_pGuiTick50ms(new ControlObjectSlave(
              "[Master]", "guiTick50ms", this)),
          m_bActive(false) {
    m_pGuiTick50ms->connectValueChanged(SLOT(slotGuiTick50ms(double)));
}

GuiTickTimer::~GuiTickTimer() {
}

void GuiTickTimer::start(mixxx::Duration duration) {
    m_interval = duration;
    m_elapsed = mixxx::Duration::fromSeconds(0);
    m_bActive = true;
}

void GuiTickTimer::stop() {
    m_bActive = false;
    m_interval = mixxx::Duration::fromSeconds(0);
    m_elapsed = mixxx::Duration::fromSeconds(0);
}

void GuiTickTimer::slotGuiTick50ms(double) {
    if (m_bActive) {
        m_elapsed += mixxx::Duration::fromMillis(50);
        if (m_elapsed >= m_interval) {
            m_elapsed = mixxx::Duration::fromSeconds(0);
            emit(timeout());
        }
    }
}
