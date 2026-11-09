#include "task_runner.h"
#include <QThread>
#include <QDebug>

TaskRunner::TaskRunner(QObject* parent)
    : QObject(parent) {
    m_pool.setMaxThreadCount(1);  // serialize work
}

TaskRunner::~TaskRunner() {
    m_pool.clear();
    m_pool.waitForDone(5000);
}

void TaskRunner::submit(const QString& label,
                        QFunctionPointer work,
                        QFunctionPointer done,
                        QFunctionPointer fail) {
    // Convert function pointers back to lambdas via void* cast.
    auto w = *reinterpret_cast<std::function<void()>*>(work);
    auto d = *reinterpret_cast<std::function<void()>*>(done);
    auto f = fail
        ? *reinterpret_cast<std::function<void(const QString&)>*>(fail)
        : nullptr;
    run(label, std::move(w), std::move(d), std::move(f));
}

void TaskRunner::run(const QString& label,
                     std::function<void()> work,
                     std::function<void()> done,
                     std::function<void(const QString&)> fail) {
    auto* task = new Task(label, std::move(work), std::move(done), std::move(fail));
    task->setAutoDelete(true);
    m_pool.start(task);
}

// ---- Task ----
TaskRunner::Task::Task(const QString& label,
                       std::function<void()> work,
                       std::function<void()> done,
                       std::function<void(const QString&)> fail)
    : m_label(label), m_work(std::move(work)), m_done(std::move(done)), m_fail(std::move(fail)) {
}

void TaskRunner::Task::run() {
    try {
        if (m_work) m_work();
        if (m_done) m_done();
    } catch (const std::exception& e) {
        if (m_fail) m_fail(QString::fromUtf8(e.what()));
        else qDebug() << "Task" << m_label << "failed:" << e.what();
    } catch (...) {
        if (m_fail) m_fail("Errore sconosciuto in task: " + m_label);
        else qDebug() << "Unknown error in task" << m_label;
    }
}
